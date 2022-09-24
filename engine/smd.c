
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include "__smd.h"
#include "__channel.h"
#include "strings.h"
#include "logging.h"

char *_instruction(char c)
{
	return index("TSF$+@", c);
}

char *valid_channel_type(char c)
{
	return index("TSDMF", c);
}

struct timer_desc *_new_timer(struct timer_desc *old, int n, char *str, int ln)
{
	struct timer_desc *new, *t;
	int i, r;

	new = realloc(old, n*sizeof(struct timer_desc));
	if (NULL == new) {
		log_ftl_msg("%s() - realloc(): %s\n", __func__, strerror(errno));
		exit(1);
	}

	t = &new[n-1];
	memset(t, 0, sizeof(struct timer_desc));

	r = sscanf(str, "%d", &i);
	if (r != 1) {
		log_ftl_msg("%s() - bad timer interval (%s)\n", __func__, str);
		exit(1);
	}

	t->interval_ms = i;
	t->seqn = n - 1;
	return new;
}

struct signal_desc *_new_signal(struct signal_desc *old, int n, char *str, int ln)
{
	struct signal_desc *new, *s;
	int sn, r;

	new = realloc(old, n*sizeof(struct signal_desc));
	if (NULL == new) {
		log_ftl_msg("%s() - realloc(): %s\n", __func__, strerror(errno));
		exit(1);
	}

	s = &new[n-1];
	memset(s, 0, sizeof(struct signal_desc));

	r = sscanf(str, "%d", &sn);
	if (r != 1) {
		log_ftl_msg("%s() - bad signal number (%s)\n", __func__, str);
		exit(1);
	}

	if (n < 0) {
		log_ftl_msg("%s() - bad signal number (%s)\n", __func__, str);
		exit(1);
	}

	s->signum = sn;
	s->seqn = n - 1;
	return new;
}

int _number_of_watches(char *str, int ln)
{
	int nw = 0, r;

	r = sscanf(str, "%d", &nw);
	if (r != 1) {
		log_ftl_msg("%s() - bad watch number (%s) in line %d\n", __func__, str, ln);
		exit(1);
	}

	return nw;
}

struct state_desc *_new_state(struct state_desc *old, int n, char *str, char *mname)
{
	struct state_desc *new, *sd;
	int len;
	static char buf[64];

	new = realloc(old, n*sizeof(struct state_desc));
	if (NULL == new) {
		log_ftl_msg("%s() - can not alloc state desc: %s\n", __func__, strerror(errno));
		exit(1);
	}
	sd = &new[n-1];
	memset(sd, 0, sizeof(struct state_desc));

	/* name of the state */
	len = strlen(str);
	sd->name = malloc(len + 1);
	memset(sd->name, 0, len + 1);
	strncpy(sd->name, str, len - 1);

	/* name of enter function */
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s_%s_enter", mname, sd->name);
	sd->enter_func_name = malloc(strlen(buf) + 1);
	strcpy(sd->enter_func_name, buf);

	/* name of leave function */
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s_%s_leave", mname, sd->name);
	sd->leave_func_name = malloc(strlen(buf) + 1);
	strcpy(sd->leave_func_name, buf);

	return new;
}

struct state_desc *_find_state_desc(char *sname, struct edsm_desc *md)
{
	struct state_desc *sd = NULL;
	int k;

	for (k = 0; k < md->nstates; k++) {
		int mismatch;
		mismatch = strcmp(sname, md->states[k].name);
		if (!mismatch) {
			sd = &md->states[k];
			break;
		}
	};
	return sd;
}

u32 _compose_ecode(char ct, int seqn)
{
	u32 chan_type = 0;

	switch (ct) {

		case 'T':
			chan_type = CT_TICK;
		break;

		case 'S':
			chan_type = CT_INTR;
		break;

		case 'D':
			chan_type = CT_DATA;
		break;

		case 'F':
			chan_type = CT_FSYS;
		break;

		case 'M':
			chan_type = CT_MQUE;
		break;

		default:
			log_bug_msg("%s() - invalid channel type ('%c')\n", __func__, ct);
	}

	return (chan_type << 16) | seqn;
}

void __set_has_flags(struct edsm_desc *md, char ct)
{
	if ('D' == ct)
		md->has_data_channel = 1;
	if ('F' == ct)
		md->has_fsys_channel = 1;
}

