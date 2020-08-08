#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

typedef enum
{
	VLEVEL_QUIET,
	VLEVEL_NORMAL,
	VLEVEL_VERBOSE,
}	t_verbose_level;

typedef enum
{
	UNIT_LINE,
	UNIT_BYTE,
}	t_unit;

char	*g_name = "head";

#define UNIT_BYTE_BUFFER_SIZE 1028

void	fatal_printf(const char *format, ...)
{
	va_list	ap;

	va_start(ap, format);
	fprintf(stderr, "%s: ", g_name);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

int	file_head(FILE *file,
			  char *header, t_unit unit,
			  unsigned long count, char line_delimiter)
{
	char	buf[UNIT_BYTE_BUFFER_SIZE] = {'\0'};
	size_t	read_size;
	size_t	write_size;
	char	*line = NULL;
	size_t	line_size = 0;
	ssize_t	ret;

	if (header != NULL)
		printf("==> %s <==\n", header);
	switch (unit)
	{
		case UNIT_BYTE:
			while (count > 0)
			{
				read_size = count > UNIT_BYTE_BUFFER_SIZE ? UNIT_BYTE_BUFFER_SIZE : count;
				write_size = fread(buf, 1, read_size, file);
				fwrite(buf, 1, write_size, stdout);
				if (write_size < read_size)
					break;
				count -= read_size;
			}
			break;

		case UNIT_LINE:
			for (; count > 0 && (ret = getdelim(&line, &line_size, line_delimiter, file)) != -1; count--)
				fwrite(line, 1, ret, stdout);
			free(line);
			break;
	}
	return 0;
}

int	file_path_head(char *path,
				   char *header, t_unit unit,
				   unsigned long count, char line_delimiter)
{
	FILE *file;

	if ((file = fopen(path, "r")) == NULL)
	{
		fprintf(stderr, "%s: cannot open '%s' for reading: %s\n", g_name, path, strerror(errno));
		return -1;
	}
	file_head(file, header, unit, count, line_delimiter);
	fclose(file);
	return 0;
}

int main(int argc, char **argv)
{
	int				option;
	t_verbose_level	verbose_level = VLEVEL_NORMAL;
	t_unit			unit = UNIT_LINE;
	unsigned long	count = 10;
	char			line_delimiter = '\n';

	g_name = argv[0];

	while ((option = getopt(argc, argv, "c:n:qvz")) != -1)
	{
		switch (option)
		{
			case 'c':
			case 'n':
				if (strchr(optarg, '-') != NULL)
					fatal_printf("invalid number of bytes: '%s'\n", optarg);
				char	*endptr;
				errno = 0;
				// TODO '-' prefix result in tail
				// TODO human readable size suffix
				count = strtoul(optarg, &endptr, 10);
				if (*endptr != '\0')
					fatal_printf("invalid number of bytes: '%s'\n", optarg);
				if (errno != 0 || *endptr != '\0')
					fatal_printf("invalid number of bytes: '%s': %s\n", optarg, strerror(errno));
				switch (option)
				{
					case 'c': unit = UNIT_BYTE; break;
					case 'n': unit = UNIT_LINE; break;
				}
				break;

			case 'q': verbose_level = VLEVEL_QUIET;   break;
			case 'v': verbose_level = VLEVEL_VERBOSE; break;
			case 'z': line_delimiter = '\0';          break;
		}
	}

	if (argv[optind] == NULL)
	{
		file_head(
			stdin, verbose_level == VLEVEL_VERBOSE ? "standard input" : NULL,
			unit, count, line_delimiter);
		return EXIT_SUCCESS;
	}

	if (argv[optind + 1] == NULL)
	{
		if (strcmp(argv[optind], "-") == 0)
			file_head(
				stdin, verbose_level == VLEVEL_VERBOSE ? "standard input" : NULL,
				unit, count, line_delimiter);
		else
			file_path_head(
				argv[optind], verbose_level == VLEVEL_VERBOSE ? argv[optind] : NULL,
				unit, count, line_delimiter);
		return EXIT_SUCCESS;
	}

	for (; optind < argc; optind++)
	{
		int	ret = 0;

		if (strcmp(argv[optind], "-") == 0)
			ret = file_head(
				stdin, verbose_level != VLEVEL_QUIET ? "standard input" : NULL,
				unit, count, line_delimiter);
		else
			ret = file_path_head(
				argv[optind], verbose_level != VLEVEL_QUIET ? argv[optind] : NULL,
				unit, count, line_delimiter);
		if (argv[optind + 1] != NULL && ret != -1)
			putchar('\n');
	}
	return EXIT_SUCCESS;
}
