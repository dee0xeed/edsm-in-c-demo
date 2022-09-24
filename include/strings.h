
#ifndef STRINGS_H
#define STRINGS_H

#include "numeric-types.h"

struct strarr;
struct string_buffer;

/* counts the number of occurrences of char 'c' in string 's' */
   int strcnt(char *s, char c);
   int memcnt(u8 *m, int n, u8 b);

   int strxcnt(char *s, char *symbols);
  char *trim(char *trimee, char *junk);
  char *rtrim(char *trimee, char *junk);

struct strarr {
	 int n;
	char **s;
};

struct strarr *new_strarr(int n);
  void del_strarr(struct strarr *sa);

struct strarr *split(char *splitee, char separator);

struct string_buffer {
	char *loc;
	 int size;
};

#endif
