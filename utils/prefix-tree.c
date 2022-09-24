
/*
 * Adaptive Prefix Tree implementation
 * 'adaptive' means that edges are stored in dynamic arrays, nothing more
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "compiler.h"
#include "mempool-api.h"
#include "prefix-tree.h"
#include "logging.h"

static struct mempool *__pt_mempool = NULL;
static struct mempool_api *mp = NULL;

__attribute__((constructor)) static void pt_init(void)
{
	mp = get_api("mpool_api");
	if (NULL == mp) {
		log_ops_msg("%s() - failed to get API for memory pool\n", __func__);
		exit(1);
	}
	__pt_mempool = mp->new("prefix-tree-nodes-mempool", sizeof(struct pt_node));
	if (NULL == __pt_mempool) {
		log_ftl_msg("%s() - failed to create memory pool\n", __func__);
		exit(1);
	}
}

static struct pt_edge *__pt_add_edge(struct pt_node *n)
{
	struct pt_edge *e = NULL;
	   int ne;

	ne = n->nedges + 1;
	e = realloc(n->edge, ne * sizeof(struct pt_edge));
	if (unlikely(NULL == e)) {
		log_sys_msg("%s() - realloc(): %s\n", __func__, strerror(errno));
		return NULL;
	}
	n->edge = e;
	return &n->edge[n->nedges];
}

static struct pt_node *__pt_new_node(struct pt_node *parent)
{
	struct pt_node *n = NULL;

	n = mp->get(__pt_mempool);
	if (unlikely(NULL == n)) {
		log_err_msg("%s() - mempool_get_block() failed\n", __func__);
		return NULL;
	}
	n->parent = NULL;
	n->edge = NULL;
	n->nedges = 0;
	n->data = NULL;

	n->parent = parent;
	return n;
}

static void  __pt_del_node(struct pt_node *n, struct pt_info *pti)
{
	struct pt_node *p;
	struct pt_edge *e;
	   int kn, k;

	p = n->parent;
	if (unlikely(NULL == p))
		goto __free_node;

	/* get kid number */
	for (kn = 0; kn < p->nedges; kn++)
		if (n == p->edge[kn].kid)
			break;

	p->nedges--;

	/* remove the gap */
	for (k = kn; k < p->nedges; k++)
		p->edge[k] = p->edge[k+1];

	e = realloc(p->edge, p->nedges*sizeof(struct pt_edge));
	if (unlikely((NULL == e) && (0 != p->nedges))) {
		log_ftl_msg("%s() - realloc(%d->%d) failed: %s\n",
			__func__, p->nedges + 1, p->nedges, strerror(errno));
		exit(1);
	}
	p->edge = e;

      __free_node:

	pti->nodes_total--;
	if (NULL == n->data)
		pti->nodes_empty--;

	pti->bytes_total -= __pt_mempool->block_size;
	pti->bytes_total -= sizeof(struct pt_edge);
	mp->put(__pt_mempool, n);
}

/*
 * See:
 * https://dirtyhandscoding.wordpress.com/2017/08/25/performance-comparison-linear-search-vs-binary-search/
 * https://schani.wordpress.com/2010/04/30/linear-vs-binary-search/

for /usr/share/dict/american-english (~100000 words)

227979 nodes / 227978 edges total (12766808 bytes); 128808 empty nodes 

:: nedges distribution ::

    0      1     2    3    4    5   6   7   8   9  10 ... 22 ...
67480 123210 24418 6432 3104 1407 646 359 235 145 124 ... 11 ...

i.e. 99.8% of nodes have <= 10 edges >>> no need in binary search

*/

static int __pt_get_edge_index(struct pt_node *n, int s)
{
	int k, i = -1;

	for (k = 0; k < n->nedges; k++) {
		if (n->edge[k].s == s) {
			i = k;
			break;
		}
	}
	return i;
}

static struct pt_node *__pt_search_node(char *key, struct pt_node *root)
{
	struct pt_node *n = root;
	  char *c;
	   int i;

	for (c = key; *c; c++) {
		i = __pt_get_edge_index(n, *c);
		if (-1 == i) {
			n = NULL;
			break;
		}
		n = n->edge[i].kid;
	}
	return n;
}

static int __pt_add_kid(struct pt_node *n, int s, struct pt_info *pti)
{
	struct pt_node *kid;
	struct pt_edge *e, new_edge;
	int k;

	kid = __pt_new_node(n);
	if (unlikely(NULL == kid)) {
		log_err_msg("%s() - __pt_new_node() failed\n", __func__);
		goto __failure;
	}

	e = __pt_add_edge(n);
	if (unlikely(NULL == e)) {
		log_err_msg("%s() - __pt_add_edge() failed\n", __func__);
		goto __failure;
	}

	e->s = s;
	e->kid = kid;

	pti->edges_dist[0]++;

	n->nedges++;

	/* update statistics */
	pti->nodes_total++;
	pti->nodes_empty++;
	pti->edges_dist[n->nedges - 1]--;
	pti->edges_dist[n->nedges]++;
	pti->bytes_total += __pt_mempool->block_size;
	pti->bytes_total += sizeof(struct pt_edge);

	/* keep the edges sorted */
	k = n->nedges - 1;
	new_edge = n->edge[k];
	k--;	/* the one before last */
	while ((k >= 0) && (new_edge.s < n->edge[k].s)) {
		n->edge[k+1] = n->edge[k];
		k--;
	}
	k++;
	n->edge[k] = new_edge;

	return k;

      __failure:

	if (kid)
		mp->put(__pt_mempool, kid);
	if (e) {
		e = realloc(n->edge, n->nedges * sizeof(struct pt_edge));
		if (NULL == e) {
			log_ftl_msg("%s() - realloc(%d->%d): %s\n", __func__,
				n->nedges + 1, n->nedges, strerror(errno));
			exit(1);
		}
		n->edge = e;
	}
	return -1;
}

