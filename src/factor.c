#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>

int g_status_code = 0;

void print_factors(const char *s)
{
	char *endptr;
	errno = 0;
	unsigned long n = strtoul(s, &endptr, 10);
	if (errno != 0 || !isdigit(*s) || *endptr != '\0')
	{
		fprintf(stderr, "factor: '%s': is not a valid positive integer\n", s);
		g_status_code = 1;
		return;
	}
	printf("%lu:", n);
	while (n > 1)
	{
		for (unsigned long d = 2; true; d++)
		{
			if (n % d == 0)
			{
				printf(" %lu", d);
				n /= d;
				break;
			}
		}
	}
	fputc('\n', stdout);
}

int main(int argc, char **argv)
{
	if (argc == 1)
	{
		char *line = NULL;
		size_t n = 0;
		while (getline(&line, &n, stdin) != -1)
			print_factors(line);
		free(line);
		return g_status_code;
	}
	for (int i = 1; i < argc; i++)
		print_factors(argv[i]);
	return g_status_code;
}
