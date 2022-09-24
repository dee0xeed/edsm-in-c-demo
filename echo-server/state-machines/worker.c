
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "compiler.h"
#include "data-buffer.h"
#include "object-pool-api.h"
#include "state-machines.h"
#include "../echo-server.h"
#include "add-sm.h"
#include "logging.h"

int __es_need_more(struct dbuf *b)
{
	char *eop;
	eop = strchr((char*)b->loc, '\n');
	return NULL == eop ? 1 : 0;
}

int __es_on_buffer_full(struct dbuf *b, int newlen)
{
	u8 *loc;

	newlen++;
	loc = realloc(b->loc, newlen);
	if (NULL == loc) {
		log_sys_msg("%s() - realloc(%d): %s\n", __func__, newlen, strerror(errno));
		return 1;
	}
	b->loc = loc;
	b->len = newlen;
	b->pos = b->loc + b->cnt;
	return 0;
}

int worker_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct echo_server *es = dptr;
	struct worker_info *wi;
	struct dbuf *obuf, *ibuf;

	wi = malloc(sizeof(struct worker_info));
	if (NULL == wi) {
		log_ftl_msg("%s/%s - malloc(info): %s\n", me->name, __func__, strerror(errno));
		exit(1);
	}
	memset(wi, 0, sizeof(struct worker_info));

	wi->rx_pool = &es->rx_pool;
	wi->tx_pool = &es->tx_pool;
	wi->manager = es->manager;
	wi->rx_tmpl = &es->mt[TN_RX];
	wi->tx_tmpl = &es->mt[TN_TX];
	wi->pool = es->pool;

	wi->io_ctx.need_more = __es_need_more;
	wi->io_ctx.timeout = 10000;
	wi->io_ctx.on_buffer_full = __es_on_buffer_full;

	ibuf = &wi->ibuf;
	ibuf->loc = malloc(1024);
	if (NULL == ibuf->loc) {
		log_ftl_msg("%s/%s - malloc(ibuf): %s\n", me->name, __func__, strerror(errno));
		exit(1);
	}
	ibuf->len = 1024;

	obuf = &wi->obuf;
	obuf->loc = malloc(1024);
	if (NULL == obuf->loc) {
		log_ftl_msg("%s/%s - malloc(obuf): %s\n", me->name, __func__, strerror(errno));
		exit(1);
	}
	obuf->len = 1024;

	me->data = wi;
	me->engine->msg(me, me, NULL, M0_IDLE);
	return 0;
}

int worker_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int worker_idle_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;
	struct object_pool *p = me->restroom;
	   int err;

	wi->ci = NULL;

	err = wi->pool->put(p, me);
	if (likely(0 == err))
		return 0;

	log_ftl_msg("%s/%s - pool->put(->'%s') (p@%p -> m@%p) failed\n",
		me->name, __func__, p->name, me, p);
	exit(1);
}

/* message from manager, new client */
int worker_idle_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;
	struct client_info *ci = dptr;

	if (unlikely(NULL == ci)) {
		log_bug_msg("%s/%s - '%s' provided no info\n", me->name, __func__, src->name);
		exit(1);
	}

	wi->ci = ci;

	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int worker_idle_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int worker_getp_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;
	struct object_pool *p = wi->rx_pool;
	struct client_info *ci = wi->ci;
	struct io_context *ctx = &wi->io_ctx;
	struct edsm *rx;
	struct edsm_api *e = me->engine;

	rx = wi->pool->get(p);
	if (NULL == rx) {
		log_err_msg("%s/%s - pool->get('%s') failed\n", me->name, __func__, p->name);
		me->engine->msg(me, wi->manager, ci, M1_GONE);
		me->engine->msg(me, me, NULL, M3_IDLE);
		return 0;
	}

	if (0 == p->avail)
		add_sm(e, wi->rx_tmpl, p, wi->pool);

	ctx->fd = ci->d_sock;
	ctx->buf = &wi->ibuf;
	// if needed:
	// ctx->need_more = ;
	// ctx->timeout = ;
	// ctx->on_buffer_full = ;
	me->engine->msg(me, rx, ctx, M1_WORK);
	return 0;
}

/* message from rx machine, rx done */
int worker_getp_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	me->engine->msg(me, me, NULL, M0_ACKP);
	return 0;
}

/* message from rx machine, failure */
int worker_getp_M2(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;

	me->engine->msg(me, wi->manager, wi->ci, M1_GONE);
	me->engine->msg(me, me, NULL, M3_IDLE);
	return 0;
}

int worker_getp_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int worker_ackp_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;
	struct object_pool *p = wi->tx_pool;
	struct client_info *ci = wi->ci;
	struct io_context *ctx = &wi->io_ctx;
	struct edsm *tx;
	struct dbuf *obuf = &wi->obuf, *ibuf = &wi->ibuf;
	struct edsm_api *e = me->engine;

	tx = wi->pool->get(p);
	if (NULL == tx) {
		log_err_msg("%s/%s - pool_get('%s') failed\n", me->name, __func__, p->name);
		e->msg(me, wi->manager, ci, M1_GONE);
		e->msg(me, me, NULL, M3_IDLE);
		return 0;
	}

	if (0 == p->avail)
		add_sm(e, wi->tx_tmpl, p, wi->pool);

	if (obuf->len < ibuf->len) {
		int err = __es_on_buffer_full(obuf, ibuf->len);
		if (err) {
			log_err_msg("%s() - realloc(obuf) failed\n", __func__);
			return 0;
		}
	}

	strcpy((char*)obuf->loc, (char*)ibuf->loc);
	obuf->cnt = obuf->cnt_init = strlen((char*)obuf->loc);

	ctx->fd = ci->d_sock;
	ctx->buf = &wi->obuf;
	e->msg(me, tx, ctx, M1_WORK);

	return 0;
}

/* message from TX machine, data sent */
int worker_ackp_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	me->engine->msg(me, me, NULL, M0_GETP);
	return 0;
}

/* message from TX machine, failure */
int worker_ackp_M2(struct edsm *src, struct edsm *me, void *dptr)
{
	struct worker_info *wi = me->data;

	me->engine->msg(me, wi->manager, wi->ci, M1_GONE);
	me->engine->msg(me, me, NULL, M3_IDLE);
	return 0;
}

int worker_ackp_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}
