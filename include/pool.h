
#ifndef POOL_H
#define POOL_H

#include "numeric-types.h"
#include "compiler.h"

#define POOL_CAP_LIMIT 0xFFFF

struct pool {

	char *name;

	 u32 cap_max;		/* maximal pool capacity */
	 u32 cap_now;		/* current pool capacity */

	 u32 nobj;		/* number of objects created for placing in the pool */
	void **addr;		/* index - nobj, value - address of object */

	void **items;
	 u32 avail;		/* number of items in the pool */
	 u32 avail_min;		/* all-time low '.avail' */
	 u32 avail_max;		/* all-time high '.avail' */
};

#define pool_is_empty(p)	!p->avail
#define pool_is_full(p)		p->avail==p->cap_now

struct pool *new_pool(char *name, u32 min_cap, u32 max_cap);
   int expand_pool(struct pool *p, u32 incr);

   int pool_new_obj(struct pool *p, void *obj);
  void *pool_get(struct pool *p);
   int pool_put(struct pool *p, void *item);

#endif
