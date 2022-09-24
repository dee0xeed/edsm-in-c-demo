
#include <stdlib.h>
#include <string.h>

#include "strings.h"

int strcnt(char *s, char c)
{
	int cnt = 0;
	while (*s) {
		if (*s == c)
			cnt++;
		s++;
	}
	return cnt;
}

int strxcnt(char *s, char *symbols)
{
	int cnt = 0;
	while (*s) {
		if (strchr(symbols, *s))
			cnt++;
		s++;
	}
	return cnt;
}

// http://stackoverflow.com/questions/656542/trim-a-string-in-c
// http://codereview.stackexchange.com/questions/20897/trim-function-in-c
// https://www.daniweb.com/software-development/c/code/216919/implementing-string-trimming-ltrim-and-rtrim-in-c

#if 0
char *trim(char *s)
{
	char *p;

	if (!s)
		return NULL;
	if (!*s)	/* empty string */
		return NULL;

	p = s + strlen(s) - 1;
	while ((' ' == *p) || ('\t' == *p))
		p--;
	p[1] = 0;

	while ((' ' == *s) || ('\t' == *s))
		s++;

	return s;
}
#endif

char *trim(char *trimee, char *junk)
{
	char *s;

	if (!trimee) 
		return trimee;
	if (!*trimee)
		return trimee;
	if (!junk)
		return trimee;
	if (!*junk)
		return trimee;

	/* trailing */
	s = trimee + strlen(trimee) - 1;
	while (strchr(junk, *s) && (s > trimee)) s--;
	s[1] = 0;

	/* leading */
	s = trimee;
	while (strchr(junk, *s) && *s) s++;

	return s;
}

struct strarr *new_strarr(int n)
{
	struct strarr *sa;
	sa = malloc(sizeof(struct strarr));
	if (!sa)
		return NULL;
	sa->n = n;
	sa->s = malloc(n*sizeof(char*));
	if (!sa->s) {
		free(sa);
		return NULL;
	}
	return sa;
}

void del_strarr(struct strarr *sa)
{
	if (sa) {
		if (sa->s)
			free(sa->s);
		free(sa);
	}
}

struct strarr *split(char *splitee, char separator)
{
	struct strarr *sa = NULL;
	int n, k, l;
	char *s;

	if (!splitee)
		return NULL;

	n = strcnt(splitee, separator);
	l = strlen(splitee);
	if (splitee[l-1] != separator)
		n++;

	sa = new_strarr(n);
	if (!sa)
		goto __failure;

	s = splitee;

	for (k = 0; k < n; k++) {

		sa->s[k] = s;

		s = strchr(s, separator);
		if (s) {
			*s = 0;
			s++;
		}
	}

	return sa;

      __failure:
	del_strarr(sa);
	return NULL;
}