static struct pt_node *__pt_insert_node(char *key, struct pt_node *root)
{
	struct pt_node *n = root;
	struct pt_info *pti = root->data;
	  char *c;
	   int i;

	for (c = key; *c; c++) {

		i = __pt_get_edge_index(n, *c);

		if (-1 == i) {
			i = __pt_add_kid(n, *c, pti);
			if (-1 == i) {
				log_err_msg("%s() - __pt_add_kid() failed\n", __func__);
				return NULL;
			}
		}
		n = n->edge[i].kid;
	}
	return n;
}

static void __pt_cut_branch(struct pt_node *tn, struct pt_node *root)
{
	struct pt_node *pn;
	struct pt_info *pti = root->data;

	while ((0 == tn->nedges) && (NULL == tn->data)) {

		pn = tn->parent;
		__pt_del_node(tn, pti);
		tn = pn;
		if (tn == root)
			break;
	}
}

/* stub the tree (delete everything exept root) */
static void __pt_trim(struct pt_node *root, pt_visit dtor, struct pt_info *pti)
{
	struct pt_node *n;

	while (root->nedges) {

		n = root->edge->kid;
		__pt_trim(n, dtor, pti);

		if (n->data)
			dtor(n->data);
		__pt_del_node(n, pti);
	}
}

static struct pt_node *__pt_create_root(void)
{
	struct pt_info *pti;
	struct pt_node *r;

	pti = malloc(sizeof(struct pt_info));
	if (NULL == pti) {
		log_sys_msg("%s() - malloc(): %s\n", __func__, strerror(errno));
		return NULL;
	}

	r = __pt_new_node(NULL);
	if (NULL == r) {
		log_err_msg("%s() - __pt_new_node() failed\n", __func__);
		return NULL;
	}

	r->data = pti;
	pti->edges_dist[0]++;
	return r;
}

static void **__pt_get(char *key, void **root, int add)
{
	struct pt_node *tn;

	if (unlikely(NULL == key))
		return NULL;
	if (unlikely(NULL == root))
		return NULL;

	tn = *root;
	if (unlikely(NULL == tn)) {
		tn = __pt_create_root();
		if (unlikely(NULL == tn))
			return NULL;
		*root = tn;
	}

	if (add)
		tn = __pt_insert_node(key, *root);
	else
		tn = __pt_search_node(key, *root);
	return tn ? &tn->data : NULL;
}

static void **pt_get(char *key, void **root)
{
	return __pt_get(key, root, 0);
}

static void pt_set(void **ref, void *new_data, void **root)
{
	struct pt_node *r;
	struct pt_info *pti;
	  void *old_data;

	if (ref && root && *root) {

		old_data = *ref;
		r = *root;
		pti = r->data;

		if ((NULL == old_data) && (NULL != new_data))
			pti->nodes_empty--;
		if ((NULL != old_data) && (NULL == new_data))
			pti->nodes_empty++;
		*ref = new_data;
	}
}

static void **pt_add(char *key, void **root)
{
	return __pt_get(key, root, 1);
}

static void pt_del(char *key, void **root, pt_visit dtor)
{
	struct pt_node *tn;

	if (unlikely(NULL == key))
		return;
	if (unlikely(NULL == root))
		return;
	if (unlikely(NULL == *root))
		return;
	if (unlikely(NULL == dtor))
		return;

	tn = __pt_search_node(key, *root);
	if (NULL == tn)
		return;

	if (tn->data)
		dtor(tn->data);

	__pt_cut_branch(tn, *root);
}

static void __pt_walk(struct pt_node *n, pt_visit visit)
{
	int k;

	if (n->data)
		visit(n->data);
	for (k = 0; k < n->nedges; k++)
		__pt_walk(n->edge[k].kid, visit);
}

static void pt_walk(void **root, pt_visit visit)
{
	struct pt_node *r;
	  void *d;

	if (visit && root && *root) {
		r = *root;
		d = r->data;
		r->data = NULL;
		__pt_walk(*root, visit);
		r->data = d;
	}
}

static void pt_stat(void **root, struct pt_info *pti)
{
	struct pt_node *r;
	struct pt_info *i;

	if (unlikely((NULL == pti) || (NULL == root)))
		return;

	r = *root;
	i = r->data;
	memcpy(pti, i, sizeof(struct pt_info));
}

static void pt_trim(void **root, pt_visit dtor)
{
	struct pt_info *pti;
	struct pt_node *n, *p, *r;
	   int k;

	if (unlikely(NULL == dtor))
		return;
	if (unlikely(NULL == root))
		return;
	if (unlikely(NULL == *root))
		return;

	r = *root;
	pti = r->data;
	for (k = 0; k < r->nedges; k++) {

		n = r->edge[k].kid;

		while (1 == n->nedges)
			n = n->edge->kid;

		/* keep the whip */
		if (0 == n->nedges)
			continue;

		p = n->parent;
		n->parent = NULL;
		__pt_trim(n, dtor, pti);
		n->parent = p;
	}
}

static void pt_nuke(void **root, pt_visit dtor)
{
	struct pt_info *pti;
	struct pt_node *r;

	if (unlikely(NULL == root))
		return;
	if (unlikely(NULL == *root))
		return;
	if (unlikely(NULL == dtor))
		return;

	r = *root;
	pti = r->data;
	__pt_trim(*root, dtor, pti);
	free(pti);

	mp->put(__pt_mempool, *root);
	*root = NULL;
}
