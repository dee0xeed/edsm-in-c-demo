
#ifndef OBJECT_POOL_API_H
#define OBJECT_POOL_API_H

#include "numeric-types.h"
#include "compiler.h"
#include "api.h"

struct object_pool {

	char *name;
	 u32 capacity;
	 int capdelta;

	void **items;
	 u32 avail;		/* number of items in the pool */
	 u32 avail_min;		/* all-time low '.avail' (low water mark) */
};

api_declaration(object_pool_api) {
	 int  (*init)(struct object_pool *p);
	 int  (*expand)(struct object_pool *p);
	 int  (*shrink)(struct object_pool *p);
	 int  (*put)(struct object_pool *p, void *o);
	void *(*get)(struct object_pool *p);
};

#endif
