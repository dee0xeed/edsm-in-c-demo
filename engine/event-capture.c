
/*
 * epoll based event capture engine
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "event-capture.h"
#include "logging.h"

static int epfd;

static void ecap__init(int nch)
{
	epfd = epoll_create(nch);
	if (-1 == epfd) {
		log_ftl_msg("%s() - epoll_create(%d): %s\n", __func__, nch, strerror(errno));
		exit(1);
	}
}

static void ecap__fini(void)
{
	int r = close(epfd);
	if (-1 == r)
		log_sys_msg("%s() - close('epfd %d'): %s\n", __func__, epfd, strerror(errno));
}

static int ecap__add(int fd, void *dptr)
{
	struct epoll_event waitfor = {0};
	   int flags, r;

	waitfor.data.ptr = dptr;

	r = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &waitfor);
	if (-1 == r) {
		log_sys_msg("%s() - epoll_ctl(%d,'ADD',%d): %s\n", __func__, epfd, fd, strerror(errno));
		return 1;
	};
	flags = fcntl(fd, F_GETFL);
	if (-1 == flags) {
		log_sys_msg("%s() - fcntl(%d,'GETFL'): %s\n", __func__, fd, strerror(errno));
		return 1;
	}
	flags |= O_NONBLOCK;
	r = fcntl(fd, F_SETFL, flags);
	if (-1 == r) {
		log_sys_msg("%s() - fcntl(%d,'SETFL'): %s\n", __func__, fd, strerror(errno));
		return 1;
	}

	/* success */
	return 0;
}

static int ecap__del(int fd)
{
	struct epoll_event waitfor = {0};
	   int r;

	r = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &waitfor);
	if (-1 == r) {
		log_sys_msg("%s() - epoll_ctl(%d,DEL,%d): %s\n", __func__, epfd, fd, strerror(errno));
		return 1;
	}
	return 0;
}

static int ecap__sigfd(int signo, int *sigfd)
{
	sigset_t ss;
	     int fd;

	/* block the signal */
	sigemptyset(&ss);
	sigaddset(&ss, signo);
	sigprocmask(SIG_BLOCK, &ss, NULL);

	fd = signalfd(-1, &ss, SFD_CLOEXEC);
	if (-1 == fd) {
		log_sys_msg("%s() - signalfd(): %s\n\n", __func__, strerror(errno));
		return 1;
	}

	*sigfd = fd;
	return 0;
}

static int ecap__timerfd(int *timerfd)
{
	int fd;

	fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
	if (-1 == fd) {
		log_sys_msg("%s() - timerfd_create(): %s\n", __func__, strerror(errno));
		return 1;
	};

	*timerfd = fd;
	return 0;
}

static int ecap__inofd(int *inofd)
{
	int fd;

	fd = inotify_init();
	if (-1 == fd) {
		log_sys_msg("%s() - inotify_init(): %s\n", __func__, strerror(errno));
		return 1;
	}

	*inofd = fd;
	return 0;
}

static int ecap__new_inowd(int fd, char *pathname, u32 fsys_event_mask, int *inowd)
{
	int wd;

	wd = inotify_add_watch(fd, pathname, fsys_event_mask);
	if (-1 == wd) {
		log_sys_msg("%s() - inotify_add_watch('%s'): %s\n", __func__, pathname, strerror(errno));
		return 1;
	}

	*inowd = wd;
	return 0;
}

static int ecap__del_inowd(int fd, int wd)
{
	int r;

	r = inotify_rm_watch(fd, wd);
	if (-1 == r) {
		log_sys_msg("%s() - inotify_rm_watch(%d,%d): %s\n", __func__, fd, wd, strerror(errno));
		return 1;
	}
	return 0;
}

static int ecap__conf(int fd, void *dptr, event_mask_t events, int oneshot)
{
	struct epoll_event waitfor = {0};
	   int r;

	waitfor.data.ptr = dptr;
	waitfor.events = events;

	if (oneshot && events)
		waitfor.events |= EPOLLONESHOT;

	r = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &waitfor);
	if (-1 == r) {
		log_sys_msg("%s() - epoll_ctl(%d, MOD, %d): %s\n", __func__, epfd, fd, strerror(errno));
		return 1;
	}
	return 0;
}

static int ecap__arm_timer(int fd, int interval, int oneshot)
{
	struct itimerspec its;
	   int r;

	memset(&its, 0, sizeof(struct itimerspec));

	its.it_value.tv_sec = interval/1000;
	its.it_value.tv_nsec = (interval % 1000)*1000*1000;

	if (!oneshot) {

		struct itimerspec curr;

		r = timerfd_gettime(fd, &curr);
		if (-1 == r) {
			log_sys_msg("%s() - timerfd_gettime(): %s\n", __func__, strerror(errno));
			return 1;
		}

		// why this only for periodic?...
		if (curr.it_value.tv_sec || curr.it_value.tv_nsec) {
			its.it_value.tv_sec = curr.it_value.tv_sec;
			its.it_value.tv_nsec = curr.it_value.tv_nsec;
		}

		its.it_interval.tv_sec = interval/1000;
		its.it_interval.tv_nsec = (interval % 1000)*1000*1000;;
	}

	r = timerfd_settime(fd, 0, &its, NULL);
	if (-1 == r) {
		log_sys_msg("%s() - timerfd_settime(%d): %s\n", __func__, fd, strerror(errno));
		return 1;
	}

	return 0;
}

static int ecap__disarm_timer(int fd)
{
	struct itimerspec its;
	   int r;

	memset(&its, 0, sizeof(struct itimerspec));

	r = timerfd_settime(fd, 0, &its, NULL);
	if (-1 == r) {
		log_sys_msg("%s() - timerfd_settime(%d): %s\n", __func__, fd, strerror(errno));
		return 1;
	}
	return 0;
}

#define MAX_EVENTS 32
static struct epoll_event events[MAX_EVENTS];

static void ecap__wait(struct message_buffer_api *mb)
{
	struct message msg = {0};
	   int n, k;

	n = epoll_wait(epfd, events, MAX_EVENTS, -1); /* infinite timeout */
	if (-1 == n) {
		if (EINTR != errno)
			log_ops_msg("%s() - epoll_wait(): %s\n", __func__, strerror(errno));
		return; // exit?
	}

	for (k = 0; k < n; k++) {
		msg.ptr = events[k].data.ptr;
		msg.u64 = events[k].events;
		mb->put(&msg);
	}
}

api_definition(ecap_api, ecap_api) {
	.init = ecap__init,
	.fini = ecap__fini,

	.add = ecap__add,
	.del = ecap__del,
	.conf = ecap__conf,

	.sigfd = ecap__sigfd,

	.timerfd = ecap__timerfd,
	.arm = ecap__arm_timer,
	.disarm = ecap__disarm_timer,

	.fsysfd = ecap__inofd,
	.add_fsys_watch = ecap__new_inowd,
	.del_fsys_watch = ecap__del_inowd,

	.wait = ecap__wait,
};
