#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

static char *g_name = "uniq";

void fatal(const char *format, ...)
{
	va_list	ap;

	va_start(ap, format);
	fprintf(stderr, "%s: ", g_name);
	vfprintf(stderr, format, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(EXIT_FAILURE);
}

char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (ret == NULL)
		fatal("%s", strerror(errno));
	return ret;
}

long parse_ulong(char *str, char *error_msg)
{
	char *end;
	long ret;

	errno = 0;
	ret = strtol(str, &end, 10);
	if (errno != 0 || ret < 0 || *end != '\0')
		fatal("%s: %s", optarg, error_msg);
	return ret;
}

enum flags
{
	FLAG_COUNT        = 1 << 0,
	FLAG_REPEATED     = 1 << 1,
	FLAG_REPEATED_ALL = 1 << 2,
	FLAG_IGNORE_CASE  = 1 << 3,
	FLAG_UNIQUE_ONLY  = 1 << 4,
};

bool line_equal(char *line1, char *line2, enum flags flags)
{
	if (flags & FLAG_IGNORE_CASE)
		return (strcasecmp(line1, line2) == 0);
	return (strcmp(line1, line2) == 0);
}

void line_print(char *line, enum flags flags, FILE *output, int counter)
{
	if (flags & FLAG_COUNT)
		fprintf(output, "%7u ", counter);
	fputs(line, output);
}

int main(int argc, char **argv)
{
	int        option;
	enum flags flags       = 0;
	long       skip_fields = -1;
	long       skip_chars  = -1;
	long       width       = -1;
	char       delimiter   = '\n';

	while ((option = getopt(argc, argv, "cdDf:is:uzw:")) != -1)
	{
		switch (option)
		{
			case 'c': flags |= FLAG_COUNT;        break;
			case 'd': flags |= FLAG_REPEATED;     break;
			case 'D': flags |= FLAG_REPEATED_ALL; break;
			case 'i': flags |= FLAG_IGNORE_CASE;  break;
			case 'u': flags |= FLAG_UNIQUE_ONLY;  break;
			case 'z': delimiter = '\0';           break;

			case 'f': skip_fields = parse_ulong(optarg, "invalid number of fields to skip");    break;
			case 's': skip_chars  = parse_ulong(optarg, "invalid number of bytes to skip");     break;
			case 'w': width       = parse_ulong(optarg, "invalid number of bytes to compare");  break;
		}
	}

	FILE *input  = stdin;
	FILE *output = stdout;

	if (argc - optind > 2)
		fatal("extra operand '%s'", argv[optind + 2]);
	if (optind < argc)
	{
		input = fopen(argv[optind], "r");
		if (input == NULL)
			fatal("%s: %s", argv[optind], strerror(errno));
		if (optind + 1 < argc)
		{
			output = fopen(argv[optind + 1], "w");
			if (output == NULL)
				fatal("%s: %s", argv[optind + 1], strerror(errno));
		}
	}

	char   *line_prev = NULL;
	char   *line      = NULL;
	size_t line_size  = 0;
	size_t counter    = 1;

	if (getdelim(&line_prev, &line_size, delimiter, input) == -1)
		return EXIT_SUCCESS;
	while (getdelim(&line, &line_size, delimiter, input) != -1)
	{
		if (line_equal(line, line_prev, flags))
		{
			counter++;
		}
		else
		{
			line_print(line_prev, flags, output, counter);
			counter = 1;
			free(line_prev);
			line_prev = xstrdup(line);
		}
	}
	line_print(line_prev, flags, output, counter);
	return EXIT_SUCCESS;
}
