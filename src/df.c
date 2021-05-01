#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/statvfs.h>
#include <math.h>
#include <limits.h>

static unsigned long g_output_block_size = 512;

typedef struct
{
	char name[PATH_MAX + 1];
	char mount[PATH_MAX + 1];
} t_fs_info;

static t_fs_info g_fs_info_dummy = { .name = "", .mount = "" };

static t_fs_info *g_fs_infos      = NULL;
static size_t     g_fs_infos_size = 0;

void put_file_fs_info(char *file_relpath, bool allow_empty)
{
	char file_path[PATH_MAX + 1] = {'\0'};
	realpath(file_relpath, file_path);

	struct statvfs statvfsbuf;
	if (statvfs(file_path, &statvfsbuf) == -1)
		exit(1);

	long block_size     = statvfsbuf.f_bsize;
	unsigned long total = statvfsbuf.f_blocks * statvfsbuf.f_frsize / g_output_block_size;
	long used           = total - statvfsbuf.f_bfree  * block_size / g_output_block_size;
	long available      = statvfsbuf.f_bavail * block_size / g_output_block_size;

	bool empty_fs = used + available == 0;
	if (empty_fs && !allow_empty)
		return;

	t_fs_info *fs_info = &g_fs_info_dummy;
	for (size_t i = 0; i < g_fs_infos_size; i++)
	{
		char *mount = g_fs_infos[i].mount;
		size_t mount_len = strlen(mount);
		if (strncmp(file_path, mount, mount_len) == 0 && mount_len > strlen(fs_info->mount))
			fs_info = &g_fs_infos[i];
	}

	printf("%-15s", fs_info->name);
	printf("%11ld",  total);
	printf("%9ld",   used);
	printf("%10ld",  available);
	if (!empty_fs)
		printf("%8.0f%%", ceil(((double)used / (double)(used + available)) * 100));
	else
		printf("%9c", '-');
	printf(" %s", fs_info->mount);
	fputc('\n', stdout);
}

int main(int argc, char **argv)
{
	int option;

	while ((option = getopt(argc, argv, "kPt")) != -1)
	{
		switch (option)
		{
			case 'k':
				g_output_block_size = 1024;
				break;
			case 'P':
			case 't':
				break;
		}
	}

	FILE *mounts_file = fopen("/etc/mtab", "r");
	if (mounts_file == NULL)
		exit(1);
	while (!feof(mounts_file))
	{
		g_fs_infos_size++;
		g_fs_infos = realloc(g_fs_infos, sizeof(t_fs_info) * g_fs_infos_size);
		if (g_fs_infos == NULL)
		{
			fclose(mounts_file);
			exit(1);
		}
		t_fs_info *last = &g_fs_infos[g_fs_infos_size - 1];
		fscanf(mounts_file, "%s %s", &last->name, &last->mount);
		int c;
		do
			c = fgetc(mounts_file);
		while (c != '\n' && c != EOF);
	}
	fclose(mounts_file);
	g_fs_infos_size--;

	printf("Filesystem     %4lu-blocks     Used Available Capacity Mounted on\n", g_output_block_size);

	if (optind == argc)
	{
		for (size_t i = 0; i < g_fs_infos_size; i++)
			put_file_fs_info(g_fs_infos[i].mount, false);
		return 0;
	}
	for (; optind < argc; optind++)
		put_file_fs_info(argv[optind], true);
	free(g_fs_infos);
	return 0;
}
