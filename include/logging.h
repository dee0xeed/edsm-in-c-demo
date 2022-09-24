
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <errno.h>

#include "numeric-types.h"

/* MIL == 'message importance level' */

#define MIL_ALL		0
#define MIL_SM_DBG	1
#define MIL_DBG		2
#define MIL_WRN		3
#define MIL_ERR		4
#define MIL_SYS		5
#define MIL_INF		6
#define MIL_OPS		7
#define MIL_FTL		8
#define MIL_BUG		9
#define MIL_NONE	10

extern int __mil_threshold;
void set_mil_threshold(int t);

#define __log_msg(mil, MIL, ...)			\
do {							\
	if (mil >= __mil_threshold) {			\
		fprintf(stdout, MIL __VA_ARGS__);	\
		fflush(stdout);				\
	}						\
} while(0);

#define log_sm_dbg_msg(...) __log_msg(MIL_SM_DBG, "SM-DBG: ", __VA_ARGS__)

#define log_dbg_msg(...) __log_msg(MIL_DBG, "DBG: ", __VA_ARGS__)
#define log_wrn_msg(...) __log_msg(MIL_WRN, "WRN: ", __VA_ARGS__)
#define log_err_msg(...) __log_msg(MIL_ERR, "ERR: ", __VA_ARGS__)
#define log_sys_msg(...) __log_msg(MIL_SYS, "SYS: ", __VA_ARGS__)
#define log_inf_msg(...) __log_msg(MIL_INF, "INF: ", __VA_ARGS__)
#define log_ops_msg(...) __log_msg(MIL_OPS, "OPS: ", __VA_ARGS__)
#define log_ftl_msg(...) __log_msg(MIL_FTL, "FTL: ", __VA_ARGS__)
#define log_bug_msg(...) __log_msg(MIL_BUG, "BUG: ", __VA_ARGS__)

void hexdump(u8 *b, int l);

#endif
