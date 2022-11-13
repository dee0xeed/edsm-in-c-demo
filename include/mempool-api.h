
/*
 * Author: Eugene Zhiganov, dee0xeed@gmail.com
 */

#ifndef _MEMPOOL_API_H
#define _MEMPOOL_API_H

#include "compiler.h"
#include "numeric-types.h"
#include "api.h"

struct mempool {
	  char *name;
	   int block_size;

	/* free block list head */
	struct block_header *head;
	    u8 **pages;
	   int npages;
};

api_declaration(mempool_api) {
	struct mempool *(*new)(char *name, int block_size);
		  void  (*del)(struct mempool *mp);
		  void *(*get)(struct mempool *mp);
		  void  (*put)(struct mempool *mp, void *ptr);
};

#endif
