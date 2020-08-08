#define _POSIX_C_SOURCE 2 // getopt
#define _XOPEN_SOURCE 500 // sigaction
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE 2048

char	*g_name = "tee";

int		main(int argc, char **argv)
{
	int		option;
	bool	append = false;

	g_name = argv[0];

	struct sigaction action;
	action.sa_handler = SIG_IGN;
	action.sa_flags = SA_NODEFER;

	while ((option = getopt(argc, argv, "ai")) != -1)
	{
		switch (option)
		{
			case 'a':
				append = true;
				break;
			case 'i':
				sigaction(SIGINT, &action, NULL);
				break;
		}
	}

	size_t	files_num = argc - optind;
	FILE	**files = calloc(files_num, sizeof(FILE*));
	if (files == NULL)
		return EXIT_FAILURE;

	for (size_t i = 0; i < files_num; i++)
	{
		files[i] = fopen(argv[optind + i], append ? "a" : "w");
		if (files[i] == NULL)
		{
			fprintf(stderr, "%s: %s: %s\n", g_name, argv[optind + i], strerror(errno));
			i--;
			files_num--;
		}
	}

	char	buf[BUFFER_SIZE + 1] = {0};
	size_t	read_size = 0;
	while ((read_size = fread(buf, 1, BUFFER_SIZE, stdin)) > 0)
	{
		fwrite(buf, 1, read_size, stdout);
		for (size_t i = 0; i < files_num; i++)
			fwrite(buf, 1, read_size, files[i]);
	}

	for (size_t i = 0; i < files_num; i++)
		fclose(files[i]);
	free(files);
	return EXIT_SUCCESS;
}
