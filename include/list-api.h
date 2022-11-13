
/*
 * Author: Eugene Zhiganov, dee0xeed@gmail.com
 */

#ifndef LIST_API_H
#define LIST_API_H

#include "numeric-types.h"
#include "api.h"

/* dpa == 'data pointer address' */
typedef void (*list_visit)(void **dpa);
typedef void list_head;

api_declaration(list_api) {

	void **(*add_tail)(void **head);
	void **(*add_head)(void **head);

//	void  *(*get_tail)(void **head);
	void  *(*get_head)(void **head);

//	void   (*del)(char *key, void **root, pt_visit dtor);
	void   (*walk)(void **head, list_visit visit);
	void   (*walk_r)(void **head, list_visit visit);
//	void   (*nuke)(void **head, list_visit dtor);
};

#endif
