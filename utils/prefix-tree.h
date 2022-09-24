
#ifndef _PREIFX_TREE_H
#define _PREIFX_TREE_H

#include "prefix-tree-api.h"

struct pt_edge;
struct pt_node;

struct pt_edge {
	   int s;
	struct pt_node *kid;
};

struct pt_node {
	struct pt_node *parent;		/* needed only for deletion */
	struct pt_edge *edge;		/* resizable array */
	   int nedges;			/* number of kids */
	  void *data;			/* ptr to arb. data */
};

/* api functions */
static void **pt_get(char *key, void **root);
static void **pt_add(char *key, void **root);
static void   pt_set(void **ref, void *data, void **root);
static void   pt_del(char *key, void **root, pt_visit dtor);

static void pt_walk(void **root, pt_visit visit);
static void pt_stat(void **root, struct pt_info *pti);
static void pt_trim(void **root, pt_visit dtor);
static void pt_nuke(void **root, pt_visit dtor);

api_definition(pt_api, pt_api) {

	.get = pt_get,
	.add = pt_add,
	.set = pt_set,
	.del = pt_del,

	.walk = pt_walk,
	.info = pt_stat,
	.trim = pt_trim,
	.nuke = pt_nuke,
};

/* functions for internal use; do not call directly */
static struct pt_edge *__pt_add_edge(struct pt_node *n);
static struct pt_node *__pt_new_node(struct pt_node *parent);
static           void  __pt_del_node(struct pt_node *n, struct pt_info *pti);
static struct pt_node *__pt_search_node(char *key, struct pt_node *root);
static struct pt_node *__pt_insert_node(char *key, struct pt_node *root);
static          void **__pt_get(char *key, void **root, int add);
static          void   __pt_cut_branch(struct pt_node *tn, struct pt_node *root);
static          void   __pt_trim(struct pt_node *root, pt_visit dtor, struct pt_info *pti);
static          void   __pt_walk(struct pt_node *tn, pt_visit visit);

#endif
