
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "queue.h"

int main(void)
{
	queue *q = NULL;
	   int k, err;

	for (k = 0; k < 10; k++) {
		char *s;
		s = malloc(16);
		sprintf(s, "node-%.3d", k);
		err = enqueue(&q, s);
		if (err) {
			printf("%s() - enqueue() failed\n", __func__);
		} else {
			printf("'%s' enqueued\n", s);
		}
	}

	for (;;) {
		char *s;
		s = dequeue(&q);
		if (NULL == s)
			break;
		printf("'%s' dequeued\n", s);
		free(s);
	}

	return 0;
}
