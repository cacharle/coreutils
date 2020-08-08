#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef struct
{
	long double	value;
	int			precision;
	int			integer_len;
}				t_num;

char			*g_name = "seq";

int				parse_num(char *s, t_num *num)
{
	char	*tmp;

	errno = 0;
	num->value = strtold(s, &tmp);
	if (errno != 0 || *tmp != '\0')
	{
		fprintf(stderr, "%s: invalid floating point argument: '%s'\n", g_name, s);
		exit(EXIT_FAILURE);
	}
	num->precision = 0;
	num->integer_len = strlen(s);
	if ((tmp = strchr(s, '.')) != NULL)
	{
		num->precision = strlen(tmp + 1);
		num->integer_len = tmp - s;
	}
	return 0;
}

int				main(int argc, char **argv)
{
	if (argc == 1)
	{
		fprintf(stderr, "%s: missing operand\n", argv[0]);
		return EXIT_FAILURE;
	}

	int		option;
	char	*separator = "\n";
	bool	padding = false;

	g_name = argv[0];

	while ((option = getopt(argc, argv, "f:s:w0123456789")) != -1)
	{
		if (isdigit(option))
			break;
		switch (option)
		{
			case 'f':
				exit(EXIT_FAILURE); // TODO
				break;
			case 's':
				separator = optarg;
				break;
			case 'w':
				padding = true;
				break;
		}
	}

	t_num first;
	t_num increment;
	t_num last;

	first.value = 1.0;
	first.precision = 0;
	increment.value = 1.0;
	increment.precision = 0;

	switch (argc - optind)
	{
		case 1:
			parse_num(argv[optind], &last);
			break;
		case 2:
			parse_num(argv[optind], &first);
			parse_num(argv[optind + 1], &last);
			break;
		case 3:
			parse_num(argv[optind], &first);
			parse_num(argv[optind + 1], &increment);
			if (increment.value == 0)
			{
				fprintf(stderr, "%s: invalid Zero increment value '%s'\n", argv[0], argv[optind + 3]);
				exit(EXIT_FAILURE);
			}
			parse_num(argv[optind + 2], &last);
			break;
		default:
			fprintf(stderr, "%s: extra operand '%s'\n", argv[0], argv[optind + 3]);
			exit(EXIT_FAILURE);
			break;
	}

	while (increment.value > 0 ? (first.value <= last.value) : (first.value >= last.value))
	{
		int precision = MAX(first.precision, increment.precision);
		if (!padding)
			printf("%.*llf", precision, first.value);
		else
		{
			int width = MAX(last.integer_len, first.integer_len);
			if (precision != 0)
				width += precision + 1;
			printf("%0*.*llf", width, precision, first.value);
		}
		first.value += increment.value;
		if (increment.value > 0 ? (first.value <= last.value) : (first.value >= last.value))
			fputs(separator, stdout);
	}
	putchar('\n');
	return EXIT_SUCCESS;
}
