
#include "logging.h"

int __mil_threshold = MIL_ERR;

void set_mil_threshold(int t)
{
	__mil_threshold = t;
}

#include <ctype.h>

void hexdump(u8 *b, int l)
{
	int k = 0;

	if (__mil_threshold > MIL_DBG)
		return;

	if ((0 == l) || (NULL == b))
		return;

	for (;;) {
		printf("%.2X", b[k]);

		k++;

		if (k % 16) {
			printf(" ");
		} else {
			int j;
			printf(" | ");
			for (j = k - 16; j < k; j++) {
				if (isprint(b[j])) {
					printf("%c", b[j]);
				} else {
					printf(".");
				}
			}
			printf("\n");
		}

		if (k >= l)
			break;
	}
	printf("\n");
}
