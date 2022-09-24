
/* prefix tree (aka 'trie') */

#ifndef PREFIX_TREE_API_H
#define PREFIX_TREE_API_H

#include "numeric-types.h"
#include "api.h"

typedef void (*pt_visit)(void *data);
typedef void trie;

struct pt_info {
	u32 nodes_total;
	u32 nodes_empty;
	u32 bytes_total;
	u32 edges_dist[256];
};

api_declaration(pt_api) {

	void **(*get)(char *key, void **root);
	void **(*add)(char *key, void **root);
	void   (*set)(void **ref, void *data, void **root);
	void   (*del)(char *key, void **root, pt_visit dtor);

	void  (*walk)(void **root, pt_visit visit);
	void  (*info)(void **root, struct pt_info *pti);
	void  (*trim)(void **root, pt_visit dtor);
	void  (*nuke)(void **root, pt_visit dtor);
};

#endif
