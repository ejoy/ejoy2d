#ifndef ejoy3d_block_h
#define ejoy3d_block_h

#include <stdlib.h>

struct block {
	char * buffer;
	int sz;
};

static inline void 
block_init(struct block * B, void * buffer, int sz) {
	B->buffer = (char*)buffer;
	B->sz = sz;
}

static inline void * 
block_slice(struct block * B, int sz) {
	if (B->sz < sz) {
		return NULL;
	}
	void * ret = B->buffer;
	B->buffer += sz;
	B->sz -= sz;
	return ret;
}

#endif
