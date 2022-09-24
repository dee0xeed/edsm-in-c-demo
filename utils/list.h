
#ifndef _LIST_H
#define _LIST_H

#include "list-api.h"

struct list_node {
	struct list_node *next;
	  void *data;
};

//  void del_list(struct list *list);
//  void clr_list(struct list *list);

void **list__add_tail(void **head);
void **list__add_head(void **head);
void  *list__get_head(void **head);

void   list__walk(void **head, list_visit v);
void   list__walk_r(void **head, list_visit v);
//  void list_del_data(struct list *list, void *ptr);
//  void list_foreach(struct list *list, void (*dothis)(void*, void*), void *xdata);
//  void *list_find(struct list *list, void *sample, int (*match)(void*, void*));
//   int list_cut(struct list *list, void *sample, int (*match)(void*, void*));

api_definition(list_api, list_api) {

	.add_tail = list__add_tail,
	.add_head = list__add_head,
	.get_head = list__get_head,
	.walk = list__walk,
	.walk_r = list__walk_r,
};

#endif
