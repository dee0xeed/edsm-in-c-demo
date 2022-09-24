
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "array-api.h"
#include "macro-magic.h"

struct test {
	int a;
	int b;
	int c;
};

void print(void *data)
{
	struct test *p;
	p = (struct test*)data;
	printf("a: %d; b: %d; c: %d\n", p->a, p->b, p->c);
}

int main(void)
{
	struct array_api *darr;
	struct array *da;
	   int k;

	darr = get_api("array_api");
	if (NULL == darr) {
		printf("OPS: %s - get_api('array_api') failed\n", __func__);
		return 1;
	}

	da = darr->new(sizeof(struct test), 5);
	if (NULL == da) {
		printf("ERR: %s - darr->new() failed\n", __func__);
		return 1;
	}

	for (k = 0; k < 30; k++) {

		struct test t;
		t.a = k;
		t.b = k * 2;
		t.c = k * 3;

		darr->add(da, &t);
	}

	darr->walk(da, print);
	darr->reset(da);
	darr->walk(da, print);
	return 0;
}
