#include "carray.h"

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "array.h"

// align to qword
#define ALIGN(n) (((n) + 7) & ~7)

struct array_node {
	struct array_node * next;
};

int 
array_size(int n, int sz) {
	sz = ALIGN(sz);
	return n * sz;
}

void 
array_init(struct array *p, void * buffer, int n, int nsz) {
	int sz = ALIGN(nsz);
	char * ptr = (char *)buffer;
	int i;
	for (i=0;i<n-1;i++) {
		struct array_node * node = (struct array_node *)(ptr + i*sz);
		struct array_node * node_next = (struct array_node *)(ptr + (i+1)*sz);
		node->next = node_next;
	}
	struct array_node * node = (struct array_node *)(ptr + (n-1)*sz);
	node->next = NULL;
	p->n = n;
	p->sz = sz;
	p->buffer = (char*)buffer;
	p->freelist = (struct array_node*)buffer;
}

void * 
array_alloc(struct array *p) {
	struct array_node * node = p->freelist;
	if (node == NULL) {
		return NULL;
	}
	p->freelist = node->next;
	memset(node, 0, p->sz);
	return node;
}

void 
array_free(struct array *p, void *v) {
	struct array_node * node = (struct array_node *)v;
	if (node) {
		node->next = p->freelist;
		p->freelist = node;
	}
}

int 
array_id(struct array *p, void *v) {
	if (v == NULL)
		return 0;
	int idx = ((char *)v - p->buffer) / p->sz;
	assert(idx >= 0 && idx < p->n);

	return idx + 1;
}

void * 
array_ref(struct array *p, int id) {
	if (id == 0)
		return NULL;
	--id;
	assert(id >= 0 && id < p->n);
	void * ptr = p->buffer + p->sz * id;
	return ptr;
}

void 
array_exit(struct array *p, void (*close)(void *p, void *ud), void *ud) {
	ARRAY(char, flag, p->n);
	memset(flag, 0, p->n);
	struct array_node * n = p->freelist;
	while (n) {
		int idx = array_id(p, n) - 1;
		flag[idx] = 1;
		n = n->next;
	}
	int i;
	for (i=0;i<p->n;i++) {
		if (flag[i] == 0) {
			struct array_node * n = (struct array_node *)array_ref(p, i + 1);
			close(n, ud);
		}
	}
}

