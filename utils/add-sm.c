
#include <stdio.h>
#include <stdlib.h>

#include "add-sm.h"
#include "logging.h"

void add_sm(struct edsm_api *e, struct edsm_template *t, struct object_pool *p, void *arg)
{
	struct edsm *m;

	m = e->new(t);
	if (NULL == m) {
		log_err_msg("%s('%s') - failed to create new machine\n", __func__, t->source);
		return;
	}

	m->restroom = p;
	e->run(m, arg);
}
