
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stack.h"

int main(void)
{
	stack *st = NULL;
	   int k, err;

	for (k = 0; k < 10; k++) {
		char *s;
		s = malloc(16);
		sprintf(s, "node-%.3d", k);
		err = push(&st, s);
		if (err) {
			printf("%s() - push() failed\n", __func__);
		} else {
			printf("'%s' pushed\n", s);
		}
	}

	for (;;) {
		char *s;
		s = pop(&st);
		if (NULL == s)
			break;
		printf("'%s' poped\n", s);
		free(s);
	}

	return 0;
}
