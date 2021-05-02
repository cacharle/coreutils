#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

static bool valid_char(int c)
{
	return isprint(c) || c == ' ' || c == '\t';
}

static size_t g_min_len = 4;
static char   g_offset_format = -1;
static char   *g_buf = NULL;

static void strings(FILE *file)
{
	size_t buf_offset = 0;
	int c;
	while ((c = fgetc(file)) != EOF)
	{
		if (valid_char(c))
		{
			if (buf_offset < g_min_len - 1)
			{
				g_buf[buf_offset] = c;
				buf_offset++;
			}
			else
			{
				switch (g_offset_format)
				{
					case 'd': printf("%7ld ", ftell(file) - g_min_len); break;
					case 'o': printf("%7lo ", ftell(file) - g_min_len); break;
					case 'x': printf("%7lx ", ftell(file) - g_min_len); break;
				}
				fwrite(g_buf, sizeof(char), buf_offset, stdout);
				buf_offset = 0;
				fputc(c, stdout);
				int c2;
				while ((c2 = fgetc(file)) != EOF && valid_char(c2))
					fputc(c2, stdout);
				fputc('\n', stdout);
			}
		}
		else
			buf_offset = 0;
	}
}

int main(int argc, char **argv)
{
	int option;
	while ((option = getopt(argc, argv, "an:t:")) != -1)
	{
		switch (option)
		{
			case 'a':
				break;
			case 'n':
				sscanf(optarg, "%lu", &g_min_len);
				break;
			case 't':
				g_offset_format = optarg[0];
				break;
		}
	}
	g_buf = malloc(sizeof(char) * g_min_len);
	if (g_buf == NULL)
		exit(1);
	if (optind == argc)
	{
		strings(stdin);
		free(g_buf);
		return 0;
	}
	for (; optind < argc; optind++)
	{
		FILE *file = fopen(argv[optind], "r");
		if (file == NULL)
			continue;
		strings(file);
		fclose(file);
	}
	free(g_buf);
	return 0;
}
