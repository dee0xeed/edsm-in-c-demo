
/*
  F0  #define IN_ACCESS		0x00000001	// File was accessed
  F1  #define IN_MODIFY		0x00000002	// File was modified
  F2  #define IN_ATTRIB		0x00000004	// Metadata changed
  F3  #define IN_CLOSE_WRITE	0x00000008	// Writtable file was closed
  F4  #define IN_CLOSE_NOWRITE	0x00000010	// Unwrittable file closed
  F5  #define IN_OPEN		0x00000020	// File was opened
  F6  #define IN_MOVED_FROM	0x00000040	// File was moved from X
  F7  #define IN_MOVED_TO	0x00000080	// File was moved to Y
  F8  #define IN_CREATE		0x00000100	// Subfile was created
  F9  #define IN_DELETE		0x00000200	// Subfile was deleted
 F10 #define IN_DELETE_SELF	0x00000400	// Self was deleted
 F11 #define IN_MOVE_SELF	0x00000800	// Self was moved
*/

#include <stdio.h>
#include "edsm.h"

#define INOMASK 		\
	IN_MOVED_TO      |	\
	IN_MOVED_FROM    |	\
	IN_CLOSE_WRITE

int test_work_enter(struct edsm *src, struct edsm *me, void *dptr)
{
	printf("%s - Hello!\n", __func__);
	me->engine->tm_start(me, 0, TM_PERIODIC);
	me->engine->tm_start(me, 1, TM_ONESHOT);
	me->engine->fs_new_watch(me, ".", INOMASK, NULL, 0);
	return 0;
}

int test_work_leave(struct edsm *src, struct edsm *me, void *dptr)
{
	printf("%s - Goodby!\n", __func__);
	me->engine->tm_stop(me, 0);
	return 0;
}

int test_work_S0(struct edsm *src, struct edsm *me, void *dptr)
{
	printf("%s - Interrupted\n", __func__);
	return 1;
}

int test_work_S1(struct edsm *src, struct edsm *me, void *dptr)
{
	printf("%s - Terminated\n", __func__);
	return 1;
}

int test_work_T0(struct edsm *src, struct edsm *me, void *dptr)
{
	static int cnt = 1;
	if (cnt & 1) {
		printf("%s - Tick\n", __func__);
	} else {
		printf("%s - Tack\n", __func__);
	}
	cnt++;
	return 0;
}

int test_work_T1(struct edsm *src, struct edsm *me, void *dptr)
{
	printf("%s - Time is over\n", __func__);
	return 1;
}

int test_work_F3(struct edsm *src, struct edsm *me, void *dptr)
{
	struct channel *ch = me->fsys;
	struct fsys_info *fi = ch->data;
//	struct inotify_event *fsev = fi->fsys_event;
//	struct fsys_watch *fsw = &fi->watch[fsev->wd];
//	struct fsys_watch_info *fswi = fsw->info;

	printf("%s - '%s' was closed (WRITE)\n", __func__, fi->filename);
	return 0;
}

int test_work_F6(struct edsm *src, struct edsm *me, void *dptr)
{
	struct channel *ch = me->fsys;
	struct fsys_info *fi = ch->data;
	struct inotify_event *fsev = fi->fsys_event;
	struct fsys_watch *fsw = &fi->watch[fsev->wd];
//	struct fsys_watch_info *fswi = fsw->info;

	printf("%s - '%s' was moved from '%s'\n", __func__, fi->filename, fsw->pathname);
	return 0;
}

int test_work_F7(struct edsm *src, struct edsm *me, void *dptr)
{
	struct channel *ch = me->fsys;
	struct fsys_info *fi = ch->data;
	struct inotify_event *fsev = fi->fsys_event;
	struct fsys_watch *fsw = &fi->watch[fsev->wd];
//	struct fsys_watch_info *fswi = fsw->info;

	printf("%s - '%s' was moved to '%s'\n", __func__, fi->filename, fsw->pathname);
	return 0;
}
