#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char	*g_name = "basename";

void	fatal(char *message)
{
	fprintf(stderr, "%s: %s\n", g_name, message);
	exit(EXIT_FAILURE);
}

char	*truncate_path(char *s, char *suffix)
{
	if (*s == '\0')
		return s;

	size_t last = strlen(s) - 1;
	while (s[last] == '/' && last != 0)
	{
		s[last] = '\0';
		last--;
	}

	char *last_slash = strrchr(s, '/');
	if (last_slash != NULL && last_slash != s)
		s = last_slash + 1;

	if (suffix != NULL)
	{
		char *end = s + strlen(s) - strlen(suffix);
		if (end > s && strcmp(end, suffix) == 0)
			*end = '\0';
	}
	return s;
}

int		main(int argc, char **argv)
{
	int		option;
	char	*suffix = NULL;
	bool	multiple = false;
	char	line_delim = '\n';

	g_name = argv[0];
	while ((option = getopt(argc, argv, "as:z")) != -1)
	{
		switch (option)
		{
			case 's':
				suffix = optarg;
			case 'a':
				multiple = true;
				break;
			case 'z':
				line_delim = '\0';
				break;
			default:
				return EXIT_FAILURE;
		}
	}

	if (optind == argc)
		fatal("missing operand");

	if (!multiple)
	{
		argv[optind] = truncate_path(argv[optind], argv[optind + 1]);
		fputs(argv[optind], stdout);
		putchar(line_delim);
		return EXIT_SUCCESS;
	}

	for (; optind < argc; optind++)
	{
		argv[optind] = truncate_path(argv[optind], suffix);
		fputs(argv[optind], stdout);
		putchar(line_delim);
	}
	return EXIT_SUCCESS;
}
