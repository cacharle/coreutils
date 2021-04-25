#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <grp.h>
#include <pwd.h>

/*
** TODO
** time formatting
** bug with random pid without optimization
** command line args
** process selection
*/

#define PROC_DIR "/proc"


struct s_field_info
{
	char       *specifier;
	char       *header;
	int        width;
	void       (*print_func)(struct s_field_info *info);
};
typedef struct s_field_info t_field_info;

// globals
#define TASK_COMM_LEN 32

static int  g_page_size  = -1;
static long g_clock_tick = -1;

static char g_file_path[PATH_MAX + 1];

static char g_comm[TASK_COMM_LEN];

static pid_t g_pid;
static pid_t g_ppid;
static pid_t g_pgrp;

static uid_t g_uid;
static gid_t g_gid;

static long g_nice;
static long g_priority;

static unsigned long long g_starttime;

static int g_tty_nr;

static unsigned long g_vsize;
static unsigned long g_rss;
static unsigned      g_flags;
static char          g_state;
static char          g_state;
static unsigned long g_wchan;

void print_func_args(t_field_info *info)
{
	snprintf(g_file_path, PATH_MAX, PROC_DIR"/%d/cmdline", g_pid);
	FILE *cmdline_file = fopen(g_file_path, "r");
	if (cmdline_file == NULL)
		return;
	int count = 0;
	int c = EOF;
	while ((c = fgetc(cmdline_file)) != EOF)
	{
		fputc(c, stdout);
		count++;
		if (count > info->width)
			break;
	}
	while (count < info->width)
		fputc(' ', stdout);
}

void print_func_comm(t_field_info *info)
{
	char *closing_parent = strrchr(g_comm, ')');
	if (closing_parent != NULL)
		*closing_parent = '\0';
	printf(
		"%*s",
		info->width,
		g_comm[0] == '(' ? (g_comm + 1) : g_comm
	);
}

void print_func_pid(t_field_info *info)  { printf("%*d", info->width, g_pid);  }  // bug without -O flag
void print_func_ppid(t_field_info *info) { printf("%*d", info->width, g_ppid); }
void print_func_pgrp(t_field_info *info) { printf("%*d", info->width, g_pgrp); }

void print_func_etime(t_field_info *info)
{
	printf("%*lld ", info->width, /*current -*/ g_starttime / g_clock_tick);
}

void print_func_time(t_field_info *info)
{
	printf("%*lld ", info->width, g_starttime / g_clock_tick);
}

void print_func_user(t_field_info *info)
{
	struct passwd *passwd_ptr = getpwuid(g_uid);
	if (passwd_ptr == NULL)
		return;
	printf("%*.*s", info->width, info->width, passwd_ptr->pw_name);
}

void print_func_group(t_field_info *info)
{
	struct group *group_ptr = getgrgid(g_gid);
	if (group_ptr == NULL)
		return;
	printf("%*.*s", info->width, info->width, group_ptr->gr_name);
}

void print_func_nice(t_field_info *info)     { printf("%*ld", info->width, g_nice);     }
void print_func_priority(t_field_info *info) { printf("%*ld", info->width, g_priority); }


bool find_tty_in_dir(char *dir_path, char dest[PATH_MAX + 1])
{
	DIR *dir = opendir(dir_path);
	if (dir == NULL)
		return false;
	struct dirent *entry;
	strcpy(dest, dir_path);
	strcat(dest, "/");
	char *dest_end = dest + strlen(dest);
	while ((entry = readdir(dir)) != NULL)
	{
		strcat(dest, entry->d_name);
		struct stat statbuf;
		if (lstat(dest, &statbuf) == -1)
			return false;
		if ((statbuf.st_mode & S_IFMT) == S_IFCHR &&
			 statbuf.st_rdev == (dev_t)g_tty_nr)
			return true;
		*dest_end = '\0';
	}
	return false;
}

void print_func_tty(t_field_info *info)
{
	if (g_tty_nr == 0)
	{
		printf("%*c", info->width, '?');
		return;
	}

	char tty_path[PATH_MAX + 1] = {0};
	bool res = find_tty_in_dir("/dev", tty_path) ||
		       find_tty_in_dir("/dev/pts", tty_path);
	if (!res)
	{
		printf("%*c", info->width, '?');
		return;
	}
	printf("%*s", info->width, tty_path + 5);
}


