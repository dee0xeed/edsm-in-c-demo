
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "message-buffer.h"
#include "logging.h"

static struct message_buffer *global_mb = NULL;

static void __del_mb(struct message_buffer *mb)
{
	if (mb) {
		if (mb->msg)
			free(mb->msg);
		free(mb);
	}
}

static struct message_buffer *__new_mb(u32 capacity)
{
	struct message_buffer *mb = NULL;
	   int n;

	n = sizeof(struct message_buffer);
	mb = malloc(n);
	if (NULL == mb) {
		log_sys_msg("%s() - malloc(%d): %s\n", __func__, n, strerror(errno));
		goto __failure;
	}
	memset(mb, 0, sizeof(struct message_buffer));
	mb->wr_index = 0xFFFFFFFF;

	mb->capacity = capacity;
	if (0 == mb->capacity) {
		log_dbg_msg("%s() - initial capacity defaults to %d messages\n", __func__,  MB_INIT_CAPACITY);
		mb->capacity = MB_INIT_CAPACITY;
	}
	mb->index_mask = mb->capacity - 1;

	n = mb->capacity*sizeof(struct message);
	mb->msg = malloc(n);

	//mb->msg = aligned_alloc(sizeof(struct message), n);
/*
https://stackoverflow.com/questions/20314602/does-realloc-of-memory-allocated-by-c11-aligned-alloc-keep-the-alignment

The alignment is not kept with the pointer. When you call realloc you can
only rely on the alignment that realloc guarantees. You'll need to use
aligned_alloc to perform any reallocations.

https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152255
*/

	if (NULL == mb->msg) {
		log_sys_msg("%s() - malloc(%d): %s\n", __func__, n, strerror(errno));
		goto __failure;
	}

	/* success */
	return mb;

      __failure:
	__del_mb(mb);
	return NULL;
}

static int __expand_mb(struct message_buffer *mb)
{
	/*       0 1 2 3 4 5 6 7	8
		| | | | |w|r| | |	need move
		|w|r| | | | | | |	need move
		|r| | | | | | |w|	do not need move
	 */

	struct message *msg = NULL;
	   u32 new_capacity, n, old_capacity;

	old_capacity = mb->capacity;
	new_capacity = mb->capacity << 1;
	n = new_capacity*sizeof(struct message);
	msg = realloc(mb->msg, n);
	if (NULL == msg) {
		log_sys_msg("%s() - realloc(%d): %s\n", __func__, n, strerror(errno));
		return 1;
	}

	mb->msg = msg;
	mb->capacity = new_capacity;
	mb->index_mask = mb->capacity - 1;

	/* shift unread messages */
	if (mb->wr_index != old_capacity - 1) {
		struct message *src = &mb->msg[mb->rd_index];
		   int nm = old_capacity - mb->rd_index;
		struct message *dst = src + nm;

		n = nm * sizeof(struct message);
		memmove(dst, src, n);
		mb->rd_index += nm;
	}
	return 0;
}

static int mb_init(u32 capacity)
{
	if (global_mb) {
		log_wrn_msg("%s() - message buffer already initialized\n", __func__);
		return 0;
	}

	global_mb = __new_mb(capacity);
	if (NULL == global_mb) {
		log_err_msg("%s() - __new_mb(%d) failed\n", __func__, capacity);
		return 1;
	}

	return 0;
}

static void mb_fini(void)
{
	__del_mb(global_mb);
	global_mb = NULL;
}

static int mb_get(struct message *msg)
{
	u32 i;

	if (unlikely(NULL == global_mb)) {
		log_ftl_msg("%s() - message buffer is not initialized\n", __func__);
		exit(1);
	}

	if (mb_empty(global_mb))
		return 1;

	i = global_mb->rd_index;
	*msg = global_mb->msg[i];

	global_mb->number--;
	global_mb->rd_index++;
	global_mb->rd_index &= global_mb->index_mask;

	return 0;
}

static void mb_put(struct message *msg)
{
	int err;
	u32 i;
//	int k;

	if (unlikely(NULL == global_mb)) {
		log_ftl_msg("%s() - message buffer is not initialized\n", __func__);
		exit(1);
	}

	if (unlikely(mb_full(global_mb))) {
		err = __expand_mb(global_mb);
		if (err) {
			log_ftl_msg("%s() - __expand_mb() failed, giving up\n", __func__);
			exit(1);
		}
	}

	global_mb->number++;
	global_mb->wr_index++;
	global_mb->wr_index &= global_mb->index_mask;

	i = global_mb->wr_index;
	global_mb->msg[i] = *msg;
}

api_definition(message_buffer_api, mb_api) {
	.init = mb_init,
	.fini = mb_fini,
	.get  = mb_get,
	.put  = mb_put,
};
