
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "object-pool.h"
#include "logging.h"

int __object_pool_resize(struct object_pool *p)
{
	void **items;
	 int new_cap;

	new_cap = p->capacity + p->capdelta;

	items = realloc(p->items, new_cap * sizeof(void*));
	if (NULL == items) {
		log_sys_msg("%s(%s, %d) - realloc() failed: %s\n", __func__, p->name, new_cap, strerror(errno));
		return 1;
	}

	p->capacity = new_cap;
	p->items = items;
	return 0;
}

int object_pool_expand(struct object_pool *p)
{
	int err = __object_pool_resize(p);
	if (0 == err)
		log_dbg_msg("%s('%s') - expanded to %d slots\n", __func__, p->name, p->capacity);
	return err;
}

int object_pool_shrink(struct object_pool *p)
{
	int err;
	p->capdelta = 0 - p->capdelta;
	err = __object_pool_resize(p);
	if (0 == err)
		log_dbg_msg("%s('%s') - shrank to %d slots\n", __func__, p->name, p->capacity);
	p->capdelta = 0 - p->capdelta;
	return err;
}

int object_pool_init(struct object_pool *p)
{
	u32 cap = p->capacity ? p->capacity : OP_DFLT_CAPACITY;
	int inc = p->capdelta ? p->capdelta : OP_DFLT_CAPDELTA;
	int err;

	p->capacity = cap;
	p->capdelta = 0;
	p->items = NULL;

	err = __object_pool_resize(p);
	if (err) {
		log_err_msg("%s('%s') - __object_pool_resize() failed, capacity was %d\n", __func__, p->name, p->capacity);
		goto __failure;
	}

	/* success */
	p->capacity = cap;
	p->capdelta = inc;
	p->avail_min = p->capacity;
	return 0;

      __failure:
	if (p->items)
		free(p->items);
	return 1;
}

void *object_pool_get(struct object_pool *p)
{
	if (unlikely(pool_is_empty(p))) {
		log_dbg_msg("%s() - object pool '%s' is empty\n", __func__, p->name);
		return NULL;
	}

	p->avail--;

	if (unlikely(p->avail < p->avail_min)) {
		p->avail_min = p->avail;
		log_dbg_msg("%s('%s') - low water mark: %d\n", __func__, p->name, p->avail);
	}

	return p->items[p->avail];
}

int object_pool_put(struct object_pool *p, void *item)
{
	int err;

	if (unlikely(pool_is_full(p))) {
		err = object_pool_expand(p);
		if (err) {
			log_err_msg("%s('%s') - __object_pool_expand() failed, capacity was %d\n", __func__, p->name, p->capacity);
			return err;
		}
		log_dbg_msg("%s() - __object_pool_expand('%s') succeded\n", __func__, p->name);
	}

	p->items[p->avail] = item;
	p->avail++;
	return 0;
}
