
#include <stdio.h>
#include "edsm.h"

struct edsm_template mt[] = {
	[0] = {
		.source = "test.smd",
		.desc = NULL,
		.state_set = NULL,
		.expected_instances = 1,
		.instance_seqn = 1,
	},
};

struct edsm_api *engine;
struct edsm *m;

int main(void)
{
	int stop;

	printf("EDSM-G2 test: signals, timers, file system monitoring\n");

	engine = edsm_init(".", mt, 1);
	m = engine->new(&mt[0]);
	engine->run(m, NULL);

	for (;;) {
		stop = engine->exec();
		if (stop)
			break;
		engine->wait();
	}

	return 0;
}
