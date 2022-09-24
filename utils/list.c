
/*
 * DOTS-DAQ: Data Acquisition for Distant Objects Tracking System
 * Author: Eugene Zhiganov, dee0xeed@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "mempool-api.h"
#include "list.h"
#include "logging.h"

static struct mempool *__list_mempool = NULL;
static struct mempool_api *mp = NULL;

__attribute__((constructor)) static void list_init(void)
{
	mp = get_api("mpool_api");
	if (NULL == mp) {
		log_ops_msg("%s() - failed to get API for memory pool\n", __func__);
		exit(1);
	}
	__list_mempool = mp->new("list-nodes-mempool", sizeof(struct list_node));
	if (NULL == __list_mempool) {
		log_ftl_msg("%s() - failed to create memory pool\n", __func__);
		exit(1);
	}
}

struct list_node *__list_new_node(void **head)
{
	struct list_node *n;

	n = mp->get(__list_mempool);
	if (unlikely(NULL == n))
		return NULL;

	if (NULL == *head)
		*head = n;

	n->next = NULL;
	return n;
}

/*
 * add a node to the end of the list
 * returns address of node->data
 */
void **list__add_tail(void **head)
{
	struct list_node *n, *p;

	n = __list_new_node(head);
	if (unlikely(NULL == n))
		return NULL;

	if (n == *head)
		goto __ret;

	p = *head;
	while(p->next)
		p = p->next;
	p->next = n;

      __ret:
	return &n->data;
}
// https://www.geeksforgeeks.org/recursive-insertion-and-traversal-linked-list/

void **list__add_head(void **head)
{
	struct list_node *p, *n;

	n = __list_new_node(head);
	if (unlikely(NULL == n))
		return NULL;

	if (n == *head)
		goto __ret;

	p = *head;
	*head = n;
	n->next = p;

      __ret:
	return &n->data;
}

void *list__get_head(void **head)
{
	struct list_node *p;
	  void *dp;

	if (NULL == *head)
		return NULL;

	p = *head;
	dp = p->data;
	*head = p->next;

	mp->put(__list_mempool, p);
	return dp;
}

void list__walk(void **head, list_visit v)
{
	struct list_node *n;

	for (n = *head; n != NULL; n = n->next)
		v(&n->data);
}

void list__walk_r(void **head, list_visit v)
{
	struct list_node *n = *head;
	if (NULL == n)
		return;
//	v(n->data);		// traverse from head to tail
	list__walk_r((void**)&n->next, v);
	v(&n->data);		// traverse from tail to head
}

/*
void clr_list(struct list *list)
{
	struct memory *mem;
	struct mempool *mp;
	struct list_item *p, *q;

	if (!list)
		return;

	mem = &list->mem;
	mp = list->mp;

	p = list->head;
	while (p) {
		q = p->next;
		mem->put(mp, p->data);
		mem->put(mp, p);
		p = q;
	};

	list->head = NULL;
	list->tail = NULL;
	list->items = 0;

	return;
}

void del_list(struct list *list)
{
	struct memory *mem;
	struct mempool *mp;

	if (!list)
		return;

	mem = &list->mem;
	mp = list->mp;
	clr_list(list);
	mem->put(mp, list);

	return;
}
*/

/*
void *list_first(struct list *list)
{
	if (!list)
		return NULL;
	if (!list->head)
		return NULL;
	return list->head->data;
}

void list_del_data(struct list *list, void *ptr)
{
	struct memory *mem = &list->mem;
	struct mempool *mp = list->mp;

	mem->put(mp, ptr);
}

void *list_find(struct list *list, void *sample, int (*match)(void*, void*))
{
	struct list_item *p;
	int m = 0;

	p = list->head;
	while (p) {
		m = match(sample, p->data);
		if (m)
			break;
		p = p->next;
	}
	return (m)?(p->data):NULL;
}

int list_cut(struct list *list, void *sample, int (*match)(void*, void*))
{
	struct memory *mem = &list->mem;
	struct mempool *mp = list->mp;
	struct list_item *curr, *prev;
	int m = 0;

	curr = list->head;
	prev = NULL;

	while (curr) {
		m = match(sample, curr->data);
		if (m)
			break;
		prev = curr;
		curr = curr->next;
	}

	if (!m)
		return 1; // error, not found

	if (!prev) { // head 
		list->head = curr->next;
		goto __free_mem;
	}

	if (!curr->next) { // tail
		prev->next = NULL;
		list->tail = prev;
		goto __free_mem;
	}

	/// somewhere in the middle
	prev->next = curr->next;

      __free_mem:
	mem->put(mp, curr->data);
	mem->put(mp, curr);
	list->items--;
	return 0;
}
*/