// sum of 1 if didn't exec and 4 if used super-user privileges
void print_func_flags(t_field_info *info) { printf("%*o",  info->width, 0); }
void print_func_state(t_field_info *info) { printf("%*c",   info->width, g_state); }
void print_func_wchan(t_field_info *info) { printf("%*lu",  info->width, g_wchan); }
void print_func_sz(t_field_info *info)   { printf("%*lu", info->width, g_vsize / g_page_size); }
void print_func_vsz(t_field_info *info)    { printf("%*lu", info->width, g_rss); }

void print_func_cpu(t_field_info *info) { printf("%*u", info->width, 2); }
void print_func_addr(t_field_info *info) { printf("%*c", info->width, '-'); }

static t_field_info field_infos[] =
{
	{"args",   "COMMAND", -30, &print_func_args},
	{"comm",   "COMMAND", -30, &print_func_comm},
	{"pid",    "PID",     7, &print_func_pid},
	{"ppid",   "PPID",    7, &print_func_ppid},
	{"pgid",   "PGID",    7, &print_func_pgrp},
	{"etime",  "ELAPSED", 10, &print_func_etime},
	{"time",   "TIME",    10, &print_func_time},

	{"group",  "GROUP",   -8, &print_func_group},
	{"rgroup", "RGROUP",  -8, &print_func_group},
	{"user",   "USER",    -8, &print_func_user},
	{"ruser",  "RUSER",   -8, &print_func_user},  // real user in /proc/[pid]/status file

	{"nice",   "NI",       3, &print_func_nice},
	{"pri",    "PRI",      3, &print_func_priority},
	{"tty",    "TTY",      -6, &print_func_tty},
	{"f",      "F",        1, &print_func_flags},
	{"s",      "S",        1, &print_func_state},
	{"vsz",    "VSZ",      9, &print_func_vsz},
	{"sz",     "SZ",       9, &print_func_sz},
	{"wchan",  "WCHAN",    -6, &print_func_wchan},

	{"c",      "C",        2, &print_func_cpu},
	{"addr",   "ADDR",     4, &print_func_addr},
};


t_field_info *field_info_from_specifier(const char *specifier)
{
	for (size_t i = 0; i < sizeof(field_infos) / sizeof(t_field_info); i++)
		if (strcmp(field_infos[i].specifier, specifier) == 0)
			return (&field_infos[i]);
	return (NULL);
}

static char g_full_format[]    = "user pid ppid c stime tty time args";
static char g_long_format[]    = "f s user pid ppid c pri nice addr sz wchan tty time comm";
static char g_default_format[] = "pid tty time comm";

#define MINOR_MASK 0b01111111111100000000000000000000
#define MINOR_MASK 0b01111111111100000000000000000000

static size_t         g_fields_size = 0;
static t_field_info **g_fields = NULL;

void fields_from_format(char *format)
{
	size_t i;
	for (i = 0; format[i] != '\0'; i++)
		if (isspace(format[i]) && isspace(format[i + 1]))
			exit(1);
	g_fields_size = 1;
	for (i = 0; format[i] != '\0'; i++)
		if (isspace(format[i]))
			g_fields_size++;
	g_fields = malloc(g_fields_size * sizeof(t_field_info*));
	if (g_fields == NULL)
		exit(1);
	i = 0;
	char *specifier = NULL;
	while ((specifier = strsep(&format, " ,")) != NULL)
	{
		g_fields[i] = field_info_from_specifier(specifier);
		if (g_fields[i] == NULL)
		{
			fprintf(stderr, "Error: %s: is not a valid specifier\n", specifier);
			exit(1);
		}
		i++;
	}
}

void print_fields_headers(void)
{
	for (size_t i = 0; i < g_fields_size; i++)
		printf("%*s ", g_fields[i]->width, g_fields[i]->header);
	fputc('\n', stdout);
}

