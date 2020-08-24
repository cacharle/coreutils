#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static char *g_name = "touch";

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

enum flags
{
	FLAG_ACCESS_ONLY       = 1 << 0,
	FLAG_MODIFICATION_ONLY = 1 << 1,
	FLAG_NO_CREATE         = 1 << 2,
	FLAG_NO_DEREFERENCE    = 1 << 3,
};

int main(int argc, char **argv)
{
	int        option;
	enum flags flags = 0;
	char       *reference_filepath = NULL;

	g_name = argv[0];
	while ((option = getopt(argc, argv, "acd:fhr:t")) != -1)
	{
		switch (option)
		{
			case 'a': flags |= FLAG_ACCESS_ONLY;       break;
			case 'm': flags |= FLAG_MODIFICATION_ONLY; break;
			case 'c': flags |= FLAG_NO_CREATE;         break;
			case 'h': flags |= FLAG_NO_DEREFERENCE;    break;

			case 'd': exit(1); break;
			case 't': exit(1); break;

			case 'r': reference_filepath = optarg; break;

			case 'f': break;
		}
	}
	if (optind == argc)
		fatal("missing file operand");

	struct timeval times[2];
	if (reference_filepath != NULL)
	{
		struct stat statbuf;
		if (stat(reference_filepath, &statbuf) == -1)
			fatal("%s", strerror(errno));
		times[0].tv_sec  = statbuf.st_atim.tv_sec;
		times[0].tv_usec = statbuf.st_atim.tv_nsec / 1000;
		times[1].tv_sec  = statbuf.st_mtim.tv_sec;
		times[1].tv_usec = statbuf.st_mtim.tv_nsec / 1000;
	}
	else
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		memcpy(&times[0], &now, sizeof(struct timeval));
		memcpy(&times[1], &now, sizeof(struct timeval));
	}

	for (; optind < argc; optind++)
	{
		struct stat statbuf;
		if (stat(argv[optind], &statbuf) == -1)
		{
			if (!(flags & FLAG_NO_CREATE))
				if (close(creat(argv[optind], 0644)) == -1)
				{
					fprintf(stderr, "%s: cannot create '%s': %s", g_name, argv[optind], strerror(errno));
					continue;
				}
			fprintf(stderr, "%s: cannot atouch '%s': %s", g_name, argv[optind], strerror(errno));
			continue;
		}
		if (flags & FLAG_ACCESS_ONLY)
		{
			times[1].tv_sec  = statbuf.st_mtim.tv_sec;
			times[1].tv_usec = statbuf.st_mtim.tv_nsec / 1000;
		}
		else if (flags & FLAG_MODIFICATION_ONLY)
		{
			times[0].tv_sec  = statbuf.st_atim.tv_sec;
			times[0].tv_usec = statbuf.st_atim.tv_nsec / 1000;
		}
		if (utimes(argv[optind], times) == -1)
			fprintf(stderr, "%s: cannot touch '%s': %s", g_name, argv[optind], strerror(errno));
	}

	return EXIT_SUCCESS;
}
