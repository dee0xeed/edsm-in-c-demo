
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

#define HAVE_T0		(1 << 0)
#define HAVE_D0		(1 << 1)
#define HAVE_D2		(1 << 2)

void __on_rx_done(struct edsm *me, struct edsm *requester, int mcode)
{
	me->engine->io_detach(me);
	me->engine->msg(me, requester, NULL, mcode);
	me->engine->msg(me, me, NULL, M0_FREE);
}

int rx_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi;

	rxi = malloc(sizeof(struct rxtx_info));
	if (NULL == rxi) {
		log_ftl_msg("%s/%s() - malloc(): %s\n", me->name, __func__, strerror(errno));
		exit(1);
	}
	memset(rxi, 0, sizeof(struct rxtx_info));
	rxi->pool = dptr;
	me->data = rxi;

	me->engine->msg(me, me, NULL, M0_IDLE);
	return 0;
}

int rx_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int rx_idle_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;
	struct object_pool_api *pool = rxi->pool;
	struct object_pool *p = me->restroom;
	   int err;

	me->flags = 0;
	err = pool->put(p, me);
	if (unlikely(err)) {
		log_ftl_msg("%s/%s() - pool_put(->'%s') (p@%p -> m@%p) failed\n",
			me->name, __func__, p->name, me, p);
		exit(1);
	}
	return 0;
}

/* message from worker, 'give me data' */
int rx_idle_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;
	struct io_context *ctx = dptr;
	struct dbuf *rxb = ctx->buf;

	rxi->requester = src;
	rxi->ctx = ctx;

	/* reset buffer */
	memset(rxb->loc, 0, rxb->len);
	rxb->pos = rxb->loc;
	rxb->cnt = 0;

	me->engine->io_attach(me, ctx->fd);
	me->engine->tm_set_interval(me, 0, ctx->timeout);
	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int rx_idle_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	/* do nothing */
	return 0;
}

int rx_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	me->engine->io_enable(me, EDSM_POLLIN, IO_ONESHOT);
	me->engine->tm_start(me, 0, TM_TIMEOUT);
	return 0;
}

int rx_work_D0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;
	struct dbuf *rxb = rxi->ctx->buf;
	struct channel *ch = me->io;
	   int fd = ch->fd;
	struct data_info *di = ch->data;
	   int r, ba, left, err, total;

	if (me->flags) {
		log_ops_msg("%s()/%s - there are partly handled events (0x%.2X)\n",
			__func__, me->name, me->flags);
		return 0;
	}

	ba = di->bytes_avail;
	left = rxb->len - rxb->cnt;
	if (ba > left)
		ba = left;
	total = rxb->cnt + ba;

	/* accumulate data */
	r = read(fd, rxb->pos, ba);
	if (-1 == r) {
		log_sys_msg("%s/%s() - read(%d): %s\n", me->name, __func__, fd, strerror(errno));
		goto __failure;
	}

	/* success */
	rxb->lr_cnt = r;
	rxb->cnt += r;
	rxb->pos += r;

	if (rxb->cnt == rxb->len) {
		err = rxi->ctx->on_buffer_full(rxb, total);
		if (err) {
			log_err_msg("%s() - on_buffer_full() failed\n", __func__);
			goto __failure;
		}
	}

	if (rxi->ctx->need_more(rxb)) {
		/* reenable POLLIN */
		me->engine->io_enable(me, EDSM_POLLIN, IO_ONESHOT);
	} else {
		__on_rx_done(me, rxi->requester, M1_DONE);
	}
	return 0;

      __failure:
	log_err_msg("%s/%s() - failure (requester - %s)\n", __func__, me->name, rxi->requester->name);
	__on_rx_done(me, rxi->requester, M2_FAIL);
	return 0;
}

int rx_stop_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;
	log_inf_msg("%s/%s() - buffer full (requester - %s)\n", me->name, __func__, rxi->requester->name);
	return 0;
}

int rx_stop_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int rx_work_D2(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;

	if ((0 == me->flags) || (HAVE_D2 == me->flags)) {
		log_dbg_msg("%s/%s() - hung up (requester - %s)\n", me->name, __func__, rxi->requester->name);
		__on_rx_done(me, rxi->requester, M2_GONE);
	} else {
		log_ops_msg("%s()/%s/%s - there are partly handled events (0x%.2X)\n",
			__func__, me->name, rxi->requester->name, me->flags);
	}

	me->flags |= HAVE_D2;
	return 0;
}

int rx_work_T0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct rxtx_info *rxi = me->data;

	if (0 == me->flags) {
		log_dbg_msg("%s/%s() - timeout (requester - %s)\n", me->name, __func__, rxi->requester->name);
		__on_rx_done(me, rxi->requester, M2_GONE);
	} else {
		log_ops_msg("%s()/%s/%s - there are partly handled events (0x%.2X)\n",
			__func__, me->name,  rxi->requester->name, me->flags);
	}

	me->flags |= HAVE_T0;
	return 0;
}

int rx_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	me->engine->tm_stop(me, 0);
	return 0;
}
