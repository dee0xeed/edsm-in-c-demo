
#ifndef MESSAGE_DISPATHER_H
#define MESSAGE_DISPATHER_H

typedef int (*edsm_read_os_event_data)(struct channel *ch);

int __read_os_event_info(struct channel *ch);
u32 __get_os_event_seqn(struct channel *ch);
int __edsm_dispatch_message(struct message *msg);

#endif
