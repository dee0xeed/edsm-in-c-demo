
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "compiler.h"
#include "data-buffer.h"
#include "object-pool-api.h"
#include "state-machines.h"
#include "logging.h"

void __on_tx_done(struct edsm *me, struct edsm *requester, int mcode)
{
	me->engine->io_detach(me);
	me->engine->msg(me, me, NULL, M0_FREE);
	me->engine->msg(me, requester, NULL, mcode);
}

void __rest(struct edsm *me)
{
	struct rxtx_info *txi = me->data;
	struct object_pool_api *pool = txi->pool;
	struct object_pool *p = me->restroom;
	   int err;

	err = pool->put(p, me);
	if (unlikely(err)) {
		log_ftl_msg("%s/%s() - pool_put(->'%s') (p@%p -> m@%p) failed\n",
			me->name, __func__, p->name, me, p);
		exit(1);
	}
}

int tx_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *txi;

	txi = malloc(sizeof(struct rxtx_info));
	if (NULL == txi) {
		log_ftl_msg("%s/%s() - malloc(): %s\n", me->name, __func__, strerror(errno));
		exit(1);
	}
	memset(txi, 0, sizeof(struct rxtx_info));
	txi->pool = dptr;
	me->data = txi;

	me->engine->msg(me, me, NULL, M0_IDLE);
	return 0;
}

int tx_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int tx_idle_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	__rest(me);
	return 0;
}

/* message from worker, 'send data' */
int tx_idle_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *txi = me->data;
	struct io_context *ctx = dptr;

	txi->requester = src;
	txi->ctx = ctx;

	me->engine->io_attach(me, ctx->fd);
	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int tx_idle_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	/* do nothing */
	return 0;
}

int tx_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *txi = me->data;
	struct dbuf *txb = txi->ctx->buf;

	txb->pos = txb->loc;
	txb->cnt = txb->cnt_init; /* restore counter */

	me->engine->io_enable(me, EDSM_POLLOUT, IO_ONESHOT);
	return 0;
}

int tx_work_D1(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *txi = me->data;
	struct dbuf *txb = txi->ctx->buf;
	struct channel *ch = me->io;
	   int r, fd = ch->fd, mcode = M1_DONE;

	if (0 == txb->cnt)
		/* most likely, it was request for asynchronous connect() */
		goto __done;

	r = write(fd, txb->pos, txb->cnt);
	if (-1 == r) {
		log_sys_msg("%s/%s() - write(fd %d, %d bytes): %s", me->name, __func__, fd, txb->cnt, strerror(errno));
		mcode = M2_FAIL;
		goto __done;
	}

	txb->pos += r;
	txb->cnt -= r;

	if (txb->cnt) {
		/* some data unsent - reenable POLLOUT */
		me->engine->io_enable(me, EDSM_POLLOUT, IO_ONESHOT);
		return 0;
	}

      __done:
	__on_tx_done(me, txi->requester, mcode);
	return 0;
}

int tx_work_D2(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *txi = me->data;

	log_dbg_msg("%s/%s() - hung up (requester - '%s')", me->name, __func__, txi->requester->name);
	__on_tx_done(me, txi->requester, M2_FAIL);
	return 0;
}

int tx_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	/* do nothing */
	return 0;
}
