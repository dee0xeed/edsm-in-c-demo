
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "queue.h"

#define NQ 3
queue **qa;

int main(void)
{
	int j, k, err;

	qa = malloc(NQ * sizeof(queue*));
	memset(qa, 0, NQ * sizeof(queue*));

	for (j = 0; j < NQ; j++) {

		for (k = 0; k < 3; k++) {
			char *s;
			s = malloc(32);
			sprintf(s, "queue-%d-node-%.3d", j, k);
			err = enqueue(&qa[j], s);
			if (err) {
				printf("%s() - enqueue() failed\n", __func__);
			} else {
				printf("'%s' enqueued\n", s);
			}
		}
	}

	for (j = 0; j < NQ; j++) {

		for (;;) {
			char *s;
			s = dequeue(&qa[j]);
			if (NULL == s)
				break;
			printf("'%s' dequeued\n", s);
			free(s);
		}
	}

	return 0;
}
