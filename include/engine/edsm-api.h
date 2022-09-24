
#ifndef EDSM_API_H
#define EDSM_API_H

#include "api.h"
#include "smd.h"
#include "channel.h"

/* timer start mode */
#define TM_PERIODIC	0
#define TM_ONESHOT	1
#define TM_HEARTBEAT	TM_PERIODIC
#define TM_TIMEOUT	TM_ONESHOT

#define IO_FOREVER	0
#define IO_ONESHOT	1

#define FS_FOREVER	0
#define FS_ONESHOT	1

api_declaration(edsm_api) {

	/*part 0: message handling */
	   int (*exec)(void);
	  void (*wait)(void);

	/* part 1: state machine ops */
	struct edsm *(*new)(struct edsm_template *t);
	  void (*del)(struct edsm *sm);
	  void (*run)(struct edsm *sm, void *dptr);
	  void (*msg)(struct edsm *src, struct edsm *dst, void *ptr, u32 code);

	/* part 2: i/o channel ops */
	  void (*io_attach)(struct edsm *sm, int fd);
	  void (*io_detach)(struct edsm *sm);
	  void (*io_enable)(struct edsm *sm, u32 what, int mode);
	  void (*io_disable)(struct edsm *sm);

	/* part 3: timers ops */
	  void (*tm_set_interval)(struct edsm *sm, int tn, int interval);
	  void (*tm_start)(struct edsm *sm, int tn, int mode);
	  void (*tm_stop)(struct edsm *sm, int tn);

	/* part 4: signals ops */
	  // nothing needed (?)

	/* part 5: file system */
	  void (*fs_new_watch)(struct edsm *sm, char *pathname, u32 fsys_event_mask, void *data, int dsize);
	  void (*fs_del_watch)(struct edsm *sm, int wd);
};

struct edsm_api *edsm_init(char *dir, struct edsm_template *ta, int nt);

#endif
