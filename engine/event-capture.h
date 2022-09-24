
/*
 * epoll() based Event Capture Engine (ECAP)
 */

#ifndef _EVENT_CAPTURE_H
#define _EVENT_CAPTURE_H

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>

//	#ifndef SFD_CLOEXEC
//		#define SFD_CLOEXEC 00000000
//	#endif
//	#ifndef TFD_CLOEXEC
//		#define TFD_CLOEXEC 02000000
//	#endif

#include "message-buffer.h"
#include "api.h"

typedef u32 event_mask_t;
#define EE_POLLIN  EPOLLIN
#define EE_POLLOUT EPOLLOUT
#define EE_POLLHUP EPOLLHUP
#define EE_POLLERR EPOLLERR
#define EE_POLLRDHUP EPOLLRDHUP

struct event;

api_declaration(ecap_api) {
	void (*init)(int nch);
	void (*fini)(void);

	 int (*add)(int fd, void *dptr);
	 int (*del)(int fd);
	 int (*conf)(int fd, void *dptr, event_mask_t events, int oneshot);
	 int (*sigfd)(int signo, int *sigfd);
	 int (*timerfd)(int *fd);
	 int (*arm)(int fd, int interval, int oneshot);
	 int (*disarm)(int fd);

	/* file system events (see `man inotify`) */
	 int (*fsysfd)(int *fd);
	 int (*add_fsys_watch)(int fd, char *pathname, u32 fsys_event_mask, int *wd);
	 int (*del_fsys_watch)(int fd, int wd);

	void (*wait)(struct message_buffer_api *mb);
};

#endif
