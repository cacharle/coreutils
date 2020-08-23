#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

bool isodigit(char c)
{
	return c >= '0' && c < '8';
}

int main(int argc, char **argv)
{
	int  option;
	bool escape  = false;
	bool newline = true;

	while ((option = getopt(argc, argv, "neE")) != -1)
	{
		switch (option)
		{
			case 'e': escape  = true;  break;
			case 'E': escape  = false; break;
			case 'n': newline = false; break;
		}
	}
	for (; optind < argc; optind++)
	{
		char *str = argv[optind];
		if (escape)
		{
			for (size_t i = 0; str[i] != '\0'; i++)
			{
				if (str[i] == '\\')
				{
					uint8_t c     = 0;
					int     shift = 1;

					switch (str[i + 1])
					{
						case '\\': c = '\\'; break;
						case 'a': c = '\a'; break;
						case 'b': c = '\b'; break;
						case 'e': c = '\033'; break;
						case 'f': c = '\f'; break;
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case 'v': c = '\v'; break;

						case '0':
							c = 0;
							for (int j = 0; isodigit(str[i + 2 + j]) && j < 3; j++)
							{
								c *= 8;
								c += str[i + 2 + j] - '0';
								shift++;
							}
							break;

						case 'x':
							c = 0;
							for (int j = 0; isxdigit(str[i + 2 + j]) && j < 2; j++)
							{
								c *= 16;
								if (islower(str[i + 2 + j]))
									c += 10 + str[i + 2 + j] - 'a';
								else if (isupper(str[i + 2 + j]))
									c += 10 + str[i + 2 + j] - 'A';
								else
									c += str[i + 2 + j] - '0';
								shift++;
							}
							break;

						case 'c':
							str[i] = '\0';
							fputs(str, stdout);
							fflush(stdout);
							exit(EXIT_SUCCESS);
							break;

						default:
							shift = -1;
							break;
					}
					if (shift != -1)
					{
						str[i] = c;
						memmove(&str[i + 1], &str[i + 1 + shift], strlen(&str[i + 1 + shift]) + 1);
					}
				}
			}
		}
		fputs(str, stdout);
		if (optind + 1 != argc)
			putchar(' ');
	}
	if (newline)
		putchar('\n');
	return EXIT_SUCCESS;
}
