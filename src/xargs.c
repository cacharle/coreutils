#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>

static char   *g_eofstr    = NULL;
static char   *g_replstr   = NULL;
static size_t g_line_count = -1;
static size_t g_arg_count  = -1;
static bool   g_prompt     = false;
static size_t g_size       = -1;
static bool   g_trace      = false;
static bool   g_terminate  = false;

static char   *g_name      = "xargs";

void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s: ", g_name);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(1);
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL)
		die(strerror(errno));
	return ret;
}

char *xstrdup(const char *src)
{
	char *ret = strdup(src);
	if (ret == NULL)
		die(strerror(errno));
	return ret;
}

size_t parse_uint(const char *optarg)
{
	unsigned long n;
	char          *end;

	errno = 0;
	n = strtoul(optarg, &end, 10);
	if (!isdigit(*optarg) || errno != 0 || *end != '\0')
		die("invalid number \"%s\"", optarg);
	return n;
}

size_t args_len(char **args)
{
	size_t	len;
	for (len = 0; args[len] != NULL; len++);
	return len;
}

void args_free(char **args)
{
	for (size_t i = 0; args[i] != NULL; i++)
		free(args[i]);
	free(args);
}

char **args_cpy(char **dst, char **src)
{
	size_t i;

	for (i = 0; src[i] != NULL; i++)
		dst[i] = xstrdup(src[i]);
	dst[i] = NULL;
	return dst;
}

char **args_dup(char **src)
{
	char **ret = xmalloc((args_len(src) + 1) * sizeof(char*));
	return args_cpy(ret, src);
}

int main(int argc, char **argv)
{
	int option;

	while ((option = getopt(argc, argv, "E:I:L:n:ps:tx")) > 0)
	{
		switch (option)
		{
			case 'E': g_eofstr     = optarg;             break;
			case 'I': g_replstr    = optarg;             break;
			case 'L': g_line_count = parse_uint(optarg); break;
			case 'n': g_arg_count  = parse_uint(optarg); break;
			case 'p': g_prompt     = true;               break;
			case 's': g_size       = parse_uint(optarg); break;
			case 't': g_trace      = true;               break;
			case 'x': g_terminate  = true;               break;
		}
	}
	if (g_line_count == 0)
		die("value 0 for -L option should be >= 1");
	if (g_arg_count == 0)
		die("value 0 for -n option should be >= 1");
	if (g_prompt)
		g_trace = true;

	char *cmd = optind != argc ? argv[optind] : "echo";
	char **args_prefix;
	if (optind == argc || optind + 1 == argc)
	{
		args_prefix = xmalloc(2 * sizeof(char*));
		args_prefix[0] = cmd;
		args_prefix[1] = NULL;
	}
	else
	{
		size_t len = argc - optind;
		args_prefix = xmalloc((len + 1) * sizeof(char*));
		size_t i;
		for (i = 0; i < len; i++)
			args_prefix[i] = argv[optind + i];
		args_prefix[len] = NULL;
	}

	size_t len;
	bool eof = false;
	size_t command_count = 0;
	while (!eof)
	{
		char   **args = args_dup(args_prefix);
		char   *line = NULL;
		size_t line_size = 0;
		size_t count = 0;

		while (getline(&line, &line_size, stdin) > 0)
		{
			// removing newline
			char *last = line + strlen(line) - 1;
			if (*last == '\n')
				*last = '\0';
			// trim blank characters
			while (isblank(*line))
				memmove(line, line + 1, strlen(line));
			len = strlen(line);
			while (len > 0 && isblank(line[len - 1]))
			{
				line[len - 1] = '\0';
				len--;
			}
			// skipping blank lines
			if (*line == '\0')
				continue;
			// stopping if eof string encountered
			if (g_eofstr != NULL && strcmp(g_eofstr, line) == 0)
			{
				eof = true;
				break;
			}
			// add line to arguments
			len = args_len(args);
			char **new_args = xmalloc((len + 2) * sizeof(char*));
			args_cpy(new_args, args);
			new_args[len] = xstrdup(line);
			new_args[len + 2] = NULL;
			/* args_free(args); */
			args = new_args;
			count++;
			if ((g_line_count != (size_t)-1 && count >= g_line_count) ||
				(g_arg_count != (size_t)-1  && count >= g_arg_count))
				break;
		}
		if (feof(stdin))
			eof = true;
		if (eof && command_count != 0 && !g_prompt)
			break;
		command_count++;

		pid_t child_pid;
		child_pid = fork();
		switch (child_pid)
		{
			case -1:
				die(strerror(errno));
				break;
			case 0:
				if (g_trace)
					for (size_t i = 0; args[i] != NULL; i++)
					{
						fputs(args[i], stderr);
						if (args[i + 1] != NULL)
							fputc(' ', stderr);
					}
				bool confirmation = true;
				if (g_prompt)
				{
					FILE *tty_file = fopen("/dev/tty", "r");
					if (tty_file == NULL)
						die("Couldn't open tty file");
					fputs(" ?...", stderr);
					int c = fgetc(tty_file);
					confirmation = c == 'y' || c == 'Y';
				}
				else if (g_trace)
					fputc('\n', stderr);
				if (!confirmation)
					exit(0);
				execvp(cmd, args);
				die(strerror(errno));
		}
		waitpid(child_pid, &child_pid, 0);
		if (WEXITSTATUS(child_pid) == 255)
			break;
	}
	return 0;
}
