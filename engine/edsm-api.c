
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "compiler.h"
#include "load-tool.h"
#include "event-capture.h"
#include "engine/edsm-api.h"
#include "strings.h"
#include "__smd.h"
#include "__channel.h"
#include "message-dispatcher.h"
#include "logging.h"

static struct ecap_api *ecap = NULL;
static struct message_buffer_api *mb = NULL;

struct edsm_api *edsm_init(char *dir, struct edsm_template *ta, int nt);

static    int edsm__process_messages(void);
static   void edsm__waitfor_messages(void);

static struct edsm *edsm__create(struct edsm_template *t);
static   void edsm__start(struct edsm *sm, void *dptr);
static   void edsm__delete(struct edsm *sm);

static   void edsm__attach_iofd(struct edsm *sm, int fd);
static   void edsm__detach_iofd(struct edsm *sm);
static   void edsm__io_enable(struct edsm *sm, u32 what, int oneshot);
static   void edsm__io_disable(struct edsm *sm);

static   void edsm__set_timer_interval(struct edsm *sm, int tn, int interval);
static   void edsm__start_timer(struct edsm *sm, int tn, int oneshot);
static   void edsm__stop_timer(struct edsm *sm, int tn);

static   void edsm__new_fsys_watch(struct edsm *sm, char *pathname, u32 fsys_event_mask, void *data, int dsize);
static   void edsm__del_fsys_watch(struct edsm *sm, int wd);

static   void edsm__put_msg(struct edsm *src, struct edsm *dst, void *ptr, u32 code);

api_definition(edsm_api, edsm_engine) {

	/* messages processing */
	.exec = edsm__process_messages,
	.wait = edsm__waitfor_messages,

	/* machine itself */
	.new = edsm__create,
	.del = edsm__delete,
	.run = edsm__start,
	.msg = edsm__put_msg,

	/* i/o */
	.io_attach = edsm__attach_iofd,
	.io_detach = edsm__detach_iofd,
	.io_enable = edsm__io_enable,
	.io_disable = edsm__io_disable,

	/* timers */
	.tm_set_interval = edsm__set_timer_interval,
	.tm_start = edsm__start_timer,
	.tm_stop = edsm__stop_timer,

	/* signals */

	/* file system monitoring */
	.fs_new_watch = edsm__new_fsys_watch,
	.fs_del_watch = edsm__del_fsys_watch,
};

static struct edsm_api *edsm = &edsm_engine;

struct edsm_api *edsm_init(char *dir, struct edsm_template *ta, int nt)
{
	struct edsm_template *t = NULL;
	   int k, err, nch = 0;

	mb = get_api("mb_api");
	if (NULL == mb) {
		log_ftl_msg("%s() - get_api('mb_api') failed\n", __func__);
		exit(1);
	}
	err = mb->init(0); /* default initial capacity */
	if (err) {
		log_ftl_msg("%s() - mb->init() failed\n", __func__);
		exit(1);
	}

	ecap = get_api("ecap_api");
	if (NULL == ecap) {
		log_ftl_msg("%s() - get_api('ecap_api') failed\n", __func__);
		exit(1);
	}

	for (k = 0, t = ta; k < nt; k++, t++) {

		struct edsm_desc *md;
		struct state *ss;

		md = smd_load_desc(dir, t->source);
		if (NULL == md) {
			log_ftl_msg("%s() - __edsm_load_desc('%s','%s') failed\n", __func__, dir, t->source);
			exit(1);
		}
		t->desc = md;

		ss = smd_get_state_set(md);
		if (NULL == ss) {
			log_ftl_msg("%s() - __edsm_get_state_set('%s') failed\n", __func__, t->source);
			exit(1);
		}
		t->state_set = ss;

		nch += md->nch * t->expected_instances;
	}

	ecap->init(nch);
	return edsm;
}

static int edsm__process_messages(void)
{
	struct message msg = {0};
	struct channel *ch = NULL;
	   u32 seqn;
	   int empty, retval = 0, err;

	for (;;) {

		empty = mb->get(&msg);
		if (empty)
			break;

		if (NULL == msg.src) {
			/* external message (i.e. from OS) */
			ch = msg.ptr;
			ch->events = msg.u64;
			err = __read_os_event_info(ch);
			if (err) {
				log_ftl_msg("%s() - __read_os_event_info() failed\n", __func__);
				exit(1);
			}
			msg.dst = ch->owner;
			msg.u64 = ch->type << 16;
			seqn = __get_os_event_seqn(ch);
			msg.u64 |= seqn;
		};

		retval = __edsm_dispatch_message(&msg);
		if (retval)
			break;
	}
	return retval;
}