void _new_trans(struct edsm_desc *md, char *str, int ln)
{
	static char state_name[16];
	static char event_name[8];
	static char next_state_name[16];
	        int r;
	       char ct; int n;

	struct state_desc *sd, *nsd;
	struct trans_desc *td;

	memset(state_name, 0, sizeof(state_name));
	memset(event_name, 0, sizeof(event_name));
	memset(next_state_name, 0, sizeof(next_state_name));

	r = sscanf(str, "%s %s %s\n", state_name, event_name, next_state_name);
	if (r != 3) {
		log_err_msg("%s() - invalid trans desc '%s' @line #%d in '%s.smd'\n",
			__func__, trim(str, "\r\n"), ln, md->name);
		exit(1);
	}

	sd = _find_state_desc(state_name, md);
	if (!sd) {
		log_ftl_msg("%s() - unknown state '%s' @line #%d in '%s.smd'\n", __func__, state_name, ln, md->name);
		exit(1);
	}

	nsd = _find_state_desc(next_state_name, md);
	if (!nsd) {
		log_ftl_msg("%s() - unknown state '%s' @line #%d in '%s.smd'\n", __func__, next_state_name, ln, md->name);
		exit(1);
	}

	r = sscanf(event_name, "%c%d", &ct, &n);
	if (r != 2) {
		log_ftl_msg("%s() - bad event desc '%s' @line #%d in '%s.smd'\n", __func__, event_name, ln, md->name);
		exit(1);
	}

	if (!valid_channel_type(ct)) {
		log_ftl_msg("%s() - bad event desc '%s' @line #%d in '%s.smd'\n", __func__, event_name, ln, md->name);
		exit(1);
	}

	__set_has_flags(md, ct);

	sd->ntrans++;
	sd->trans = realloc(sd->trans, sd->ntrans*sizeof(struct trans_desc));
	td = &sd->trans[sd->ntrans - 1];
	td->ecode = _compose_ecode(ct, n);

	td->next_state = nsd;
	td->next_state_number = nsd - md->states;
}

void _new_action(struct edsm_desc *md, char *str, int ln)
{
	static char state_name[64];
	static char event_name[64];
	static char func_name[64];

	struct state_desc *sd;
	struct action_desc *ad;

	int n, r;
	char ct;

	memset(state_name, 0, sizeof(state_name));
	memset(event_name, 0, sizeof(event_name));
	memset(func_name, 0, sizeof(func_name));

	r = sscanf(str, "%s %s\n", state_name, event_name);
	if (r != 2) {
		log_ftl_msg("%s() - bad action desc '%s' @line #%d in '%s.smd'\n", __func__, str, ln, md->name);
		exit(1);
	}
	sprintf(func_name, "%s_%s_%s", md->name, state_name, event_name);

	sd = _find_state_desc(state_name, md);
	if (!sd) {
		log_ftl_msg("%s() - unknown state '%s' @line #%d in '%s.smd'\n", __func__, state_name, ln, md->name);
		exit(1);
	}

	r = sscanf(event_name, "%c%d", &ct, &n);
	if (r != 2) {
		log_ftl_msg("%s() - bad event desc '%s' @line #%d in '%s.smd'\n", __func__, event_name, ln, md->name);
		exit(1);
	}

	if (!valid_channel_type(ct)) {
		log_ftl_msg("%s() - bad event desc '%s' @line #%d in '%s.smd'\n", __func__, event_name, ln, md->name);
		exit(1);
	}

	__set_has_flags(md, ct);

	sd->nactions++;
	sd->actions = realloc(sd->actions, sd->nactions*sizeof(struct action_desc));
	ad = &sd->actions[sd->nactions-1];
	ad->ecode = _compose_ecode(ct, n);

	n = strlen(func_name);
	ad->func_name = malloc(n + 1);
	memset(ad->func_name, 0, n + 1);
	strncpy(ad->func_name, func_name, n);
}

#define BUFSZ 128
char buf[BUFSZ];

void _extract_edsm_name(char *fname, struct edsm_desc *desc)
{
	char *c, *s;
	memset(buf, 0, BUFSZ);
	if (strlen(fname) > BUFSZ-1) {
		log_ftl_msg("%s() - file name '%s' is too long\n", __func__, fname);
		exit(1);
	}

	strcpy(buf, fname);
	c = rindex(buf, '/');
	if (c) {
		*c = 0;
		c++;
	} else {
		c = buf;
	}
	s = index(c, '.');
	if (s)
		*s = 0;
	desc->name = malloc(strlen(c) + 1);
	strcpy(desc->name, c);
}

void _resolv(struct edsm_desc *md)
{
	void *program;
	int k, j;

	program = dlopen(NULL, RTLD_LAZY);
	if (!program) {
		log_ftl_msg("%s() - dlopen('self'): %s\n", __func__, dlerror());
		exit(1);
	}
	
	for (k = 0; k < md->nstates; k++) {
		struct state_desc *sd = &md->states[k];

		/* state.enter is mandatory */
		dlerror();
		sd->enter_func_addr = dlsym(program, sd->enter_func_name);
		if (!sd->enter_func_addr) {
			log_ftl_msg("%s() - function not found: %s\n", __func__, dlerror());
			exit(1);
		}

		/* state.leave is mandatory too */
		dlerror();
		sd->leave_func_addr = dlsym(program, sd->leave_func_name);
		if (!sd->leave_func_addr) {
			log_ftl_msg("%s() - function not found: %s\n", __func__, dlerror());
			exit(1);
		}

		for (j = 0; j < sd->nactions; j++) {
			struct action_desc *ad = &sd->actions[j];
			dlerror();
			ad->func_addr = dlsym(program, ad->func_name);
			if (!ad->func_addr) {
				log_ftl_msg("%s() - function not found: %s\n", __func__, dlerror());
				exit(1);
			}
		}
	}
	dlclose(program);
}

