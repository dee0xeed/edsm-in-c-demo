
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "array.h"
#include "compiler.h"
#include "logging.h"

int __array__setcap(struct array *a, u32 capacity)
{
	 u32 new_size;
	void *d;

	if (unlikely((NULL == a) || (0 == capacity)))
		return 1;

	new_size = capacity * a->item_size;

	d = realloc(a->data, new_size);
	if (NULL == d) {
		log_sys_msg("%s() - realloc(%d): %s\n", __func__, new_size, strerror(errno));
		return 1;
	}

	log_dbg_msg("%s() - realloc() succeded, new_size = %d\n", __func__, new_size);
	a->data = d;
	a->capacity = capacity;
	return 0;
}

#define ARRAY_DEFAULT_CAPACITY 512
struct array *new__array(u32 item_size, u32 capacity)
{
	struct array *a = NULL;

	if (0 == item_size)
		return NULL;

	a = calloc(1, sizeof(struct array));
	if (NULL == a) {
		log_sys_msg("%s() - malloc(#1): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	if (0 == capacity) {
		capacity = ARRAY_DEFAULT_CAPACITY;
		log_inf_msg("%s() - array capacity set to %d\n", __func__, ARRAY_DEFAULT_CAPACITY);
	}

	a->data = calloc(capacity * item_size, 1);
	if (NULL == a->data) {
		log_sys_msg("%s() - malloc(#2): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	a->item_size = item_size;
	a->capacity = capacity;
	return a;

      __failure:
	del__array(a);
	return NULL;
}

void del__array(struct array *a)
{
	if (a) {
		if (a->data)
			free(a->data);
		free(a);
	}
}

void *array__add(struct array *a, void *data)
{
	int err;
	 u8 *p;

	if (unlikely((NULL == a) || (NULL == data)))
		return NULL;

	if (unlikely(a->nitems == a->capacity)) {

		u32 new_cap = 2 * a->capacity;
		err = __array__setcap(a, new_cap);
		if (err)
			return NULL;
	};

	p = a->data + a->item_size * a->nitems;
	memcpy(p, data, a->item_size);
	a->nitems++;
	return p;
}

void array__reset(struct array *a)
{
	if (unlikely(NULL == a))
		return;

	a->nitems = 0;
}

void array__walk(struct array *a, array_visit v)
{
	int k;
	u8 *p;

	if (unlikely((NULL == a) || (NULL == v)))
		return;

	for (k = 0; k < a->nitems; k++) {
		p = a->data + k * a->item_size;
		v(p);
	}
}

int array__setcap(struct array *a, u32 capacity)
{
	return __array__setcap(a, capacity);
}

u32 array__getcap(struct array *a)
{
	return a ? a->capacity : 0;
}
