#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv)
{
	int	option;

	while ((option = getopt(argc, argv, "bfinS:t:Tuv")) != -1)
	{
		switch (option)
		{
			case 'b':
				break;
			case 'f':
				break;
			case 'i':
				break;
			case 'n':
				break;
			case 'S':
				break;
			case 't':
				break;
			case 'T':
				break;
			case 'u':
				break;
			case 'v':
				break;
		}
	}

	return EXIT_SUCCESS;
}
