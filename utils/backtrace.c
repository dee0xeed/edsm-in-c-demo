
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <string.h>
#include <errno.h>

#include "backtrace.h"
#include "logging.h"

#define BT_DEFAULT_DEPTH 32
int bt_depth = BT_DEFAULT_DEPTH;
void *bt_buff = NULL;

void bt_print_call_stack(void)
{
	char **bt_strings;
	int n, j;

	n = backtrace(bt_buff, bt_depth);
	bt_strings = backtrace_symbols(bt_buff, n);
	if (!bt_strings) {
		log_err_msg("%s() - backtrace_symbols(): %s\n", __func__, strerror(errno));
		exit(1); /* ? */
	}

	log_inf_msg("backtrace (most recent call first):\n");
	for (j = 0; j < n; j++)
		log_inf_msg("%d: %s\n", j, bt_strings[j]);
	free(bt_strings);
}

void sigsegv_handler(int signo, siginfo_t *sinfo, void *xinfo)
{
	log_ops_msg("%s() - we have crashed (got signal #%d)\n", __func__, signo);
	bt_print_call_stack();
	exit(1); /* ? */
}

/* 0 - success, 1 - failure */
int bt_init(int depth)
{
	struct sigaction sa;
	int r;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigsegv_handler;

	r = sigaction(SIGSEGV, &sa, NULL);
	if (-1 == r) {
		log_sys_msg("%s() - sigaction(SIGSEGV): %s\n", __func__, strerror(errno));
		return 1;
	}

	r = sigaction(SIGBUS, &sa, NULL);
	if (-1 == r) {
		log_sys_msg("%s() - sigaction(SIGSEGV): %s\n", __func__, strerror(errno));
		return 1;
	}

	r = sigaction(SIGILL, &sa, NULL);
	if (-1 == r) {
		log_sys_msg("%s() - sigaction(SIGSEGV): %s\n", __func__, strerror(errno));
		return 1;
	}

	if (depth <= 0) {
		log_dbg_msg("%s() - trace depth defaults to %d\n", __func__, bt_depth);	} else {
		bt_depth = depth;
	}

	bt_buff = malloc(bt_depth*sizeof(void*));
	if (!bt_buff) {
		log_sys_msg("%s() - can not allocate bt_buff: %s\n", __func__, strerror(errno));
		return 1;
	}
	return 0;
}