static void edsm__waitfor_messages(void)
{
	ecap->wait(mb);
}

char *__edsm_make_sm_name(struct edsm_template *t)
{
	struct edsm_desc *md = t->desc;
	static char seqn_str[16];
	   int l;
	  char *s;

	sprintf(seqn_str, "%d", t->instance_seqn);
	l = 8 + strlen(md->name) + strlen(seqn_str);
	s = malloc(l);
	if (NULL == s)
		return NULL;

	sprintf(s, "%s_%.4d", md->name, t->instance_seqn);
	t->instance_seqn++;
	return s;
}

static struct edsm *edsm__create(struct edsm_template *t)
{
	struct edsm_desc *md = t->desc;
	struct state *sset = t->state_set;
	struct edsm *sm;
	   int j, k, err;

	if (NULL == ecap) {
		log_bug_msg("%s() - event capture engine is not initialised\n", __func__);
		exit(1);
	}

	sm = malloc(sizeof(struct edsm));
	if (NULL == sm) {
		log_ftl_msg("%s() - malloc('edsm'): %s\n", __func__, strerror(errno));
		exit(1);
	}
	memset(sm, 0, sizeof(struct edsm));

	sm->name = __edsm_make_sm_name(t);
	if (NULL == sm->name) {
		log_ftl_msg("%s() - __edsm_make_sm_name() failed\n", __func__);
		exit(1);
	}

	sm->engine = edsm;
	sm->state = sset;	/* initial state */
	sm->ntimers = md->ntimers;
	sm->nintrs = md->nsignals;
	sm->running = 0;

	/* fill transition/action matrix */
	for (j = 0; j < md->nstates; j++) {
		struct state *s = &sset[j];
		for (k = 0; k < s->nrefl; k++) {
			struct reflex *r = &s->reflex[k];
			int row = r->ecode >> 16;
			int col = r->ecode & 0xFFFF;
			if (row > 4) {
				log_ops_msg("%s() - row > 4\n", __func__);
				exit(1);
			}
			if (col > 11) {
				log_ops_msg("%s() - col > 11\n", __func__);
				exit(1);
			}
			s->reflex_matrix[row][col] = r;
		}
	}

	sm->timers = NULL;
	if (sm->ntimers) {
		sm->timers = malloc(sm->ntimers*sizeof(struct channel*));
		if (NULL == sm->timers) {
			log_ftl_msg("%s() - malloc('sm->timers'): %s\n", __func__, strerror(errno));
			exit(1);
		}
		for (k = 0; k < sm->ntimers; k++) {
			int i = md->timers[k].interval_ms;
			err =  __edsm_new_timer(ecap, sm, i, k);
			if (err) {
				log_ftl_msg("%s() - __edsm_new_timer() failed\n", __func__);
				exit(1);
			}
		}
	}

	sm->intrs = NULL;
	if (sm->nintrs) {
		sm->intrs = malloc(sm->nintrs*sizeof(struct channel*));
		if (NULL == sm->intrs) {
			log_ftl_msg("%s() - malloc('sm->intrs'): %s\n", __func__, strerror(errno));
			exit(1);
		}
		for (k = 0; k < sm->nintrs; k++) {
			int signum = md->signals[k].signum;
			err = __edsm_new_interrupt(ecap, sm, signum, k);
			if (err) {
				log_ftl_msg("%s() - __edsm_new_interrupt() failed\n", __func__);
				exit(1);
			}
		}
	}

	if (md->has_data_channel) {
		/* application must use edsm->io_attach(sm,fd) */
		err = __edsm_new_iochan(ecap, sm);
		if (err) {
			log_ftl_msg("%s() - __edsm_new_iochan() failed\n", __func__);
			exit(1);
		}
	}

	if (md->has_fsys_channel) {
		err =__edsm_new_fschan(ecap, sm, md->nwatches);
		if (err) {
			log_ftl_msg("%s() - __edsm_new_fschan() failed\n", __func__);
			exit(1);
		}
	}

