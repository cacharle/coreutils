#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/utsname.h>

enum name
{
	NAME_SYSNAME  = 1 << 0,
	NAME_NODENAME = 1 << 1,
	NAME_RELEASE  = 1 << 2,
	NAME_VERSION  = 1 << 3,
	NAME_MACHINE  = 1 << 4,
	NAME_ALL      = NAME_SYSNAME | NAME_NODENAME | NAME_RELEASE | NAME_VERSION | NAME_MACHINE,
};

static void print_name(char *name)
{
	static bool previous = false;
	if (previous)
		fputc(' ' ,stdout);
	else
		previous = true;
	fputs(name, stdout);
}

int main(int argc, char *argv[])
{
	enum name names = 0;
	int option;
	while ((option = getopt(argc, argv, "amnrsv")) != -1)
	{
		switch (option)
		{
			case 'a': names |= NAME_ALL; break;
			case 'm': names |= NAME_MACHINE; break;
			case 'n': names |= NAME_NODENAME; break;
			case 'r': names |= NAME_RELEASE; break;
			case 's': names |= NAME_SYSNAME; break;
			case 'v': names |= NAME_VERSION; break;
		}
	}
	if (optind != argc)
	{
		fprintf(stderr, "%s: extra operand '%s'\n", argv[0], argv[optind]);
		exit(1);
	}
	if (names == 0)
		names = NAME_SYSNAME;

	struct utsname names_buf;
	uname(&names_buf);

	if (names & NAME_SYSNAME)
		print_name(names_buf.sysname);
	if (names & NAME_NODENAME)
		print_name(names_buf.nodename);
	if (names & NAME_RELEASE)
		print_name(names_buf.release);
	if (names & NAME_VERSION)
		print_name(names_buf.version);
	if (names & NAME_MACHINE)
		print_name(names_buf.machine);
	fputc('\n', stdout);
	return 0;
}
