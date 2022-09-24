
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "api.h"
#include "prefix-tree-api.h"
#include "macro-magic.h"

struct key_val {
	char key[32];
	 int val;
};

void print(void *data)
{
	struct key_val *kv = data;
	printf("KEY: '%s'; VAL: %d\n", kv->key, kv->val);
}

void nothing(void *data)
{
}

void delete(void *data)
{
	struct key_val *kv = data;
	printf("DATA for KEY '%s' DELETED\n", kv->key);
	free(kv);

//	*data = NULL;
//	don't do this - it will result in incorrect empty_nodes
//	and, after all, the node is being DELETED, no sense to set it's fields
}

int main(void)
{
	struct pt_api *pt;
//	 void *root = NULL;
	 trie *root = NULL;
	 void **ref;
	struct pt_info pti;

	   int k = 0, r;
	struct key_val *kv;

	pt = get_api("pt_api");
	if (NULL == pt) {
		printf("OPS: %s - get_api('pt_api') failed\n", __func__);
		return 1;
	}

	for (;;) {

		kv = malloc(sizeof(struct key_val));
		kv->val = ++k;

		r = scanf("%s", kv->key);

		if ((0 == r) || (0 == strlen(kv->key)))
			break;

//		printf("Adding '%s'... ", kv->key);
		ref = pt->add(kv->key, &root);
		if (NULL == ref) {
//			printf("FAILED\n");
		} else {
//			printf("OK, ref = %p\n", ref);
			//*ref = kv;
			pt->set(ref, kv, &root);
//			printf("OK, *ref = %p\n", *ref);
		}

//		printf("Getting '%s'... ", kv->key);
		ref = pt->get(kv->key, &root);
		if (NULL == ref) {
//			printf("failure\n");
		} else {
			kv = *ref;
//			printf("success, val is %d\n", kv->val);
		}
	}

/*
   /usr/share/dict/american-english ~100000 words
   227979 nodes / 227978 edges total (12766808 bytes); 128808 empty nodes
*/

//	for (k = 0; k < 10000; k++)
//		pt->iter(&root, nothing);

/* iterative (10000 times nothing)
    real    1m50.242s
    user    1m49.248s
    sys     0m0.428s
*/

	pt->walk(&root, print);

//	for (k = 0; k < 2000; k++)
//		pt->walk(&root, nothing);


/* recursive (10000 times nothing)
    real    1m0.336s
    user    0m59.188s
    sys     0m0.608s
*/

	pt->info(&root, &pti);
	printf("%d nodes total (%d bytes); %d empty nodes \n",
		pti.nodes_total, pti.bytes_total, pti.nodes_empty);

	printf(":: nedges distribution ::\n");
	for (k = 0; k < 20; k++) {
		printf("%.3d : %d\n", k, pti.edges_dist[k]);
	}

// ***
//	pt->del("123", &root, delete);
//	pt->iter(&root, print);

// ***
	printf("now will trim...\n");
	fflush(stdout);
	pt->trim(&root, delete);
	pt->info(&root, &pti);
	printf("%d nodes total (%d bytes); %d empty nodes \n",
		pti.nodes_total, pti.bytes_total, pti.nodes_empty);
	pt->walk(&root, print);

// ***
//	printf("now will strip...\n");
//	pt->walk(&root, delete);
//	pt->info(&root, &pti);
//	printf("%d nodes total (%d bytes); %d empty nodes \n",
//		pti.nodes_total, pti.bytes_total, pti.nodes_empty);
//	pt->walk(&root, print);

// ***
//	printf("now will nuke the trie...\n");
//	pt->nuke(&root, delete);
//	printf("done\n");
//	pt->walk(&root, print);
//	printf("walk done\n");

	return 0;
}
