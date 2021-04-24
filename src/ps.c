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

#define PROC_DIR "/proc"

#define TASK_COMM_LEN 32

static int page_size   = -1;
static long clock_tick = -1;

typedef enum
{
	FIELD_ARGS,
	FIELD_COMM,
	FIELD_PID,
	FIELD_PPID,
	FIELD_PGID,
	FIELD_ETIME,
	FIELD_TIME,
	FIELD_GROUP,
	FIELD_RGROUP,
	FIELD_USER,
	FIELD_RUSER,
	FIELD_NICE,
	FIELD_TTY,
	FIELD_PCPU,
	FIELD_VSZ,
	FIELD_ADDR,
	FIELD_ADDR,
	FIELD_C,
	FIELD_PRI,
	FIELD_NI,
	FIELD_ADDR,
	FIELD_SZ,
	FIELD_WCHAN,
} t_field_id;

typedef struct
{
	t_field_id id;
	char       *specifier;
	char       *header;
	int        width;
} t_field_info;

static t_field_info field_infos[] = {
	{FIELD_ARGS,   "args",   "COMMAND", 30},
	{FIELD_COMM,   "comm",   "COMMAND", -1},
	{FIELD_PID,    "pid",    "PID",     8},
	{FIELD_PPID,   "ppid",   "PPID",    8},
	{FIELD_PGID,   "pgid",   "PGID",    8},
	{FIELD_ETIME,  "etime",  "ELAPSED", -1},
	{FIELD_TIME,   "time",   "TIME",    -1},
	{FIELD_GROUP,  "group",  "GROUP",   -1},
	{FIELD_RGROUP, "rgroup", "RGROUP",  -1},
	{FIELD_USER,   "user",   "USER",    -1},
	{FIELD_RUSER,  "ruser",  "RUSER",   -1},
	{FIELD_NICE,   "nice",   "NI",      -1},
	{FIELD_TTY,    "tty",    "TT",      -1},
	{FIELD_PCPU,   "pcpu",   "%CPU",    -1},
	{FIELD_VSZ,    "vsz",    "VSZ",     -1},
	{FIELD_FLAGS,  "f",      "F",       1},
	{FIELD_STATE,  "s",      "S",       1},
	{FIELD_C,      "c",      "C",       -1},
	{FIELD_PRI,    "pri",    "PRI",     -1},
	{FIELD_ADDR,   "addr",   "ADDR",    -1},
	{FIELD_SZ,     "sz",     "SZ",      -1},
	{FIELD_WCHAN,  "wchan",  "WCHAN",   -1},
};


t_field_info *filed_info_from_specifier(const char *specifier)
{
	for (size_t i = 0; i < sizeof(field_infos) / sizeof(t_field_info); i++)
		if (strcmp(field_infos[i].specifier, specifier) == 0)
			return (&field_infos[i]);
	return (NULL);
}

static struct stat proc_statbuf;
static char        *stat_file;
static char        *cmdfile_file;

void print_field_resource(t_field_info *field_info)
{
	switch (field_info->id)
	{
		case FIELD_ARGS:
			snprintf(file_path, PATH_MAX, "%s/%d/cmdline", PROC_DIR, pid);
			FILE *cmdline_file = fopen(file_path, "r");
			if (cmdline_file == NULL)
				return;
			int c = EOF;
			while ((c = fgetc(cmdline_file)) != EOF)
				fputc(c, stdout);
			break;

		case FIELD_COMM:
			char *closing_parent = strrchr(proc_stat.comm + 0, ')');
			if (closing_parent != NULL)
				*closing_parent = '\0';
			printf("%s", proc_stat.comm + 1);
			break;

		case FIELD_PID:  printf("%*ld ", info.width, proc_stat.pid);  break;
		case FIELD_PPID: printf("%*ld ", info.width, proc_stat.ppid); break;
		case FIELD_PGID: printf("%*ld ", info.width, proc_stat.pgrp); break;

		case FIELD_ETIME:
			printf("%6lld ", /*current -*/ proc_stat.starttime / clock_tick);
			break;
		case FIELD_TIME:
			printf("%6lld ", proc_stat.starttime / clock_tick);
			break;

		case FIELD_GROUP:
			break;
		case FIELD_RGROUP:
			break;
		case FIELD_USER:
			printf("%6d ", proc_statbuf.st_uid);
			break;
		case FIELD_RUSER:
			break;

		case FIELD_NICE:
			printf("%3ld ", proc_stat.nice);
			break;
		case FIELD_PRI:
			printf("%3ld ", proc_stat.priority);
			break;

		case FIELD_TTY:
			printf("%6d ", proc_stat.tty_nr);
			break;
		case FIELD_PCPU:
			break;
		case FIELD_VSZ:
			printf("%5ld ", proc_stat.vsize / page_size);
			break;
		case FIELD_ADDR:
			break;
		case FIELD_FLAGS:
			printf("%o ", 0); //proc_stat.flags,
			break;
		case FIELD_STATE:
			printf("%c ", proc_stat.state);
			break;

		case FIELD_C:
			break;
		case FIELD_SZ:
			break;

		case FIELD_WCHAN:
			printf("%4ld ", proc_stat.wchan);
			break;

		default:
			exit(1);
	}
}

char full_format[]    = "user pid ppid c stime tty time args";
char long_format[]    = "f s user pid ppid c pri ni addr sz wchan tty time cmd";
char default_format[] = "pid tty time cmd";

