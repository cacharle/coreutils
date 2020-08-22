#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

typedef enum
{
	RMODE_FOLLOW_ROOT,
	RMODE_FOLLOW_ALL,
	RMODE_NO_FOLLOW,
}	t_recursive_mode;

typedef enum
{
	VLEVEL_CHANGE,
	VLEVEL_QUIET,
	VLEVEL_VERBOSE,
}	t_verbose_level;

static t_verbose_level  g_verbosity      = VLEVEL_QUIET;
static bool             g_follow_link    = true;
static bool             g_recursive      = false;
static t_recursive_mode g_recursive_mode = RMODE_NO_FOLLOW;
static char             *g_name          = "chown";
static uid_t            g_user_id        = -1;
static gid_t            g_group_id       = -1;

void	verror_log(const char *format, va_list ap)
{
	fprintf(stderr, "%s: ", g_name);
	vfprintf(stderr, format, ap);
	fputc('\n', stderr);
}

void	error_log(const char *format, ...)
{
	va_list	ap;

	va_start(ap, format);
	verror_log(format, ap);
	va_end(ap);
}

void	fatal(const char *format, ...)
{
	va_list	ap;

	va_start(ap, format);
	verror_log(format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

bool parse_id(char *id_str, id_t *id_ptr, bool group)
{
	char *end;

	if (isdigit(*id_str))
	{
		errno = 0;
		long tmp = strtol(id_str, &end, 10);
		if (errno != 0 || tmp > UINT_MAX || *end != '\0')
			return false;
		*id_ptr = (id_t)tmp;
	}
	else
	{
		struct passwd *pw = getpwnam(id_str);
		if (pw == NULL)
			return false;
		if (group)
			*id_ptr = pw->pw_gid;
		else
			*id_ptr = pw->pw_uid;
	}
	return true;
}

bool file_chown(char *path, bool root)
{
	char		recursive_path[PATH_MAX + 1] = {'\0'};
	struct stat	statbuf;


	if (stat(path, &statbuf) == -1)
	{
		error_log("cannot access '%s': %s", path, strerror(errno));
		return false;
	}

	errno = 0;
	if ((!g_recursive && g_follow_link)
		|| (g_recursive && (g_recursive_mode == RMODE_FOLLOW_ALL
			|| (root && g_recursive_mode == RMODE_FOLLOW_ROOT))))
		chown(path, g_user_id, g_group_id);
	else
		lchown(path, g_user_id, g_group_id);
	if (errno != 0)
	{
		error_log("changing ownership of '%s': %s", path, strerror(errno));
		return false;
	}

	if (g_recursive)
	{
		errno = 0;
		DIR *directory = opendir(path);

		if (directory == NULL && errno == ENOTDIR)
			return true;
		if (directory == NULL)
		{
			error_log("cannot access '%s': %s", path, strerror(errno));
			return false;
		}

		strcpy(recursive_path, path);
		if (path[strlen(path) - 1] != '/')
			strcat(recursive_path, "/");

		struct dirent *entry;
		while ((entry = readdir(directory)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			strcat(recursive_path, entry->d_name);
			file_chown(recursive_path, false);
			strrchr(recursive_path, '/')[1] = '\0';
		}
		closedir(directory);
	}
	return true;
}

/*
**
** TODO
** don't change group/user if not asked
** change group to login group is trailling : after user in cmd
**
*/

int main(int argc, char **argv)
{
	int option;

	while ((option = getopt(argc, argv, "cfvhRHLP")) != -1)
	{
		switch (option)
		{
			case 'c': g_verbosity      = VLEVEL_CHANGE;     break;
			case 'f': g_verbosity      = VLEVEL_QUIET;      break;
			case 'v': g_verbosity      = VLEVEL_VERBOSE;    break;
			case 'h': g_follow_link    = false;             break;
			case 'R': g_recursive      = true;              break;
			case 'H': g_recursive_mode = RMODE_FOLLOW_ROOT; break;
			case 'L': g_recursive_mode = RMODE_FOLLOW_ALL;  break;
			case 'P': g_recursive_mode = RMODE_NO_FOLLOW;   break;
		}
	}

	if (argc - optind == 0)
		fatal("missing operand");
	if (argc - optind == 1)
		fatal("missing operand after '%s'", argv[optind]);

	char *user_str = argv[optind];
	char *group_str = strchr(argv[optind], ':');
	if (group_str != NULL)
	{
		*group_str = '\0';
		group_str++;
	}

	if (!parse_id(user_str, &g_user_id, false))
		fatal("invalid user '%s'", user_str);
	if (group_str != NULL && !parse_id(group_str, &g_group_id, true))
		fatal("invalid group '%s:%s'", user_str, group_str);

	for (optind++; optind < argc; optind++)
		file_chown(argv[optind], true);

	return EXIT_SUCCESS;
}
