
/*
 * DOTS-DAQ: Data Acquisition for Distant Objects Tracking System
 * Author: Eugene Zhiganov, dee0xeed@gmail.com
 */

#ifndef DYNAMIC_ARRAY_API_H
#define DYNAMIC_ARRAY_API_H

#include "numeric-types.h"
#include "api.h"

struct array {
	void *data;
	 u32 item_size;
	 u32 nitems;		/* */
	 u32 capacity;		/* maximal number of items */
};

typedef struct array Array;

typedef void (*array_visit)(void *data);

api_declaration(array_api) {
	struct array *(*new)(u32 item_size, u32 capacity);
	  void  (*del)(struct array *a);
	  void *(*add)(struct array *a, void *data);
	  void  (*reset)(struct array *a);
	  void  (*walk)(struct array *a, array_visit v);
	   int  (*setcap)(struct array *a, u32 capacity);
	   u32  (*getcap)(struct array *a);
};

#endif
