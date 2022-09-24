
#ifndef ECHO_SERVER_MACHINES
#define ECHO_SERVER_MACHINES

// TX and RX
#define M0_IDLE		0
#define M1_WORK		1
#define M1_MORE		1
#define M0_FREE		0
#define M1_DONE		1
#define M2_FAIL		2
#define M4_BUFULL	4

// WORKER
#define M0_WORK		0
#define M3_IDLE		3
#define M0_GETP		0
#define M0_ACKP		0

#define M0_MEET		0
#define M0_SEEOFF	0
#define M1_GONE		1
#define M2_GONE		2

// MANAGER
// LISTENER

#include "data-buffer.h"
#include "edsm.h"
#include "object-pool-api.h"

typedef int (*need_more_data_f)(struct dbuf *buf);
typedef int (*on_buffer_full_f)(struct dbuf *buf, int need);

struct io_context {
	struct dbuf *buf;
	int fd;
	need_more_data_f need_more;
	on_buffer_full_f on_buffer_full;
	int timeout;		// in milliseconds
};

struct rxtx_info {
	struct edsm *requester;
//	  char errmsg[256];
	struct object_pool_api *pool;
	struct io_context *ctx;
};

struct listener_info {
	   int l_sock;
	   u16 port;
	struct edsm *manager;
};

struct client_info {
	   int d_sock;
	  char addr[32];
};

struct worker_info {
	struct edsm *manager;
	struct dbuf ibuf;
	struct dbuf obuf;

	struct object_pool_api *pool;
	struct object_pool *rx_pool;
	struct object_pool *tx_pool;
	struct edsm_template *rx_tmpl;
	struct edsm_template *tx_tmpl;

	struct client_info *ci;
	struct io_context io_ctx;
};

#endif
