
#include <stdlib.h>

#include "list-api.h"
#include "logging.h"

#define queue_api list_api
static struct list_api *qi = NULL;

__attribute__((constructor)) static void queue_init(void)
{
	qi = get_api("list_api");
	if (NULL == qi) {
		log_ops_msg("%s() - get_api('list_api') failed\n", __func__);
		exit(1);
	}
}

/* returns 0 on success, 1 otherwise */
int enqueue(void **head, void *dp)
{
	void **dpa;
	dpa = qi->add_tail(head);
	if (dpa)
		*dpa  = dp;
	return dpa ? 0 : 1;
}

void *dequeue(void **head)
{
	return qi->get_head(head);
}
