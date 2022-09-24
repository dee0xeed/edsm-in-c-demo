
#include "engine/smd.h"
#include "engine/channel.h"
#include "engine/edsm-api.h"

#define main_loop(e)				\
	for (;;) {				\
		int stop;			\
		stop = (e)->exec();		\
		if (stop)			\
			break;			\
		(e)->wait();			\
	}

#include <sys/epoll.h>

#define EDSM_POLLIN		EPOLLIN
#define EDSM_POLLOUT		EPOLLOUT
#define EDSM_POLLHUP		EPOLLHUP
#define EDSM_POLLERR		EPOLLERR
#define EDSM_POLLRDHUP		EPOLLRDHUP
