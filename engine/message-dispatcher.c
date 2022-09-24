
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "compiler.h"
#include "event-capture.h"
#include "__channel.h"
#include "__smd.h"
#include "message-dispatcher.h"
#include "logging.h"

static char event_type_chars[] = "MTSDF";

u32 __fsys_seqn(u32 mask)
{
	if (unlikely(0 == mask)) {
		/* must not happen */
		log_bug_msg("%s() - empty inotify event mask\n", __func__);
		exit(1);
	}

	return __builtin_ctz(mask);
	/* see https://gcc.gnu.org/onlinedocs/gcc-4.5.4/gcc/Other-Builtins.html */
}

int __read_tick_data(struct channel *ch)
{
	struct tick_info *ti = ch->data;
	   int n = sizeof(u64);
	   int r, fd = ch->fd;

	r = read(fd, &ti->expire_count, n);
	if (unlikely(-1 == r)) {
		log_sys_msg("%s() - read(%d): %s\n", __func__, fd, strerror(errno));
		return 1;
	}
	if (unlikely(n != r)) {
		log_ops_msg("%s() - read(%d) = %d: %s\n", __func__, fd, r, strerror(errno));
		return 1;
	}
	return 0;
}

int __read_intr_data(struct channel *ch)
{
	struct intr_info *ii = ch->data;
	int n = sizeof(struct signalfd_siginfo);
	int r, fd = ch->fd;

	r = read(fd, &ii->si, n);
	if (unlikely(-1 == r)) {
		log_sys_msg("%s() - read(%d): %s\n", __func__, fd, strerror(errno));
		return 1;
	}
	if (unlikely(n != r)) {
		log_ops_msg("%s() - read(%d) = %d: %s\n", __func__, fd, r, strerror(errno));
		return 1;
	}
	return 0;
}

int __read_data(struct channel *ch)
{
	/* it is application program task */
	return 0;
}

/*
 * a little bit tricky function that reads
 * exactly _one_ event from inotify system
 */
int __inotify_get_ONE_event(int fsfd, u8 *buf, int bsize)
{
	struct inotify_event *fsev;
	int r, len, ret = 0;

	memset(buf, 0, bsize);
	len = sizeof(struct inotify_event);

	for (;;) {
		r = read(fsfd, buf, len);
		if (r > 0)
			break;

		/* -1 assumed */
		if (errno != EINVAL) {
			log_sys_msg("%s() - read(fsfd): %s\n", __func__, strerror(errno));
			ret = 1;
			break;
		}

		/*
		 * EINVAL => buffer too small
		 * First check if we have something,
		 * then increase len
		 */

		fsev = (struct inotify_event *)buf;
		if (fsev->mask) /* should be enough */
			break;

		len += sizeof(struct inotify_event);
		if (len >= bsize) {
			log_err_msg("%s() - buffer for inotify event is too small (%d bytes)\n", __func__, bsize);
			ret = 1;
			break;
		}
	}
	return ret;
}

int __read_fsys_data(struct channel *ch)
{
	struct fsys_info *fi = ch->data;
	   int err, fd = ch->fd;

	err = __inotify_get_ONE_event(fd, fi->fsys_event_buf, FSYS_EVENT_BUF_SIZE);
	if (unlikely(err)) {
		log_err_msg("%s() - __inotify_get_ONE_event(%d) failed\n", __func__, fd);
		return 1;
	}
	return 0;
}

static edsm_read_os_event_data __read_os_event_data[] = {
	[CT_NONE] = NULL,
	[CT_TICK] = __read_tick_data,
	[CT_INTR] = __read_intr_data,
	[CT_DATA] = __read_data,
	[CT_FSYS] = __read_fsys_data,
};

int __read_os_event_info(struct channel *ch)
{
	edsm_read_os_event_data f;
	if (CT_DATA == ch->type)
		return 0;

	f = __read_os_event_data[ch->type];
	return f ? f(ch) : 1;
}

u32 __get_os_event_seqn(struct channel *ch)
{
	u32 seqn = 0;

	if (CT_TICK == ch->type) {
		struct tick_info *ti = ch->data;
		seqn = ti->seqn;
		goto __ret;
	}

	if (CT_INTR == ch->type) {
		struct intr_info *ii = ch->data;
		seqn = ii->seqn;
		goto __ret;
	}

	if (CT_FSYS == ch->type) {
		struct fsys_info *fi = ch->data;
		struct inotify_event *fsev = fi->fsys_event;
		seqn = __fsys_seqn(fsev->mask);
		goto __ret;
	}

	/* i/o channel */
	if (ch->events & (EE_POLLHUP | EE_POLLERR | EE_POLLRDHUP)) {
		seqn = 2;
		goto __ret;
	}

	if (ch->events & EE_POLLIN) {
		struct data_info *di = ch->data;
		int r;

		if (ch->flags & EEF_NOFIONREAD)
			/* account for listening sockets (seqn == 0) */
			goto __ret;

		r = ioctl(ch->fd, FIONREAD, &di->bytes_avail);
		if (-1 == r) {
			log_ops_msg("%s() - ioctl(%d, FIONREAD): %s\n", __func__, ch->fd, strerror(errno));
			seqn = 2;
			goto __ret;
		}
		if (0 == di->bytes_avail)
			seqn = 2;
		goto __ret;
	}

	if (ch->events & EE_POLLOUT)
		seqn = 1;

      __ret:
	return seqn;
}

#define msg_ACCEPTED						\
	log_sm_dbg_msg(						\
		"%s@%s ACCEPTED '%c%d' from %s\n",		\
		dst->name, state->name, etc, seqn,		\
		src == dst ? "self" : src ? src->name : "OS"	\
	);

#define msg_TRANSITION						\
	log_sm_dbg_msg(						\
		"'%s' transited to '%s'\n",			\
		dst->name, state->name				\
	);

#define msg_DROPPED						\
	log_inf_msg(						\
		"%s@%s DROPPED '%c%d' from %s\n",		\
		dst->name, state->name, etc, seqn,		\
		src == dst ? "self" : src ? src->name : "OS"	\
	);

struct reflex *__edsm_get_reflex(struct state *st, int code)
{
	int row = code >> 16;
	int col = code & 0xFFFF;
	return st->reflex_matrix[row][col];
}

int __edsm_dispatch_message(struct message *msg)
{
	struct edsm *src = msg->src;
	struct edsm *dst = msg->dst;
	   u32 code = msg->u64;
	   u32 seqn = code & 0x0000FFFF;
	struct state *state = dst->state;
	struct reflex *r = NULL;
	  char etc = event_type_chars[code >> 16];

	r = __edsm_get_reflex(state, code);
	if (unlikely(NULL == r)) {
		msg_DROPPED;
		return 0;
	} else {
		msg_ACCEPTED;
	}

	if (r->action) {
		int ret;
		ret = r->action(src, dst, msg->ptr);
		if (unlikely(ret))
			state->leave(src, dst, msg->ptr);
		return ret;
	}

	state->leave(src, dst, msg->ptr);
	dst->state = state = r->next_state;
	msg_TRANSITION;
	return state->enter(src, dst, msg->ptr);
}
