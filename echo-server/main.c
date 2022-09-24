
#include <stdio.h>
#include <stdlib.h>

#include "edsm.h"
#include "macro-magic.h"
#include "echo-server.h"
#include "state-machines/state-machines.h"
#include "logging.h"
#include "backtrace.h"

void start_machines(struct edsm_api *e, struct echo_server *s)
{
	struct object_pool *p;
	struct edsm *m;
	   int k;

static	struct listener_info li = {0};

	m = s->poolman = e->new(&s->mt[TN_POOLMAN]);
	e->run(m, s);

	p = &s->rx_pool;
	for (k = 0; k < p->capacity; k++) {
		m = e->new(&s->mt[TN_RX]);
		m->restroom = p;
		e->run(m, s->pool);
	}

	p = &s->tx_pool;
	for (k = 0; k < p->capacity; k++) {
		m = e->new(&s->mt[TN_TX]);
		m->restroom = p;
		e->run(m, s->pool);
	}

	m = s->manager = e->new(&s->mt[TN_MANAGER]);
	e->run(m, s);

	p = &s->worker_pool;
	for (k = 0; k < p->capacity; k++) {
		m = e->new(&s->mt[TN_WORKER]);
		m->restroom = p;
		e->run(m, s);
	}

	li.port = s->tcp_port;
	li.manager = s->manager;
	m = s->listener = e->new(&s->mt[TN_LISTENER]);
	e->run(m, &li);
}

struct edsm_template mt[] = {

	[TN_POOLMAN] = {
		.source = "poolman.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 1,
		.instance_seqn = 0,
	},

	[TN_RX] = {
		.source = "rx.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 100,
		.instance_seqn = 0,
	},

	[TN_TX] = {
		.source = "tx.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 100,
		.instance_seqn = 0,
	},

	[TN_WORKER] = {
		.source = "worker.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 100,
		.instance_seqn = 0,
	},

	[TN_MANAGER] = {
		.source = "manager.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 1,
		.instance_seqn = 0,
	},

	[TN_LISTENER] = {
		.source = "listener.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 1,
		.instance_seqn = 0,
	},
};

struct edsm_api *engine = NULL;
struct echo_server server = {
	.mt = mt,
	.tcp_port = TCP_PORT,
};

void get_envars(struct echo_server *s)
{
	char *o;

	o = getenv("TCP_PORT");
	if (o)
		sscanf(o, "%hu", &s->tcp_port);

	o = getenv("MIL_THRESHOLD");
	if (o)
		sscanf(o, "%d", &s->mil_threshold);
}

int main(void)
{
	int err = bt_init(-1); /* -1 => default trace depth */
	if (err)
		exit(1);

	get_envars(&server);
	set_mil_threshold(server.mil_threshold);

	log_inf_msg("*** EDSM-G2 based echo-server V5 ***\n");
	engine = edsm_init("/opt/echod", mt, ARRAY_SIZE(mt));
	start_machines(engine, &server);
	main_loop(engine);
	return 0;
}
