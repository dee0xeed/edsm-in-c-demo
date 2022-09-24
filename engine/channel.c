
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "__channel.h"
#include "__smd.h"
#include "event-capture.h"
#include "logging.h"

struct channel *__edsm_new_channel(struct edsm *sm, int fd, int data_size)
{
	struct channel *ch;

	ch = malloc(sizeof(struct channel));
	if (NULL == ch) {
		log_sys_msg("%s() - malloc('ch'): %s\n", __func__, strerror(errno));
		goto __failure;
	}
	memset(ch, 0, sizeof(struct channel));

	ch->data = malloc(data_size);
	if (NULL == ch->data) {
		log_sys_msg("%s() - malloc('data'): %s\n", __func__, strerror(errno));
		goto __failure;
	}
	memset(ch->data, 0, data_size);

	ch->fd = fd;
	ch->owner = sm;
	return ch;

      __failure:
	if (ch) {
		if (ch->data)
			free(ch->data);
		free(ch);
	}
	return NULL;
}

void __edsm_del_channel(struct ecap_api *ecap, struct edsm *sm, struct channel *ch)
{
	int k, fd = ch->fd, r;

	if (fd < 0)
		goto ___free;

	ecap->del(fd);
	r = close(fd);
	if (-1 == r)
		log_sys_msg("%s() - close(%d): %s (%s)\n", __func__, fd, strerror(errno), sm->name);

	if (ch->type == CT_FSYS) {
		struct fsys_info *fi = ch->data;
		if (fi->watch) {
			for (k = 0; k < fi->nwatches; k++)
				if (fi->watch[k].pathname)
					free(fi->watch[k].pathname);
			free(fi->watch);
		}
	}

	// restore procmask // ?
     ___free:
	if (ch->data)
		free(ch->data);
	free(ch);
}

/*
 * NOTE: application must use edsm->io_attach(sm, fd)
 */
int __edsm_new_iochan(struct ecap_api *ecap, struct edsm *sm)
{
	struct channel *ch = sm->io;

	if (ch) {
		log_wrn_msg("%s() - i/o channel for '%s' already created\n", __func__, sm->name);
		return 0;
	}

	ch = __edsm_new_channel(sm, -1, sizeof(struct data_info));
	if (NULL == ch) {
		log_err_msg("%s() - __edsm_new_channel() failed\n", __func__);
		return 1;
	}

	ch->type = CT_DATA;
	sm->io = ch;
	return 0;
}

int __edsm_new_interrupt(struct ecap_api *ecap, struct edsm *sm, int signo, int seqn)
{
	struct channel *ch;
	struct intr_info *ii;
	   int fd, err;

	err = ecap->sigfd(signo, &fd);
	if (err) {
		log_err_msg("%s() - ecap->sigfd(%d) failed\n", __func__, signo);
		return 1;
	}

	ch = __edsm_new_channel(sm, fd, sizeof(struct intr_info));
	if (NULL == ch) {
		log_err_msg("%s() - __edsm_new_channel() failed\n", __func__);
		return 1;
	}

	err = ecap->add(fd, ch);
	if (err) {
		log_err_msg("%s() - ecap->add(%d) failed\n", __func__, fd);
		return 1;
	}

	err = ecap->conf(fd, ch, EE_POLLIN, 0);
	if (err) {
		log_err_msg("%s() - ecap->conf('%s',%d) failed\n", __func__, sm->name, fd);
		return 1;
	}

	ch->type = CT_INTR;
	ii = ch->data;
	ii->seqn = seqn;
	sm->intrs[seqn] = ch;

	return 0;
}

int __edsm_new_timer(struct ecap_api *ecap, struct edsm *sm, int interval, int seqn)
{
	struct tick_info *ti;
	struct channel *ch;
	   int fd, err;

	err = ecap->timerfd(&fd);
	if (err) {
		log_err_msg("%s() - ecap->timerfd('%s') failed\n", __func__, sm->name);
		return 1;
	}

	ch = __edsm_new_channel(sm, fd, sizeof(struct tick_info));
	if (NULL == ch) {
		log_err_msg("%s() - __edsm_new_channel('%s') failed\n", __func__, sm->name);
		return 1;
	}

	err = ecap->add(fd, ch);
	if (err) {
		log_err_msg("%s() - ecap->add('%s',%d) failed\n", __func__, sm->name, fd);
		return 1;
	}

	err = ecap->conf(fd, ch, EE_POLLIN, 0);
	if (err) {
		log_err_msg("%s() - ecap->conf('%s',%d) failed\n", __func__, sm->name, fd);
		return 1;
	}

	err = ecap->disarm(fd);
	if (err) {
		log_err_msg("%s() - ecap->disarm('%s',%d) failed\n", __func__, sm->name, fd);
		return 1;
	}

	ch->type = CT_TICK;
	ti = ch->data;
	ti->seqn = seqn;
	ti->interval_ms = interval;
	sm->timers[seqn] = ch;

	return 0;
}

int __edsm_new_fschan(struct ecap_api *ecap, struct edsm *sm, u32 nw)
{
	struct channel *ch = sm->fsys;
	struct fsys_info *fi;
	   int fd = -1, err;

	if (ch) {
		log_wrn_msg("%s() - fsys channel for '%s' already created\n", __func__, sm->name);
		return 0;
	}

	err = ecap->fsysfd(&fd);
	if (err) {
		log_err_msg("%s() - ecap->inofd() failed\n", __func__);
		return 1;
	}

	if (0 == nw)
		nw = DEFAULT_NWATCHES;

	ch = __edsm_new_channel(sm, fd, sizeof(struct fsys_info));
	if (NULL == ch) {
		log_err_msg("%s() - __edsm_new_channel() failed\n", __func__);
		return 1;
	}

	fi = ch->data;
	fi->nwatches = nw;

	fi->watch = malloc(nw * sizeof(struct fsys_watch));
	if (NULL == fi->watch) {
		log_sys_msg("%s() - malloc('watch'): %s\n", __func__, strerror(errno));
		goto __failure;
	}
	memset(fi->watch, 0, nw*sizeof(struct fsys_watch));

	fi->fsys_event_buf = malloc(FSYS_EVENT_BUF_SIZE);
	if (NULL == fi->fsys_event_buf) {
		log_sys_msg("%s() - malloc('buf'): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	fi->fsys_event = (struct inotify_event*)fi->fsys_event_buf;
	fi->filename = (char*)fi->fsys_event_buf + sizeof(struct inotify_event);

	err = ecap->add(fd, ch);
	if (err) {
		log_err_msg("%s() - ecap->add(%d) failed\n", __func__, fd);
		goto __failure;
	}

	err = ecap->conf(fd, ch, EE_POLLIN, 0);
	if (err) {
		log_err_msg("%s() - ecap->conf('%s',%d) failed\n", __func__, sm->name, fd);
		goto __failure;
	}

	ch->type = CT_FSYS;
	sm->fsys = ch;
	log_inf_msg("%s() - fsys channel for '%s' created (%d watches)\n", __func__, sm->name, fi->nwatches);
	return 0;

      __failure:
	if (-1 != fd)
		close(fd);
	if (ch) {
		if (fi->watch)
			free(fi->watch);
		if (fi->fsys_event_buf)
			free(fi->fsys_event_buf);
		free(fi);
		free(ch);
	}
	return 1;
}
