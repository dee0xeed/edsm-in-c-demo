
#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include "numeric-types.h"

struct dbuf {
	 u8 *loc;
	 u8 *pos;
	u32 len;
	u32 cnt;
	u32 cnt_init;	/* used as cnt storage for TX machines */
	u32 lr_cnt;	/* current read() result. Fuck OMNICOMM! */
	u32 maxlen;
};

#endif
