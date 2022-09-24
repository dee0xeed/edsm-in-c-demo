
/* Expandable circular message buffer */

#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H

#include "numeric-types.h"
#include "api.h"

struct message {
	void *src;
	void *dst;
	void *ptr;
	 u64 u64;
};

struct message_buffer {
	struct message *msg;
	   u32 capacity;
	   u32 number;
	   u32 wr_index;
	   u32 rd_index;
	   u32 index_mask;
};

/* _MUST_ be an integer power of 2 */
#define MB_INIT_CAPACITY 128
//#define MB_INIT_CAPACITY 2
//#define MB_INIT_CAPACITY 1024

#define mb_empty(mb) (0 == mb->number)
#define mb_full(mb) (mb->number == mb->capacity)

api_declaration(message_buffer_api) {
	   int (*init)(u32 capacity);
	  void (*fini)(void);
	   int (*get)(struct message *msg);
	  void (*put)(struct message *msg);
};

#endif
