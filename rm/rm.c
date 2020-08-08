#define _POSIX_C_SOURCE 200112L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef enum
{
	FLAG_FORCE            = 1 << 0,
	FLAG_INTERACTIVE      = 1 << 1,
	/* FLAG_INTERACTIVE_ONCE = 1 << 2, */ // TODO
	FLAG_RECURSIVE        = 1 << 3,
	FLAG_DIRECTORY        = 1 << 4,
	FLAG_VERBOSE          = 1 << 5,
}	t_flags;

char	*g_name = "rm";

void	log_errno(char *path)
{
	fprintf(stderr, "%s: cannot remove '%s': %s\n", g_name, path, strerror(errno));
}

void	fatal_errno(char *path)
{
	log_errno(path);
	exit(EXIT_FAILURE);
}

int		rm_file(char *path, t_flags flags)
{
	struct stat	statbuf;
	char		recursive_path[PATH_MAX + 1] = {'\0'};

	if (lstat(path, &statbuf) == -1)
		fatal_errno(path);
	if (flags & FLAG_INTERACTIVE
		|| (!(flags & FLAG_FORCE)
		&& !((statbuf.st_mode & S_IWUSR && statbuf.st_uid == getuid()) ||
			 (statbuf.st_mode & S_IWGRP && statbuf.st_gid == getgid()) ||
			  statbuf.st_mode & S_IWOTH)))
	{
		printf("%s: remove regular file '%s'? ", g_name, path);
		if (tolower(getchar()) != 'y')
			return 0;
	}

	if (S_ISDIR(statbuf.st_mode) && flags & FLAG_RECURSIVE)
	{
		DIR				*directory;
		struct dirent	*entry;

		if ((directory = opendir(path)) == NULL)
		{
			log_errno(path);
			return 0;
		}
		strcpy(recursive_path, path);
		if (path[strlen(path) - 1] != '/')
			strcat(recursive_path, "/");
		while ((entry = readdir(directory)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			strcat(recursive_path, entry->d_name);
			rm_file(recursive_path, flags);
			strrchr(recursive_path, '/')[1] = '\0';
		}
		closedir(directory);
		if (rmdir(path) == -1)
			log_errno(path);
		else if (flags & FLAG_VERBOSE)
			printf("removed directory '%s'\n", path);
	}
	else if (S_ISDIR(statbuf.st_mode) && flags & FLAG_DIRECTORY)
	{
		if (rmdir(path) == -1)
			log_errno(path);
		else if (flags & FLAG_VERBOSE)
			printf("removed directory '%s'\n", path);
	}
	else
	{
		if (unlink(path) == -1)
			log_errno(path);
		else if (flags & FLAG_VERBOSE)
			printf("removed '%s'\n", path);
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		fprintf(stderr, "%s: missing operand\n", argv[0]);
		return EXIT_FAILURE;
	}
	g_name = argv[0];

	int		option;
	t_flags	flags = 0;

	while ((option = getopt(argc, argv, "firRdv")) != -1)
	{
		switch (option)
		{
			case 'f': flags |= FLAG_FORCE;            break;
			case 'i': flags |= FLAG_INTERACTIVE;      break;
			/* case 'I': flags |= FLAG_INTERACTIVE_ONCE; break; */
			case 'd': flags |= FLAG_DIRECTORY;        break;
			case 'v': flags |= FLAG_VERBOSE;          break;

			case 'r':
			case 'R':
				flags |=  FLAG_RECURSIVE; break;
				break;
		}
	}

	for (; optind < argc; optind++)
		rm_file(argv[optind], flags);
	return EXIT_SUCCESS;
}
