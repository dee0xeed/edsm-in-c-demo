
#include <stdlib.h>

#include "list-api.h"
#include "logging.h"

//#define stack_api list_api
static struct list_api *st = NULL;

__attribute__((constructor)) static void stack_init(void)
{
	st = get_api("list_api");
	if (NULL == st) {
		log_ops_msg("%s() - get_api('list_api') failed\n", __func__);
		exit(1);
	}
}

/* returns 0 on success, 1 otherwise */
int push(void **head, void *dp)
{
	void **dpa;
	dpa = st->add_head(head);
	if (dpa)
		*dpa  = dp;
	return dpa ? 0 : 1;
}

void *pop(void **head)
{
	return st->get_head(head);
}
