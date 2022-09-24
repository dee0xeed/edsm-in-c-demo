
#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H

#include "numeric-types.h"
#include "object-pool-api.h"
#include "edsm.h"

#define TCP_PORT	1771

/* machine template numbers */
#define TN_POOLMAN	0
#define TN_RX		1
#define TN_TX		2
#define TN_WORKER	3
#define TN_MANAGER	4
#define TN_LISTENER	5

struct echo_server {

	struct edsm_template *mt;

	struct edsm *poolman;
	struct edsm *listener;
	struct edsm *manager;
	   u16 tcp_port;

	struct object_pool_api *pool;

	struct object_pool worker_pool;
	struct object_pool rx_pool;
	struct object_pool tx_pool;

	int mil_threshold;
};

#endif
