
#ifndef _MEMPOOL_H
#define _MEMPOOL_H

#include "mempool-api.h"

struct block_header {
	struct block_header *next;
};

#define MEMPOOL_MIN_BLOCK_SIZE sizeof(struct block_header)
//#define MEMPOOL_BLOCKS_PER_PAGE 1024
#define MEMPOOL_BLOCKS_PER_PAGE 512

struct mempool *new_mempool(char *name, int block_size);
  void del_mempool(struct mempool *mp);

  void *mempool_get_block(struct mempool *mp);
  void mempool_put_block(struct mempool *mp, void *addr);

api_definition(mempool_api, mpool_api) {
	.new = new_mempool,
	.del = del_mempool,
	.get = mempool_get_block,
	.put = mempool_put_block,
};

#endif