size_t         fields_size = 0;
t_field_info **field_infos = NULL;

void field_infos_from_format(char *format)
{
	size_t i;

	for (i = 0; format[i] != '\0'; i++)
		if (isspace(format[i]) && isspace(format[i + 1]))
			exit(1);

	fields_size = 0;
	for (i = 0; format[i] != '\0'; i++)
		if (isspace(format[i]))
			fields_size++;

	field_infos = malloc(fields_size * sizeof(t_field_info*));
	if (field_infos == NULL)
		exit(1);

	i = 0;
	char *specifier = NULL;
	while ((specifier = strsep(&format, " ,")) != NULL)
	{
		field_infos[i] = field_info_from_specifier(specifier);
		if (field_infos[i] == NULL)
			exit(1);
		i++;
	}
}

void print_header(void)
{
	for (size_t i = 0; i < fields_size; i++)
		printf("%*s ", field_infos[i].width, field_infos[i].header);
}

typedef struct
{
	pid_t              pid;
	char               comm[TASK_COMM_LEN];
	char               state;
	pid_t              ppid;
	pid_t              pgrp;
	int                session;
	int                tty_nr;
	int                tpgid;
	unsigned int       flags;
	unsigned long      minflt;
	unsigned long      cminflt;
	unsigned long      majflt;
	unsigned long      cmajflt;
	unsigned long      utime;
	unsigned long      stime;
	long               cutime;
	long               cstime;
	long               priority;
	long               nice;
	long               num_threads;
	long               itrealvalue;
	unsigned long long starttime;
	unsigned long      vsize;
	long               rss;
	unsigned long      rsslim;
	unsigned long      startcode;
	unsigned long      endcode;
	unsigned long      startstack;
	unsigned long      kstkesp;
	unsigned long      kstkeip;
	unsigned long      signal;
	unsigned long      blocked;
	unsigned long      sigignore;
	unsigned long      sigcatch;
	unsigned long      wchan;
	unsigned long      nswap;
	unsigned long      cnswap;
	int                exit_signal;
	int                processor;
	unsigned int       rt_priority;
	unsigned long      delayacct_blkio_ticks;
	unsigned long      guest_time;
	long               cguest_time;
	unsigned long      end_data;
	unsigned long      start_brk;
	unsigned long      arg_start;
	unsigned long      arg_end;
	unsigned long      env_start;
	unsigned long      env_end;
	int                exit_code;
} t_proc_stat;

int main(int argc, char **argv)
{
	field_infos_from_format(default_format);

	free(field_infos);
	return;


	page_size = getpagesize();
	clock_tick = sysconf(_SC_CLK_TCK);

	DIR *proc_dir = opendir(PROC_DIR);
	if (proc_dir == NULL)
		exit(1);
	struct dirent *proc_dir_entry = NULL;
	while ((proc_dir_entry = readdir(proc_dir)) != NULL)
	{
		pid_t pid = 0;
		if (sscanf(proc_dir_entry->d_name, "%d", &pid) != 1)
			continue;

		char file_path[PATH_MAX + 1] = {'\0'};
		snprintf(file_path, PATH_MAX, "%s/%d/stat", PROC_DIR, pid);
		FILE *stat_file = fopen(file_path, "r");
		if (stat_file == NULL)
			continue;

		t_proc_stat proc_stat;
		fscanf(
			stat_file,
			"%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld "
			"%llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d %d %u "
			"%llu %lu %ld %lu %lu %lu %lu %lu %lu %d",
			&proc_stat.pid,
			&proc_stat.comm,
			&proc_stat.state,
			&proc_stat.ppid,
			&proc_stat.pgrp,
			&proc_stat.session,
			&proc_stat.tty_nr,
			&proc_stat.tpgid,
			&proc_stat.flags,
			&proc_stat.minflt,
			&proc_stat.cminflt,
			&proc_stat.majflt,
			&proc_stat.cmajflt,
			&proc_stat.utime,
			&proc_stat.stime,
			&proc_stat.cutime,
			&proc_stat.cstime,
			&proc_stat.priority,
			&proc_stat.nice,
			&proc_stat.num_threads,
			&proc_stat.itrealvalue,
			&proc_stat.starttime,
			&proc_stat.vsize,
			&proc_stat.rss,
			&proc_stat.rsslim,
			&proc_stat.startcode,
			&proc_stat.endcode,
			&proc_stat.startstack,
			&proc_stat.kstkesp,
			&proc_stat.kstkeip,
			&proc_stat.signal,
			&proc_stat.blocked,
			&proc_stat.sigignore,
			&proc_stat.sigcatch,
			&proc_stat.wchan,
			&proc_stat.nswap,
			&proc_stat.cnswap,
			&proc_stat.exit_signal,
			&proc_stat.processor,
			&proc_stat.rt_priority,
			&proc_stat.delayacct_blkio_ticks,
			&proc_stat.guest_time,
			&proc_stat.cguest_time,
			&proc_stat.end_data,
			&proc_stat.start_brk,
			&proc_stat.arg_start,
			&proc_stat.arg_end,
			&proc_stat.env_start,
			&proc_stat.env_end,
			&proc_stat.exit_code
		);

		struct stat statbuf;
		if (stat(file_path, &statbuf) == -1)
			continue;

		fputc('\n', stdout);
		break;
	}

	return 0;
}
