#define _POSIX_C_SOURCE 2
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

typedef enum
{
	FLAG_COMPLEMENT = 1 << 0,
	FLAG_DELETE     = 1 << 1,
	FLAG_SQUEEZE    = 1 << 2,
}	t_flags;


typedef enum
{
	MTAG_CHAR,
	MTAG_CLASS,
	MTAG_RANGE,
	/* MTAG_REPEAT, */
	/* MTAG_EQUIVALENT, */ // e.g [=e=] == eéèê
}	t_matcher_tag;

typedef struct
{
	t_matcher_tag		tag;
	union
	{
		char		c;
		int			(*class)(int c);
		char		range[2];
		struct
		{
			char	c;
			size_t	count;
		}			repeat;
	};
}					t_matcher;

struct char_class_entry {
	char	*id;
	int		(*func)(int c);
};

struct char_class_entry char_classes[] = {
	{"[:alnum:]", isalnum},
	{"[:alpha:]", isalpha},
	{"[:blank:]", isblank},
	{"[:cntrl:]", iscntrl},
	{"[:digit:]", isdigit},
	{"[:graph:]", isgraph},
	{"[:lower:]", islower},
	{"[:print:]", isprint},
	{"[:punct:]", ispunct},
	{"[:space:]", isspace},
	{"[:upper:]", isupper},
	{"[:xdigit:]", isxdigit},
};

typedef struct
{
	t_matcher	*array;
	size_t		count;
}				t_matchers;

void	matchers_push(t_matchers *matchers, t_matcher_tag tag, void *data)
{
	t_matcher	*pushed;

	matchers->count++;
	matchers->array = realloc(matchers->array, matchers->count);
	assert(matchers->array != NULL);
	pushed = &matchers->array[matchers->count - 1];
	pushed->tag = tag;
	switch (tag)
	{
		case MTAG_CHAR:
			pushed->c = *(char*)data;
			break;
		case MTAG_CLASS:
			pushed->class = (int (*)(int))data;
			break;
		case MTAG_RANGE:
			pushed->range[0] = ((short)data & 0x00ff) >> 0;
			pushed->range[1] = ((short)data & 0xff00) >> 8;
			break;
		default:
			abort();
	}
}

bool isodigit(char c) { return c >= '0' && c <= '7'; }

void parse(t_matchers *matchers, char *s)
{
	for (size_t	i = 0; s[i] != '\0'; i++)
	{
		if (s[i] == '\\')
		{
			memmove(&s[i], &s[i + 1], strlen(&s[i + 1]));

			switch (s[i])
			{
				case 'a': s[i] = '\a'; break;
				case 'b': s[i] = '\b'; break;
				case 'f': s[i] = '\f'; break;
				case 'n': s[i] = '\n'; break;
				case 'r': s[i] = '\r'; break;
				case 't': s[i] = '\t'; break;
				case 'v': s[i] = '\v'; break;
			}

			if (isodigit(s[i]))
			{
				char num[4] = {'\0'};
				strncpy(num, &s[i], 3);
				char *end;
				s[i] = strtol(num, &end, 8);
				memmove(&s[i + 1], &s[i + 1 + (end - num)], strlen(&s[i + 1 + (end - num)]));
			}
			matchers_push(matchers, MTAG_CHAR, (void*)s[i]);
		}
		else if (s[i + 1] == '-' && s[i + 2] != '\0')
			matchers_push(matchers, MTAG_RANGE, (void*)(s[i] | (s[i] << 8)));
		else if (s[i] == '[' && s[i + 1] == ':')
		{
			/* for (size_t	j = 0; j < sizeof(char_classes) / sizeof(char_class_entry); j++) */
			/* { */
			/* 	if (strncmp( */
			/* } */
		}
		else
			matchers_push(matchers, MTAG_CHAR, (void*)s[i]);
	}
}

char	*g_name = "tr";

void	fatal(const char *format, ...)
{
	va_list	ap;

	va_start(ap, format);
	fprintf(stderr, "%s: ", g_name);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

#define BUFFER_SIZE 256

/*
** no 't' flag, truncate is the default behavior
*/

int main(int argc, char **argv)
{
	int		option;
	t_flags	flags = 0;

	while ((option = getopt(argc, argv, "cCds")) != -1)
	{
		switch (option)
		{
			case 'c':
			case 'C':
				flags |= FLAG_COMPLEMENT;
				break;
			case 'd':
				flags |= FLAG_DELETE;
				break;
			case 's':
				flags |= FLAG_SQUEEZE;
				break;
		}
	}
	if (optind == argc)
		fatal("missing operand\n");
	if (argc - optind == 3)
		fatal("extra operand '%s'\n", argv[argc - 1]);

	if (flags & FLAG_DELETE && flags & FLAG_SQUEEZE && argc - optind != 2)
		fatal("missing operand after '%s'\n"
			  "Two strings must be given when both deleting and squezzing repeats.\n",
			  argv[optind]);

	if (flags & FLAG_DELETE && argc - optind != 1)
		fatal("extra operand '%s'\n",
 		      "Only one string may be given when deleting without squeezing repeats.\n",
 			  argv[optind + 1]);

	t_matchers	set1 = { .array = NULL, .count = 0 };
	t_matchers	set2 = { .array = NULL, .count = 0 };

	char	buf[BUFFER_SIZE + 1] = {'\0'};
	size_t	read_size;

	while ((read_size = fread(buf, sizeof(char), BUFFER_SIZE, stdin)) > 0)
	{
		for (size_t i = 0; i < read_size; i++)
		{
			/* if (flags & FLAG_DELETE && flags & FLAG_SQUEEZE) */
			/* { */
            /*  */
			/* } */
			/* else if (flags & FLAG_DELETE && match()) */
			/* { */
			/* 	memmove(&buf[i], &buf[i + 1], read_size - i); */
            /*  */
			/* } */
			/* else if (flags & FLAG_SQUEEZE) */
			/* { */
			/* 	while (match(s[i + 1])) */
            /*  */
			/* 		memmove(&buf[i + 1], &buf[i + 2], read_size - i); */
			/* } */
			/* else */
			/* { */
				/* if (match()) */
				/* 	buf[i] = set2 */
			/* } */
		}
	}

	return EXIT_SUCCESS;
}