	return sm;
}

/* call this once right after edsm_create() */
static void edsm__start(struct edsm *sm, void *dptr)
{
	int err;

	if (sm->running) {
		log_bug_msg("%s() - '%s@%p' state machine already running\n", __func__, sm->name, sm);
		exit(1);
	}

	err = sm->state->enter(sm, sm, dptr);
	if (err) {
		log_ftl_msg("%s() - failed to run '%s' machine\n", __func__, sm->name);
		exit(1);
	}

	sm->running = 1;
}

static void edsm__delete(struct edsm *sm)
{
	int k;

	if (sm->timers) {
		for (k = 0; k < sm->ntimers; k++)
			__edsm_del_channel(ecap, sm, sm->timers[k]);
		free(sm->timers);
	}

	if (sm->intrs) {
		for (k = 0; k < sm->nintrs; k++)
			__edsm_del_channel(ecap, sm, sm->intrs[k]);
		free(sm->intrs);
	}

	if (sm->io)
		__edsm_del_channel(ecap, sm, sm->io);
	if (sm->fsys)
		__edsm_del_channel(ecap, sm, sm->fsys);

	if (sm->name)
		free(sm->name);
	free(sm);
}

/*
 * i/o channel
 */
static void edsm__attach_iofd(struct edsm *sm, int fd)
{
	struct channel *ch = sm->io;
	   int err;

	if (unlikely(NULL == ch)) {
		log_bug_msg("%s() - '%s' has no i/o channel\n", __func__, sm->name);
		exit(1);
	}
	if (unlikely(fd < 0)) {
		log_bug_msg("%s() - invalid fd (%d)\n", __func__, fd);
		exit(1);
	}

	err = ecap->add(fd, ch);
	if (err) {
		log_ftl_msg("%s() - ecap->add('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}

	ch->fd = fd;
}

static void edsm__detach_iofd(struct edsm *sm)
{
	struct channel *ch = sm->io;
	   int err, fd;

	if (unlikely(NULL == ch)) {
		log_bug_msg("%s() - '%s' has no i/o channel\n", __func__, sm->name);
		exit(1);
	}

	fd = ch->fd;

	if (unlikely(fd < 0)) {
		log_wrn_msg("%s() - '%s' has no i/o fd attached\n", __func__, sm->name);
		return;
	}

	err = ecap->del(fd);
	if (err) {
		log_ftl_msg("%s() - ecap->del('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}

	ch->fd = -1;
}

static void edsm__io_enable(struct edsm *sm, u32 what, int oneshot)
{
	struct channel *ch = sm->io;
	   int err, fd;

	if (unlikely(NULL == ch)) {
		log_bug_msg("%s() - '%s' has no i/o channel\n", __func__, sm->name);
		exit(1);
	}

	fd = ch->fd;

	if (unlikely(fd < 0)) {
		log_bug_msg("%s() - '%s' i/o channel has no fd attached\n", __func__, sm->name);
		exit(1);
	}

	err = ecap->conf(fd, ch, what, oneshot);
	if (unlikely(err)) {
		log_ftl_msg("%s() - ecap->conf('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}
}

static void edsm__io_disable(struct edsm *sm)
{
	struct channel *ch = sm->io;
	   int err, fd;

	if (unlikely(NULL == ch)) {
		log_bug_msg("%s() - '%s' has no i/o channel\n", __func__, sm->name);
		exit(1);
	}

	fd = ch->fd;

	if (unlikely(fd < 0)) {
		log_bug_msg("%s() - '%s' i/o channel has no fd attached\n", __func__, sm->name);
		exit(1);
	}

	err = ecap->conf(fd, ch, 0, 0);
	if (unlikely(err)) {
		log_ftl_msg("%s() - ecap->conf('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}
}

/*
 *	Timers
 */

static struct channel *__edsm_get_timer(struct edsm *sm, int tn)
{
	if (unlikely(0 == sm->ntimers)) {
		log_bug_msg("%s() - sm '%s' has no timers\n", __func__, sm->name);
		exit(1);
	}
	if (unlikely(tn >= sm->ntimers)) {
		log_bug_msg("%s() - sm '%s' has no timer #%d\n", __func__, sm->name, tn);
		exit(1);
	}
	return sm->timers[tn];
}

static void edsm__set_timer_interval(struct edsm *sm, int tn, int interval)
{
	struct channel *ch;
	struct tick_info *ti;

	ch = __edsm_get_timer(sm, tn);
	ti = ch->data;
	ti->interval_ms = interval;
}

static void edsm__start_timer(struct edsm *sm, int tn, int oneshot)
{
	struct channel *ch;
	struct tick_info *ti;
	   int fd, err;

	ch = __edsm_get_timer(sm, tn);
	fd = ch->fd;
	ti = ch->data;

	err = ecap->arm(fd, ti->interval_ms, oneshot);
	if (err) {
		log_ftl_msg("%s() - ecap->arm('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}
}

static void edsm__stop_timer(struct edsm *sm, int tn)
{
	struct channel *ch;
	   int fd, err;

	ch = __edsm_get_timer(sm, tn);
	fd = ch->fd;

	err = ecap->disarm(fd);
	if (err) {
		log_ftl_msg("%s() - ecap->disarm('%s',%d) failed\n", __func__, sm->name, fd);
		exit(1);
	}
}

/*
 * file system monitoring
 */
static void edsm__new_fsys_watch(struct edsm *sm, char *pathname, u32 fsys_event_mask, void *data, int dsize)
{
	struct channel *ch = sm->fsys;
	struct fsys_info *fi;
	struct fsys_watch *fsw;
	   int wd, err;

	if (NULL == ch) {
		log_bug_msg("%s() - '%s' has no fsys channel\n", __func__, sm->name);
		exit(1);
	}

	fi = ch->data;

	err = ecap->add_fsys_watch(ch->fd, pathname, fsys_event_mask, &wd);
	if (err) {
		log_ftl_msg("%s() - ecap->inowd('%s',%d) failed\n", __func__, sm->name, ch->fd);
		exit(1);
	}

	if (wd >= fi->nwatches) {
		struct fsys_watch *tmp;
		   int n = 32 + wd;

		tmp = realloc(fi->watch, n * sizeof(struct fsys_watch));
		if (NULL == tmp) {
			log_ftl_msg("%s() - realloc('%d watches'): %s\n", __func__, n, strerror(errno));
			exit(1);
		}
		fi->watch = tmp;
		fi->nwatches = n;
	}

	fsw = &fi->watch[wd];

	/* _copy_ pathname */
	fsw->pathname = malloc(1 + strlen(pathname));
	if (NULL == fsw->pathname) {
		log_ftl_msg("%s() - malloc('pathname'): %s\n", __func__, strerror(errno));
		exit(1);
	}
	strcpy(fsw->pathname, pathname);

	/* copy user data if any */
	if (data && dsize) {
		fsw->info = malloc(dsize);
		if (NULL == fsw->info) {
			log_ftl_msg("%s() - malloc('data'): %s\n", __func__, strerror(errno));
			exit(1);
		}
		memcpy(fsw->info, data, dsize);
	}

	log_inf_msg("%s() - fsys watch for '%s' added (wd #%d)\n", __func__, pathname, wd);
}

static void edsm__del_fsys_watch(struct edsm *sm, int wd)
{
	struct channel *ch = sm->fsys;
	struct fsys_info *fi;
	   int err;

	if (NULL == ch) {
		log_bug_msg("%s() - '%s' has no fsys channel\n", __func__, sm->name);
		exit(1);
	}

	fi = ch->data;

	err = ecap->del_fsys_watch(ch->fd, wd);
	if (err) {
		log_ftl_msg("%s() - ecap->del_fsys_watch(%d,%d) failed\n", __func__, ch->fd, wd);
		exit(1);
	}

	log_inf_msg("%s() - fsys watch for '%s' (wd #%d) removed\n", __func__, fi->watch[wd].pathname, wd);
	if (fi->watch[wd].pathname) {
		free(fi->watch[wd].pathname);
		fi->watch[wd].pathname = NULL;
	}
}

static void edsm__put_msg(struct edsm *src, struct edsm *dst, void *ptr, u32 code)
{
	struct message msg = {0};

	msg.src = src;
	msg.dst = dst;
	msg.ptr = ptr;
	msg.u64 = code;
	mb->put(&msg);
}
