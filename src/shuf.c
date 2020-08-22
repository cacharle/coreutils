#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define NUMBER_LEN(x) strlen(#x);

static char *g_name = "shuf";

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

long parse_ulong(char *str, char *error_msg)
{
	char *end;
	long ret;

	errno = 0;
	ret = strtol(str, &end, 10);
	if (errno != 0 || ret < 0 || *end != '\0')
		fatal("%s '%s'", error_msg, optarg);
	return ret;
}

typedef struct
{
	char **data;
	size_t len;
	size_t cap;
} t_lines;

#define INITIAL_CAPACITY 64

void lines_init(t_lines *lines)
{
	lines->len = 0;
	lines->cap = INITIAL_CAPACITY;
	lines->data = malloc(lines->cap * sizeof(char*));
	if (lines->data == NULL)
		fatal("%s", strerror(errno));
}

void lines_destroy(t_lines *lines)
{
	for (size_t i = 0; i < lines->len; i++)
		free(lines->data[i]);
	free(lines->data);
}

#define GROWTH_FACTOR 2

void lines_push(t_lines *lines, char *line)
{
	if (lines->len >= lines->cap)
	{
		lines->cap *= GROWTH_FACTOR;
		lines->data = realloc(lines->data, lines->cap * sizeof(char*));
		if (lines->data == NULL)
			fatal("%s", strerror(errno));
	}
	lines->data[lines->len] = line;
	lines->len++;
}

#define RANDOM_FILEPATH "/dev/random"

int main(int argc, char **argv)
{
	int  option;
	long max_output       = -1;
	char *output_filepath = NULL;
	bool repeat           = false;
	bool echo             = false;
	char delimiter        = '\n';
	long range_start      = -1;
	long range_stop       = -1;

	while ((option = getopt(argc, argv, "ei:n:o:rz")) != -1)
	{

		switch (option)
		{
			case 'e': echo            = true;   break;
			case 'r': repeat          = true;   break;
			case 'o': output_filepath = optarg; break;
			case 'z': delimiter       = '\0';   break;

			case 'i':
			{
				char *hyphen = strchr(optarg, '-');
				if (hyphen == NULL)
					fatal("invalid input range '%s'", optarg);
				*hyphen = '\0';
				range_start = parse_ulong(optarg, "invalid input range");
				*hyphen = '-';
				range_stop  = parse_ulong(hyphen + 1, "invalid input range");
				if (range_start > range_stop)
					fatal("invalid input range '%s'", optarg);
				break;
			}

			case 'n':
				max_output = parse_ulong(optarg, "invalid line count");
				break;
		}

	}
	if (range_start != -1 && echo)
		fatal("cannot combine -e and -i options");
	if (range_start != -1 && optind != argc)
		fatal("extra operand '%s'", argv[optind]);
	if (!echo && argc - optind > 1)
		fatal("extra operand '%s'", argv[optind]);

	unsigned int seed;
	FILE *seed_file = fopen(RANDOM_FILEPATH, "r");
	if (seed_file == NULL)
		fatal("%s: %s", RANDOM_FILEPATH, strerror(errno));
	fread(&seed, sizeof(seed), 1, seed_file);
	srand(seed);

	t_lines lines;
	lines_init(&lines);

	if (range_start != -1)
	{
		size_t buf_len = NUMBER_LEN(LONG_MAX);
		for (; range_start <= range_stop; range_start++)
		{
			char *line = malloc(buf_len * sizeof(char));
			if (line == NULL)
				fatal("%s", strerror(errno));
			sprintf(line, "%ld", range_start);
			lines_push(&lines, line);
		}
	}
	else if (echo)
	{
		for (; optind < argc; optind++)
		{
			char *line = strdup(argv[optind]);
			if (line == NULL)
				fatal("%s", strerror(errno));
			lines_push(&lines, line);
		}
	}
	else
	{
		FILE *input = stdin;
		if (optind != argc)
		{
			input = fopen(argv[optind], "r");
			if (input == NULL)
				fatal("%s: %s", argv[optind], strerror(errno));
		}

		char   *line     = NULL;
		size_t line_size = 0;
		errno = 0;
		while (getdelim(&line, &line_size, delimiter, input) != -1)
		{
			lines_push(&lines, line);
			line = NULL;
			line_size = 0;
		}
		if (errno != 0)
		{
			fclose(input);
			fatal("%s: %s", argv[optind], strerror(errno));
		}
		fclose(input);
	}

	if (lines.len == 0)
	{
		lines_destroy(&lines);
		return EXIT_SUCCESS;
	}
	for (size_t i = lines.len - 1; i > 0; i--)
	{
		size_t j = rand() % i;
		char *tmp = lines.data[i];
		lines.data[i] = lines.data[j];
		lines.data[j] = tmp;
	}

	FILE *output = stdout;
	if (output_filepath != NULL)
	{
		output = fopen(output_filepath, "w");
		if (output == NULL)
			fatal("%s: %s", output_filepath, strerror(errno));
	}

	if (repeat)
	{
		for (size_t i = 0; true; i = (i + 1) % lines.len)
		{
			if (fputs(lines.data[i], output) == EOF)
				break;
			if (echo || range_start != -1)
				if (fputc('\n', output) == EOF)
					break;
		}
	}
	else
	{
		for (size_t i = 0;
			 max_output == -1 ? (i < lines.len) : (i < (unsigned long)max_output);
			 i++)
		{
			if (fputs(lines.data[i], output) == EOF)
				break;
			if (echo || range_start != -1)
				if (fputc('\n', output) == EOF)
					break;
		}
	}
	fclose(output);

	lines_destroy(&lines);
	return EXIT_SUCCESS;
}
