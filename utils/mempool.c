
/*
 * Author: Eugene Zhiganov, dee0xeed@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mempool.h"
#include "logging.h"

void __mp__print_free_list(struct mempool* mp)
{
	int k = 0;
	struct block_header *bh;

	bh = mp->head;
	while (bh) {
		log_dbg_msg("%s() - block #%d is at %p\n", __func__, k, bh);
		k++;
		bh = bh->next;
	}
}

void __mp__init_blocks(u8 *addr, int block_size, int blocks)
{
	struct block_header *bh;
	u8 *a;

	for (a = addr; --blocks; a += block_size) {
		bh = (struct block_header*)a;
		bh->next = (struct block_header*)(a + block_size);
	}
	bh = (struct block_header*)a;
	bh->next = NULL;
}

int __mp__new_page(struct mempool *mp)
{
	u8 *addr;

	mp->npages++;
	addr = realloc(mp->pages, mp->npages * sizeof(u8*));
	if (unlikely(NULL == addr)) {
		log_sys_msg("%s('%s') - realloc(npages %d) failed\n", __func__, mp->name, mp->npages);
		mp->npages--;
		return 1;
	}
	//log_msg("INF: realloc(npages %d) in _new_page('%s') succeded, head %p\n", mp->npages, mp->name, mp->head);

	mp->pages = (u8**)addr;

	addr = malloc(MEMPOOL_BLOCKS_PER_PAGE * mp->block_size);
	if (unlikely(NULL == addr)) {
		log_sys_msg("%s('%s'): malloc(mp->pages[%d]): %s\n", __func__, mp->name, mp->npages - 1, strerror(errno));
		return 1;
	}
	log_dbg_msg("%s('%s') - expanded (%d pages, %d blocks, %d bytes)\n", __func__,
		mp->name, mp->npages, mp->npages*MEMPOOL_BLOCKS_PER_PAGE,
		mp->npages*MEMPOOL_BLOCKS_PER_PAGE*mp->block_size);

	mp->pages[mp->npages - 1] = addr;
	mp->head = (struct block_header*)addr;
	__mp__init_blocks(addr, mp->block_size, MEMPOOL_BLOCKS_PER_PAGE);

	return 0;
}

struct mempool *new_mempool(char *name, int block_size)
{
	struct mempool *mp;
	int err;

	if (block_size < sizeof(struct block_header))
		block_size = sizeof(struct block_header);

	/* 8 byte align */
	block_size += 7;
	block_size &= 0xFFFFFFF8;
	log_dbg_msg("%s('%s') -  block size will be %d\n", __func__, name, block_size);

	mp = malloc(sizeof(struct mempool));
	if (unlikely(NULL == mp)) {
		log_sys_msg("%s('%s') - malloc(): %s\n", __func__, name, strerror(errno));
		return NULL;
	}

	mp->name = name;
	mp->block_size = block_size;

	mp->npages = 0;
	mp->pages = NULL;
	mp->head = NULL;

	err = __mp__new_page(mp);
	if (unlikely(err))
		return NULL;

	/* debug */
	//__mp__print_free_list(mp);

	return mp;
}

void del_mempool(struct mempool *mp)
{
	int k;

	if (!mp)
		return;
	if (!mp->pages)
		return;

	for (k = 0; k < mp->npages; k++)
		if (mp->pages[k])
			free(mp->pages[k]);
	free(mp->pages);
	free(mp);
}

void *mempool_get_block(struct mempool *mp)
{
	struct block_header *bh;
	int err;

	if (unlikely(NULL == mp->head)) {
		err = __mp__new_page(mp);
		if (err)
			return NULL;
	}

	/* exclude first block from the head of free block list */
	bh = mp->head;
	mp->head = bh->next;

	return bh;
}

void mempool_put_block(struct mempool *mp, void *addr)
{
	struct block_header *x;

	/* insert the block into the head of the free block list */
	x = mp->head;
	mp->head = (struct block_header*)addr;
	mp->head->next = x;
}