static char smd_file[256];

struct edsm_desc *smd_load_desc(char *dir, char *file)
{
	FILE *text;
	int line_count;
	struct edsm_desc *md;

	struct state_desc *states = NULL;
	struct timer_desc *timers = NULL;
	struct signal_desc *signals = NULL;

	if (NULL == dir) {
		log_bug_msg("%s() - 'dir' is NULL\n", __func__);
		return NULL;
	}

	if (NULL == file) {
		log_bug_msg("%s() - 'file' is NULL\n", __func__);
		return NULL;
	}

	md = malloc(sizeof(struct edsm_desc));
	if (NULL == md) {
		log_sys_msg("%s() - malloc(edsm_desc): %s\n", __func__, strerror(errno));
		return NULL;
	}
	memset(md, 0, sizeof(struct edsm_desc));

	_extract_edsm_name(file, md);

	sprintf(smd_file, "%s/%s", dir, file);

	text = fopen(smd_file, "r");
	if (NULL == text) {
		log_sys_msg("%s() - fopen('%s'): %s\n", __func__, smd_file, strerror(errno));
		return NULL;
	}

	line_count = 0;

	while (!feof(text)) {

		char *line, instr;
		memset(buf, 0, BUFSZ);
		line = fgets(buf, BUFSZ, text);
		if (NULL == line)
			break;
		line_count++;

		if (!rindex(line, '\n')) {
			log_err_msg("%s() - line #%d in '%s' is too long\n", __func__, line_count, file);
			return NULL;
		}

		if (!_instruction(*line))
			continue;

/*
		if ((index(line, ' ')) || (index(line, '\t'))) {
			printf("ERR: space(s) in line #%d in '%s'", line_count, file);
			exit(13);
		}
*/

		instr = *line;
		line++; /* step over instruction symbol */

		switch (instr) {

			case 'T':
				md->ntimers++;
				timers = _new_timer(timers, md->ntimers, line, line_count);
				md->timers = timers;
			break;

			case 'S':
				md->nsignals++;
				signals = _new_signal(signals, md->nsignals, line, line_count);
				md->signals = signals;
			break;

			/* number of watch descriptors for inotify instance */
			case 'F':
				md->nwatches = _number_of_watches(line, line_count);
			break;

			/* state */
			case '$':
				md->nstates++;
				states = _new_state(states, md->nstates, line, md->name);
				md->states = states;
			break;

			/* transition */
			case '+':
				_new_trans(md, line, line_count);
			break;

			/* action */
			case '@':
				_new_action(md, line, line_count);
			break;
		}
	}

	_resolv(md);
	fclose(text);

	log_dbg_msg("%s() - sm '%s' has %d timers, %d signals and %d states\n",
		__func__, md->name, md->ntimers, md->nsignals, md->nstates);
	if (md->has_data_channel)
		log_dbg_msg("%s() - sm '%s' has i/o channel\n", __func__, md->name);
	if (md->has_fsys_channel)
		log_dbg_msg("%s() - sm '%s' has fsys channel\n", __func__, md->name);

	return md;
}

struct state *smd_get_state_set(struct edsm_desc *md)
{
	struct state *sset;
	   int k, j, n;

	n = md->nstates*sizeof(struct state);
	sset = malloc(n);
	if (NULL == sset) {
		log_ftl_msg("%s() - malloc('state set': %s\n", __func__, strerror(errno));
		return NULL;
	}
	memset(sset, 0, n);

	for (k = 0; k < md->nstates; k++) {

		struct state_desc *sd = &md->states[k];
		struct reflex *r;

		sset[k].name = sd->name;
		sset[k].enter = sd->enter_func_addr;
		sset[k].leave = sd->leave_func_addr;

		n = sd->ntrans + sd->nactions;
		sset[k].nrefl = n;
		sset[k].reflex = malloc(n*sizeof(struct reflex));

		for (j = 0; j < sd->ntrans; j++) {

			struct trans_desc *td = &sd->trans[j];
			int nsn = td->next_state_number;
			r = &sset[k].reflex[j];

			r->ecode = td->ecode;
			r->action = NULL;
			r->next_state = &sset[nsn];
		}

		for (j = 0; j < sd->nactions; j++) {

			struct action_desc *ad = &sd->actions[j];
			r = &sset[k].reflex[j+sd->ntrans];

			r->ecode = ad->ecode;
			r->action = ad->func_addr;
			r->next_state = NULL;
		}
	}

	md->nch = 2; /* i/o and fsys channels */
	md->nch += md->ntimers;
	md->nch += md->nsignals;

	return sset;
}
