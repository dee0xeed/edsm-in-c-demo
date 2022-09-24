
#ifndef CHANNEL_H_FOR_ENGINE
#define CHANNEL_H_FOR_ENGINE

//#include <sys/inotify.h>
#include <sys/signalfd.h>

#include "numeric-types.h"

/* for channel.type */
#define CT_NONE 0
#define CT_MQUE 0
#define CT_TICK 1
#define CT_INTR 2
#define CT_DATA 3
#define CT_FSYS 4

/* for channel.flags */
//#define EEF_ONE_SHOT	(1 << 15)
//#define EEF_ARMED	(1 << 16)
#define EEF_NOFIONREAD	(1 << 0)
/* for listening sockets */

/* channel.type specific data structures */

struct tick_info {
	int interval_ms;
	u64 expire_count;
	u32 seqn;
};

struct intr_info {
	struct signalfd_siginfo si;
	   u32 seqn;
};

#include "engine/channel.h"
#include "event-capture.h"

#define FSYS_EVENT_BUF_SIZE 256
#define DEFAULT_NWATCHES 128

 int __edsm_new_iochan(struct ecap_api *ecap, struct edsm *sm);
 int __edsm_new_interrupt(struct ecap_api *ecap, struct edsm *sm, int signo, int seqn);
 int __edsm_new_timer(struct ecap_api *ecap, struct edsm *sm, int interval, int seqn);
 int __edsm_new_fschan(struct ecap_api *ecap, struct edsm *sm, u32 nw);
void __edsm_del_channel(struct ecap_api *ecap, struct edsm *sm, struct channel *ch);

#endif
