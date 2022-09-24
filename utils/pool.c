
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "pool.h"
#include "logging.h"

struct pool *new_pool(char *name, u32 min_cap, u32 max_cap)
{
	struct pool *p = NULL;

	if ((!max_cap) || (!min_cap) || (max_cap < min_cap) || (max_cap > POOL_CAP_LIMIT)) {
		log_bug_msg("%s() - invalid pool capacities (min %u, max %u)\n", __func__, min_cap, max_cap);
		goto __failure;
	}

	p = malloc(sizeof(struct pool));
	if (unlikely(NULL == p)) {
		log_sys_msg("%s() - malloc(pool): %s\n", __func__, strerror(errno));
		goto __failure;
	}
	memset(p, 0, sizeof(struct pool));

	p->items = malloc(min_cap*sizeof(void*));
	if (unlikely(NULL == p->items)) {
		log_sys_msg("%s() - malloc(pool->items): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	p->addr = malloc(min_cap*sizeof(void*));
	if (unlikely(NULL == p->addr)) {
		log_sys_msg("%s() - malloc(pool->addr): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	p->name = malloc(strlen(name) + 1);
	if (unlikely(NULL == p->name)) {
		log_sys_msg("%s() - malloc(pool->name): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	/* success */
	strcpy(p->name, name);
	p->cap_max = max_cap;
	p->cap_now = min_cap;
	p->avail_min = POOL_CAP_LIMIT;
	return p;

      __failure:
	if (p) {
		if (p->name)
			free(p->name);
		if (p->items)
			free(p->items);
		if (p->addr)
			free(p->addr);
		free(p);
	}
	return NULL;
}

int expand_pool(struct pool *p, u32 incr)
{
	void **items, **addr;
	int new_cap;

	if (!incr)
		return 1;
	if (p->cap_now == p->cap_max) {
		log_wrn_msg("%s() - '%s' has reached max capacity (%d)\n", __func__, p->name, p->cap_max);
		return 1;
	}

	new_cap = p->cap_now + incr;
	if (new_cap > POOL_CAP_LIMIT)
		new_cap = POOL_CAP_LIMIT;

	items = realloc(p->items, new_cap*sizeof(void*));
	if (NULL == items) {
		log_sys_msg("%s() - expand_pool(%s, %d, items): %s\n", __func__, p->name, new_cap, strerror(errno));
		return 1;
	}

	addr = realloc(p->addr, new_cap*sizeof(void*));
	if (NULL == addr) {
		log_sys_msg("%s() - expand_pool(%s, %d, addr): %s\n", __func__, p->name, new_cap, strerror(errno));
		return 1;
	}

	p->cap_now = new_cap;
	p->items = items;
	p->addr = addr;
	log_inf_msg("%s() - '%s' has %d slots now\n", __func__, p->name, new_cap);
	return 0;
}

/* register an object as a member of a pool */
int pool_new_obj(struct pool *p, void *obj)
{
	u32 *nobj_ptr;

	if (p->nobj == p->cap_now) {
		log_err_msg("%s() - number of objects in '%s' reached it's capacity\n", __func__, p->name);
		return 1;
	}

	p->addr[p->nobj] = obj;
	nobj_ptr = (u32*)obj;
	*nobj_ptr = p->nobj;
	p->nobj++;
	return 0;
}

void *pool_get(struct pool *p)
{
	if (unlikely(pool_is_empty(p))) {
		log_inf_msg("%s() - '%s' is empty\n", __func__, p->name);
		return NULL;
	}

	p->avail--;
	if (unlikely(p->avail < p->avail_min)) {
		p->avail_min = p->avail;
		log_inf_msg("%s() - '%s' low water mark: %d\n", __func__, p->name, p->avail);
	}

	return p->items[p->avail];
}

int pool_put(struct pool *p, void *item)
{
	u32 *nobj_ptr = (u32*)item;
	u32 nobj = *nobj_ptr;

	if (unlikely(nobj >= p->cap_now)) {
		log_bug_msg("%s() - application bug, invalid index (%d)\n", __func__, nobj);
		return 1;
	}

	/* object was not registered */
	if (unlikely(p->addr[nobj] != item)) {
		log_bug_msg("%s() - application bug, address mismatch (item %p, addr[%d] %p)\n", __func__, item, nobj, p->addr[nobj]);
		return 1;
	}

	/* should never happen... */
	if (unlikely(pool_is_full(p))) {
		log_ops_msg("%s() - '%s' is full\n", __func__, p->name);
		return 1;
	}

	p->items[p->avail] = item;
	p->avail++;
	if (p->avail_max < p->avail) {
		p->avail_max = p->avail;
		log_inf_msg("%s() - '%s' high water mark: %d\n", __func__, p->name, p->avail);
	}
	return 0;
}
