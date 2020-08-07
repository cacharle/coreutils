#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#define LIST_VALUE_INFINITY -1
#define LIST_VALUE_SINGLETON -2

#define LINE_BUFFER_SIZE 2024

typedef enum
{
	LIST_BYTE,
	LIST_CHAR,
	LIST_FIELD,
	LIST_UNDEFINED,
}		t_list_type;

char	*g_name = "cut";

void	fatal_msg(char *message)
{
	fprintf(stderr, "%s: %s\n", g_name, message);
	exit(EXIT_FAILURE);
}

void	fatal_errno(void)
{
	perror(g_name);
	exit(EXIT_FAILURE);
}

int		parse_uint(char *s, char **endptr)
{
	if (!isdigit(*s))
		return -1;
	unsigned long x = strtoul(s, endptr, 10);
	return (int)x;
}

int 	main(int argc, char **argv)
{
	int					option;
	int	list_start = LIST_VALUE_INFINITY;
	int	list_end   = LIST_VALUE_INFINITY;
	t_list_type			list_type  = LIST_UNDEFINED;
	wchar_t				delimiter  = '\t';
	bool				print_only_delimiter = false;
	char				line_delimiter = '\n';

	g_name = argv[0];
	while ((option = getopt(argc, argv, "b:c:d:f:sz")) > 0)
	{

		switch (option)
		{
			case 'b':
			case 'c':
			case 'f':
				if (list_type != LIST_UNDEFINED)
					fatal_msg("only one type of list may be specified");

				switch (option)
				{
					case 'b': list_type = LIST_BYTE; break;
					case 'c': list_type = LIST_CHAR; break;
					case 'f': list_type = LIST_FIELD; break;
				}

				char *ptr = optarg;

				char *hyphen = strchr(ptr, '-');
				if (hyphen == NULL) // N
				{
					list_start = parse_uint(ptr, &ptr);
					if (*ptr != '\0')
						fatal_msg("");
					list_end = LIST_VALUE_SINGLETON;
				}
				else
				{
					if (ptr == hyphen) // -M
					{
						list_end = parse_uint(ptr + 1, &ptr);
					printf(">> %d %d |%s|\n", list_start, list_end, ptr);
						if (*ptr != '\0')
							fatal_msg("");
					}
					else
					{
						list_start = parse_uint(ptr, &ptr);
						if (*ptr != '-')
							fatal_msg("");
						ptr++;
						if (*ptr != '\0') // N-M
						{
							list_end = parse_uint(ptr, &ptr);
						}
						if (*ptr != '\0')
							fatal_msg("");
					}
				}
				break;

			case 'd':
				delimiter = optarg[0];
				if (delimiter != '\0' && optarg[1] != '\0')
					fatal_msg("the delimiter must be a single character");
				break;

			case 's':
				print_only_delimiter = true;
				break;
			case 'z':
				line_delimiter = '\0';
				break;
		}
	}
	if (list_type == LIST_UNDEFINED)
		fatal_msg("you must specify a list of bytes, characters, or fields");
	if (list_type != LIST_FIELD && delimiter != '\t')
		fatal_msg("an input delimiter may be specified only when operating on fields");
	if (list_type != LIST_FIELD && print_only_delimiter)
		fatal_msg("suppressing non-delimited lines makes sense only when operating on fields");

	if (list_start == 0)
		fatal_msg("list are numbered from 1");
	if (list_end > 0 && list_start > list_end)
		fatal_msg("invalid decreasing range");

	char	*line = NULL;
	size_t	line_buffer_size = LINE_BUFFER_SIZE;
	ssize_t	read_size = -1;

	if ((line = malloc(LINE_BUFFER_SIZE)) == NULL)
		exit(1);

	char *field;
	int counter = 1;
	char delimiter_buf[2] = {delimiter, '\0'};

	if (optarg == NULL)
	{
		errno = 0;
		while ((read_size = getdelim(&line, &line_buffer_size, line_delimiter, stdin)) > 0)
		{
			switch (list_type)
			{
				case LIST_BYTE:
				case LIST_CHAR: // TODO unicode
					if (list_end == LIST_VALUE_SINGLETON)
					{
						if (read_size - 1 >= list_start)
							fputc(line[list_start - 1], stdout);
					}
					else if (list_start == LIST_VALUE_INFINITY)
					{
						line[list_end] = '\0';
						fputs(line, stdout);
					}
					else if (list_end == LIST_VALUE_INFINITY)
					{
						fputs(&line[list_start - 1], stdout);
					}
					else
					{
						line[list_end] = '\0';
						fputs(&line[list_start - 1], stdout);
					}
					break;
				case LIST_FIELD:
					while ((field = strsep(&line, delimiter_buf)) != NULL)
					{
						if (list_end == LIST_VALUE_SINGLETON && counter == list_start)
							fputs(field, stdout);
						else if (list_start == LIST_VALUE_INFINITY && counter <= list_end)
							fputs(field, stdout);
						else if (list_end == LIST_VALUE_INFINITY && counter >= list_start)
							fputs(field, stdout);
						else if (counter >= list_start && counter <= list_end)
							fputs(field, stdout);
						counter++;
					}
					break;
			}
			putchar('\n');
		}
		free(line);
		if (errno != 0)
			fatal_errno();
		return EXIT_SUCCESS;
	}


	/* while (optarg != NULL) */
	/* { */
	/* 	if (strcmp(optarg, "-") == 0) */
	/* 	{ */
	/* 		// read stdin */
	/* 	} */
	/* 	else */
	/* 	{ */
	/* 		FILE *file; */
    /*  */
	/* 		if ((file = fopen(optarg, "r")) == NULL) */
	/*			fatal_errno(); */
    /*  */
	/* 		while (getdelim(&line, 0, line_delimiter, file)) */
	/* 		{ */
	/* 			if (field) */
	/* 				strsep(line, delimiter); */
	/* 		} */
	/* 	} */
	/* } */



	return EXIT_SUCCESS;
}