/*
static
struct
{
	char *specifier;
	void *store;
}
g_stat_file_parsing_entries[] =
{
	{"%d",   &g_pid},
	{"%s",   &g_comm},
	{"%c",   &g_state},
	{"%d",   &g_ppid},
	{"%d",   &g_pgrp},
	{"%d",   NULL},  // session
	{"%d",   &g_tty_nr},
	{"%d",   NULL},  // tpgid
	{"%u",   &g_flags},
	{"%lu",  NULL},  // minflt
	{"%lu",  NULL},  // cminflt
	{"%lu",  NULL},  // majflt
	{"%lu",  NULL},  // cmajflt
	{"%lu",  NULL},  // utime
	{"%lu",  NULL},  // stime
	{"%ld",  NULL},  // cutime
	{"%ld",  NULL},  // cstime
	{"%ld",  NULL},  // priority
	{"%ld",  &g_nice},
	{"%ld",  NULL},  // num_threads
	{"%ld",  NULL},  // itrealvalue
	{"%llu", &g_starttime},
	{"%lu",  &g_vsize},
	{"%ld",  NULL},  // rss
	{"%lu",  NULL},  // rsslim
	{"%lu",  NULL},  // startcode
	{"%lu",  NULL},  // endcode
	{"%lu",  NULL},  // startstack
	{"%lu",  NULL},  // kstkesp
	{"%lu",  NULL},  // kstkeip
	{"%lu",  NULL},  // signal
	{"%lu",  NULL},  // blocked
	{"%lu",  NULL},  // sigignore
	{"%lu",  NULL},  // sigcatch
	{"%lu",  &g_wchan},
	{"%lu",  NULL},  // nswap
	{"%lu",  NULL},  // cnswap
	{"%d",   NULL},  // exit_signal
	{"%d",   NULL},  // processor
	{"%u",   NULL},  // rt_priority
	{"%llu", NULL},  // delayacct_blkio_ticks
	{"%lu",  NULL},  // guest_time
	{"%ld",  NULL},  // cguest_time
	{"%lu",  NULL},  // end_data
	{"%lu",  NULL},  // start_brk
	{"%lu",  NULL},  // arg_start
	{"%lu",  NULL},  // arg_end
	{"%lu",  NULL},  // env_start
	{"%lu",  NULL},  // env_end
	{"%d",   NULL},  // exit_cod
};
*/

void parse_stat_file(FILE *stat_file)
{
	fscanf(
		stat_file,
		"%*d %s %c %d %d %*d %d %*d %u %*lu %*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld "
		"%ld %*ld %*ld %llu %lu %ld %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu "
		"%lu %*lu %*lu %*d %*d %*u %*llu %*lu %*ld %*lu %*lu %*lu %*lu %*lu %*lu %*d",
		/* &g_pid, */
		g_comm,
		&g_state,
		&g_ppid,
		&g_pgrp,
		&g_tty_nr,
		&g_flags,
		&g_nice,
		&g_starttime,
		&g_vsize,
		&g_rss,
		&g_wchan
	);
}


int main(int argc, char **argv)
{
	g_page_size = getpagesize();
	g_clock_tick = sysconf(_SC_CLK_TCK);

	char format[] = "sz vsz comm";

	fields_from_format(g_long_format);
	print_fields_headers();

	DIR *proc_dir = opendir(PROC_DIR);
	if (proc_dir == NULL)
		exit(1);
	struct dirent *proc_dir_entry = NULL;
	while ((proc_dir_entry = readdir(proc_dir)) != NULL)
	{
		if (sscanf(proc_dir_entry->d_name, "%d", &g_pid) != 1)
			continue;
		snprintf(g_file_path, PATH_MAX, PROC_DIR"/%d/stat", g_pid);
		struct stat statbuf;
		if (stat(g_file_path, &statbuf) == -1)
			continue;
		g_uid = statbuf.st_uid;
		g_gid = statbuf.st_gid;
		FILE *stat_file = fopen(g_file_path, "r");
		if (stat_file == NULL)
			continue;
		parse_stat_file(stat_file);
		for (size_t i = 0; i < g_fields_size; i++)
		{
			(g_fields[i]->print_func)(g_fields[i]);
			fputc(' ', stdout);
		}
		fputc('\n', stdout);
	}
	closedir(proc_dir);
	return 0;
}
