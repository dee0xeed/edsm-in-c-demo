

/*
	State Machine Description
*/

#ifndef STATE_MACHINE_DESC_H
#define STATE_MACHINE_DESC_H

#include "engine/channel.h"

struct timer_desc;
struct signal_desc;
struct state_desc;
struct trans_desc;
struct action_desc;
struct edsm_desc;

struct state;
struct reflex;

struct edsm;
struct edsm_template;

typedef int (*action)(struct edsm *msg_src, struct edsm *msg_dst, void *msg_data);

struct timer_desc {
	int interval_ms;
	int seqn;
};

struct signal_desc {
	int signum;
	int seqn;
};

struct state_desc {
	  char *name;

	  char *enter_func_name;
	  char *leave_func_name;

	  void *enter_func_addr;
	  void *leave_func_addr;

	struct trans_desc *trans;
	   int ntrans;

	struct action_desc *actions;
	   int nactions;
};

struct trans_desc {
	   u32 ecode;
	struct state_desc *next_state;
	   int next_state_number;
};

struct action_desc {
	 u32 ecode;
	char *func_name;
	void *func_addr;
};

struct edsm_desc {
	  char *name;
	   int nch;

	struct timer_desc *timers;
	   int ntimers;

	struct signal_desc *signals;
	   int nsignals;

	struct state_desc *states;
	   int nstates;

	/* number of inotify watches */
	   int nwatches;

	  int has_data_channel;
	  int has_fsys_channel;
};

struct state {
	  char *name;
	action enter;
	action leave;

	/* list of actions/transitions */
	struct reflex *reflex;
	  int nrefl;

	struct reflex *reflex_matrix[5][12];
	/*
	 * row number = channel.type
	 * col number = seqn
	 */
};

struct reflex {

	u32 ecode;
	/*
	 * Event code == (channel_type << 16) | n
	 * For timers, signals and internal events n is the seqn of them
	 * For i/o channel it is POLLIN/POLLOUT/ERROR (0,1,2)
	 * For fsys channel it is IN_ACCESS/..(0,1,2..) (see man inotify)
	 */

	/*
	 * For Moore machine:
	 *  action = NULL,
	 *  next_state = some_OTHER_state
	 *  both state.leave() and state.enter() are always not NULL
	 * For Mealy machine:
	 *  action = some_action (not NULL),
	 *  next_state is always NULL (no transition)
	 */

	action action;
	struct state *next_state;
};

struct edsm_template {
	  char *source;		/* smd file name */
	struct edsm_desc *desc;
	struct state *state_set;
	   u32 expected_instances;	/* estimate of number of machines of this type */
	   u32 instance_seqn;
};

struct edsm {

	struct object_pool *restroom;
	/* Needed when machines are placed in a pool */
	  int flags;

	  char *name;		/* symbolic name of the machine */
	struct edsm_api *engine;
	   int running;		/* edsm->run() has been invoked */

	struct state *state;	/* current state */
	  void *data;

	struct channel **timers;
	   int ntimers;
	struct channel **intrs;
	   int nintrs;
	struct channel *io;
	struct channel *fsys;
};

#endif
