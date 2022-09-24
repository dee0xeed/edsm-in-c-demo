
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "compiler.h"
#include "state-machines.h"
#include "tcp-sockets.h"
#include "logging.h"

int listener_init_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct listener_info *li = dptr;
	   int sk, err;

	err = new_l_sock(NULL, li->port, &sk);
	if (err) {
		log_ftl_msg("%s() - new_l_sock(%d) failed\n", __func__, li->port);
		exit(1);
	}

	li->l_sock = sk;
	me->data = li;
	me->engine->msg(me, me, NULL, M0_WORK);
	return 0;
}

int listener_init_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	return 0;
}

int listener_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	struct listener_info *li = me->data;
	   int sk = li->l_sock;

	me->io->flags = 1; // engine/__channel.h:#define EEF_NOFIONREAD       (1 << 0)
	me->engine->io_attach(me, sk);
	me->engine->io_enable(me, EDSM_POLLIN, IO_FOREVER);
	// тут можно и без enable, но тогда у attach нужен ещё параметр с маской событий...
	return 0;
}

/* message from OS, new connection */
int listener_work_D0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct listener_info *li = me->data;
	struct client_info *ci = NULL;
	   int sk = -1, err;
	  char a[32];

	ci = malloc(sizeof(struct client_info));
	err = new_d_sock(me->io->fd, &sk, a);

	if (NULL == ci) {
		log_err_msg("%s() - malloc(): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	if (err) {
		log_err_msg("%s() - new_d_sock() failed\n", __func__);
		goto __failure;
	}

	ci->d_sock = sk;
	strcpy(ci->addr, a);
	me->engine->msg(me, li->manager, ci, M0_MEET);
	log_dbg_msg("%s() - new connection from %s (fd = %d)\n", __func__, ci->addr, sk);
	return 0;

      __failure:
	if (-1 != sk) {
		log_err_msg("%s() - failed to accept connection from %s (fd = %d)\n", __func__, ci->addr, sk);
		close(sk);
	}
	if (ci)
		free(ci);
	return 0;
}

/* message from manager - client gone */
int listener_work_M0(struct edsm *src, struct edsm *me, void *dptr)
{
	struct client_info *ci = dptr;
	   int r;

	log_dbg_msg("%s() - '%s' gone\n", __func__, ci->addr);

	r = close(ci->d_sock);
	if (-1 == r) {
		log_sys_msg("%s() - close(%d): %s\n", __func__,ci->d_sock, strerror(errno));
		exit(1);
	}
	free(ci);

	return 0;
}

/* message from manager, we are terminating */
int listener_work_M1(struct edsm *src, struct edsm *me, void *dptr)
{
	/* stop accepting clients */
	me->engine->io_disable(me);
	return 0;
}

int listener_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	/* do nothing */
	return 0;
}
