
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "list-api.h"
#include "macro-magic.h"

void print(void **dpa)
{
	long long int p = (long long int)*dpa;
	printf("VAL: %lld\n", p);
}

int main(void)
{
	struct list_api *list;
	  void *head = NULL;
	  void **ref;		// data ptr address
	   int k;

	list = get_api("list_api");
	if (NULL == list) {
		printf("OPS: %s - get_api('list_api') failed\n", __func__);
		return 1;
	}

	for (k = 0; k < 10; k++) {

		printf("Adding node #%d...", k);
		ref = list->add_tail(&head);
		if (NULL == ref) {
			printf("FAILED\n");
		} else {
			printf("OK, ref = %p\n", ref);
			*ref = (void*)(long long unsigned int)k;
		}
	}

	ref = list->add_head(&head);
	*ref = (void*)(long long unsigned int)123;

	printf(" *** PASS 1 ***\n");
	list->walk(&head, print);
	printf(" *** END OF PASS 1 ***\n");

	list->get_head(&head);
	list->get_head(&head);

	printf(" *** PASS 2 ***\n");
	list->walk(&head, print);
	printf(" *** END OF PASS 2 ***\n");

	for (k = 0; k < 12; k++) {
		void *p;
		p = list->get_head(&head);
		printf("%d - %p\n", k, p);
	}

	printf(" *** PASS 3 ***\n");
	list->walk(&head, print);
	printf(" *** END OF PASS 3 ***\n");

	for (k = 0; k < 7; k++) {

		printf("Adding node #%d...", k);
		ref = list->add_tail(&head);
		if (NULL == ref) {
			printf("FAILED\n");
		} else {
			printf("OK, ref = %p\n", ref);
			*ref = (void*)(long long unsigned int)k;
		}
	}

	printf(" *** PASS 4 ***\n");
	list->walk(&head, print);
	printf(" *** END OF PASS 4 ***\n");

	return 0;
}
