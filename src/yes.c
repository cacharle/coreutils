#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
	while (true)
	{
		if (argc == 1)
			fputs("y", stdout);
		else
			for (int i = 1; i < argc; i++)
			{
				fputs(argv[i], stdout);
				if (i < argc - 1)
					putchar(' ');
			}
		putchar('\n');
	}
	return EXIT_SUCCESS;
}
