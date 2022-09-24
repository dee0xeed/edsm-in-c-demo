
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <systemd/sd-daemon.h>

#include "compiler.h"
#include "data-buffer.h"
#include "object-pool-api.h"
#include "state-machines.h"
#include "../echo-server.h"
#include "add-sm.h"
#include "logging.h"

void __pm__init_pools(struct echo_server *s)
{
	int err;

	s->pool = get_api("object_pool_api");
	if (NULL == s->pool) {
		log_err_msg("%s() - get_api('object_pool_api') failed\n", __func__);
		exit(1);
	}

	s->rx_pool.name = "RX";
	s->rx_pool.capacity = 16;
	s->rx_pool.capdelta = 4;
	err = s->pool->init(&s->rx_pool);
	if (err) {
		log_err_msg("%s() - pool->init('RX') failed\n", __func__);
		exit(1);
	}

	s->tx_pool.name = "TX";
	s->tx_pool.capacity = 16;
	s->tx_pool.capdelta = 4;
	err = s->pool->init(&s->tx_pool);
	if (err) {
		log_err_msg("%s() - pool->init('TX') failed\n", __func__);
		exit(1);
	}

	s->worker_pool.name = "WORKERS";
	s->worker_pool.capacity = 128;
	s->worker_pool.capdelta = 32;
	err = s->pool->init(&s->worker_pool);
	if (err) {
		log_err_msg("%s() - pool->init('WORKER') failed\n", __func__);
		exit(1);
	}
}

int poolman_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct echo_server *es = dptr;
	  char *s;
	me->data = dptr;	/* struct webd* */

	__pm__init_pools(es);

	/* systemd watchdog facility */
	s = getenv("WATCHDOG_USEC");
	if (s) {
		int to;
		sscanf(s, "%d", &to);
		to /= 1000; /* usec -> msec */
		log_inf_msg("%s() - systemd watchdog timeout is %d msec\n", __func__, to);
		to /= 5;
		me->engine->tm_set_interval(me, 1, to);
		log_inf_msg("%s() - watchdog kick period set to %d msec\n", __func__, to);
		me->engine->tm_start(me, 1, TM_PERIODIC);
	}

	me->engine->tm_start(me, 0, TM_PERIODIC);
	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int poolman_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int poolman_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

void __pm__shrink_pool(struct object_pool *p, struct object_pool_api *po, struct edsm_api *eo)
{
	struct edsm *m;
	   int k;

	if (p->avail_min <= p->capdelta)
		return;

	for (k = 0; k < p->capdelta; k++) {
		m = po->get(p);
		eo->del(m);
	}

	po->shrink(p);
	p->avail_min = p->capacity;
//	log_msg_dbg("%s('%s') - have shrunk, capacity is %d\n", __func__, p->name, p->capacity);
}

/* shrink pools if needed */
int poolman_work_T0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct echo_server *es = me->data;

	__pm__shrink_pool(&es->worker_pool, es->pool, me->engine);
	__pm__shrink_pool(&es->rx_pool, es->pool, me->engine);
	__pm__shrink_pool(&es->tx_pool, es->pool, me->engine);

	return 0;
}

int poolman_work_T1(struct edsm *me)
{
	sd_notifyf(0, "WATCHDOG=1");
	return 0;
}

int poolman_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}
