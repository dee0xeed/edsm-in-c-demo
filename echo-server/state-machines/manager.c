
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

int manager_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	me->data = dptr;	/* struct echo_server */
	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int manager_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int manager_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

/* message from listener, new client */
int manager_work_M0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct client_info *ci = dptr;
	struct echo_server *es = me->data;
	struct object_pool *p = &es->worker_pool;
	struct edsm *worker;
	struct edsm_api *e = me->engine;

	worker = es->pool->get(p);
	if (unlikely(NULL == worker)) {
		log_err_msg("%s() - pool->get('%s') failed\n", __func__, p->name);
		e->msg(me, es->listener, ci, M0_SEEOFF);
		return 0;
	};

	if (0 == p->avail)
		add_sm(e, &es->mt[TN_WORKER], p, es);

	e->msg(me, worker, ci, M1_WORK);
	return 0;
}

/* message from worker, client gone */
int manager_work_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	struct client_info *ci = dptr;
	struct echo_server *es = me->data;

	me->engine->msg(me, es->listener, ci, M0_SEEOFF);
	return 0;
}

int manager_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int manager_work_S0(struct edsm *src, struct edsm *me, void *dptr)
{
	 /* exit event capture loop */
	return 1;
}

int manager_work_S1(struct edsm *src, struct edsm *me, void *dptr)
{
	 /* exit event capture loop */
	return 1;
}
