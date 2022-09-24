
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "list-api.h"
#include "macro-magic.h"

void print(void **dpa)
{
	char *s = *dpa;
	printf("'%s'\n", s);
}

void nothing(void **dpa)
{
}

int main(void)
{
	struct list_api *list;
	list_head *head = NULL;
	  void **dpa;		// data pointer address
	   int k;

	list = get_api("list_api");
	if (NULL == list) {
		printf("OPS: %s - get_api('list_api') failed\n", __func__);
		return 1;
	}

	for (k = 0; k < 10; k++) {

		char *s;
		s = malloc(16);
		sprintf(s, "node-%.3d", k);
		dpa = list->add_tail(&head);
		*dpa = s;
	}

	printf(" *** PASS 1 ***\n");
	list->walk(&head, print);

	printf(" *** PASS 2 ***\n");
	list->walk_r(&head, print);

//	for (k = 0; k < 10000000; k++)
//		list->walk(&head, nothing);

//	for (k = 0; k < 10000000; k++)
//		list->walk_r(&head, nothing);

	return 0;
}
