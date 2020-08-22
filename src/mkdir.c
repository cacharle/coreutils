#define _POSIX_C_SOURCE 2

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

char	*g_name = "mkdir";

int		mkdir_wrapper(const char *path, mode_t mode, bool verbose)
{
	int	ret;

	if ((ret = mkdir(path, mode)) == -1 && (verbose || errno != EEXIST))
		fprintf(stderr, "%s: cannot create directory '%s': %s\n", g_name, path, strerror(errno));
	else if (verbose)
		printf("%s: created directory '%s'\n", g_name, path);
	return ret;
}

int main(int argc, char **argv)
{
	int		option;
	mode_t	mode    = 0755;
	bool	parent  = false;
	bool	verbose = false;

	g_name = argv[0];
	if (argc == 1)
	{
		fprintf(stderr, "%s: missing operand\n", g_name);
		return EXIT_FAILURE;
	}
	while ((option = getopt(argc, argv, "m:pv")) != -1)
	{
		switch (option)
		{
			case 'm':
				sscanf(optarg, "%o", &mode); // TODO mode not working
				break;
			case 'p':
				parent = true;
				break;
			case 'v':
				verbose = true;
				break;
		}
	}
	for (; optind < argc; optind++)
	{
		// remove duplicate slash
		for (size_t	i = 0; argv[optind][i] != '\0'; i++)
		{
			if (argv[optind][i] == '/' && argv[optind][i + 1] == '/')
			{
				memmove(&argv[optind][i], &argv[optind][i + 1], strlen(&argv[optind][i + 1]) + 1);
				i--;
			}
		}

		if (parent)
		{
			char	*tmp;
			char	*searched = argv[optind];

			if (*searched == '/')
				searched++;
			for (; (tmp = strchr(searched, '/')) != NULL; searched = tmp + 1)
			{
				if (tmp[1] == '\0')
					continue;
				*tmp = '\0';
				mkdir_wrapper(argv[optind], mode, verbose);
				*tmp = '/';
			}
			mkdir_wrapper(argv[optind], mode, verbose);
		}
		else
			mkdir_wrapper(argv[optind], mode, verbose);
	}
	return EXIT_SUCCESS;
}
