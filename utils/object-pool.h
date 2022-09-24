
#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include "object-pool-api.h"

 int  object_pool_init(struct object_pool *p);
 int  object_pool_expand(struct object_pool *p);
 int  object_pool_shrink(struct object_pool *p);
 int  object_pool_put(struct object_pool *p, void *object);
void *object_pool_get(struct object_pool *p);

api_definition(object_pool_api, object_pool_api) {
	    .init = object_pool_init,
	  .expand = object_pool_expand,
	  .shrink = object_pool_shrink,
	     .put = object_pool_put,
	     .get = object_pool_get,
};

/* functions for internal use */
int __object_pool_resize(struct object_pool *p);

#define pool_is_empty(p)	0 == (p)->avail
#define pool_is_full(p)		(p)->avail == (p)->capacity

#define OP_DFLT_CAPACITY		1024
#define OP_DFLT_CAPDELTA		 256

#endif
