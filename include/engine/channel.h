
#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/inotify.h>
#include "numeric-types.h"

struct data_info {
	 int bytes_avail;
};

struct fsys_watch {
	char *pathname;
	 int event_count[12];
	void *info;
};

struct fsys_info {
	    u8 *fsys_event_buf;
	struct inotify_event *fsys_event; /* points to .fsys_event_buf */
	  char *filename;		  /* points to name in the buf */
	struct fsys_watch *watch;
	   int nwatches;
};

struct edsm;

struct channel {
	   u32 type;
	   int fd;
	struct edsm *owner;
	   u32 events;
	   u32 flags;
	  void *data;	/* .type dependent data */
};

#endif
