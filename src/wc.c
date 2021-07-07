#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <string.h>
#include <locale.h>

enum active_counters
{
	COUNTER_BYTES = 1 << 0,
	COUNTER_CHARS = 1 << 1,
	COUNTER_LINES = 1 << 2,
	COUNTER_WORDS = 1 << 3,
};

static enum active_counters active_counters = 0;

struct counters
{
	size_t bytes;
	size_t chars;
	size_t lines;
	size_t words;
};

#define BUF_SIZE 4096

static void count_file(FILE *file, struct counters *counters)
{
	wchar_t buf[BUF_SIZE + 1] = {0};

	if (active_counters == COUNTER_BYTES)
	{
		fseek(file, 0, SEEK_END);
		counters->bytes = ftell(file);
		return;
	}
	while (fgetws(buf, BUF_SIZE, file) != NULL)
	{
		if (active_counters & COUNTER_CHARS)
			counters->chars += wcslen(buf);
		if (active_counters & COUNTER_LINES)
		{
			for (size_t i = 0; buf[i] != L'\0'; i++)
			{
				if (buf[i] == L'\n')
					counters->lines++;
			}
		}
		if (active_counters & COUNTER_WORDS)
		{
			for (size_t i = 0; i < BUF_SIZE && buf[i] != L'\0'; i++)
			{
				while (i < BUF_SIZE && iswspace(buf[i]))
					i++;
				if (i < BUF_SIZE && !iswspace(buf[i]) && buf[i] != L'\0')
					counters->words++;
				while (i < BUF_SIZE  && !iswspace(buf[i]))
					i++;
			}
		}
		wmemset(buf, L'\0', BUF_SIZE + 1);  // fgetws doesn't empty buf if emptyline
	}
	if (active_counters & COUNTER_BYTES)
	{
		fseek(file, 0, SEEK_END);
		counters->bytes = ftell(file);
		// FIXME does not work with stdin
		// use fgetwc and mblen, fgetws uses fgetwc anyway so no performance improvement
		// stream are cached, I assume no read(x, y, 1) occurs
	}
}

static void print_counters(struct counters *counters, char *name)
{
	bool previous = false;
	if (active_counters & COUNTER_LINES)
	{
		printf("%s%zu", previous ? " " : "", counters->lines);
		previous = true;
	}
	if (active_counters & COUNTER_WORDS)
	{
		printf("%s%zu", previous ? " " : "", counters->words);
		previous = true;
	}
	if (active_counters & COUNTER_CHARS)
	{
		printf("%s%zu", previous ? " " : "",  counters->chars);
		previous = true;
	}
	if (active_counters & COUNTER_BYTES)
	{
		printf("%s%zu", previous ? " " : "",  counters->bytes);
		previous = true;
	}
	fputc(' ', stdout);
	fputs(name, stdout);
	fputc('\n', stdout);
}

static void count_filepath(char *filepath, struct counters *total_counters)
{
	FILE *file;
	if (strcmp(filepath, "-") == 0)
		file = stdin;
	else
	{
		file = fopen(filepath, "r");
		if (file == NULL)
		{
			fprintf(stderr, "wc: %s: %s\n", filepath, strerror(errno));
			return;
		}
	}
	struct counters counters = {0};
	count_file(file, &counters);
	print_counters(&counters, filepath);
	total_counters->lines += counters.lines;
	total_counters->words += counters.words;
	total_counters->chars += counters.chars;
	total_counters->bytes += counters.bytes;
	if (strcmp(filepath, "-") != 0)
		fclose(file);
}

int main(int argc, char *argv[])
{
	// needed for fgetws to work properly with utf-8
	// when value == "" set locale according to environment variable
	// with glibc, we can do fopen(x, "r,ccs=utf-8") but it's an extension
	if (setlocale(LC_ALL, "") == NULL)
		exit(1);
	int option;
	while ((option = getopt(argc, argv, "clmw")) != -1)
	{
		switch (option)
		{
			case 'c': active_counters |= COUNTER_BYTES; break;
			case 'l': active_counters |= COUNTER_LINES; break;
			case 'm': active_counters |= COUNTER_CHARS; break;
			case 'w': active_counters |= COUNTER_WORDS; break;
		}
	}
	if (active_counters == 0)
		active_counters = COUNTER_BYTES | COUNTER_LINES | COUNTER_WORDS;
	struct counters total_counters = {0};
	if (argc - optind == 0)
		count_filepath("-", &total_counters);
	for (int i = optind; i < argc; i++)
		count_filepath(argv[i], &total_counters);
	if (argc - optind > 1)
		print_counters(&total_counters, "total");
	return 0;
}
