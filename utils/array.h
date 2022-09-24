
#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "array-api.h"

struct array *new__array(u32 item_size, u32 capacity);
        void  del__array(struct array *a);
        void *array__add(struct array *a, void *data);
        void  array__reset(struct array *a);
        void  array__walk(struct array *a, array_visit v);
         int  array__setcap(struct array *a, u32 capacity);
         u32  array__getcap(struct array *a);

api_definition(array_api, array_api) {
	   .new = new__array,
	   .del = del__array,

	   .add = array__add,
	 .reset = array__reset,
	  .walk = array__walk,
	.setcap = array__setcap,
	.getcap = array__getcap,
};

#endif
