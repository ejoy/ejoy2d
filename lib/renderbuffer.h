#ifndef ejoy2d_renderbuffer_h
#define ejoy2d_renderbuffer_h

#include <stdint.h>

#define MAX_COMMBINE 1024

struct vertex_pack {
	float vx;
	float vy;
	float tx;
	float ty;
};

struct vertex {
	struct vertex_pack vp;
	uint8_t rgba[4];
};

struct quad {
	struct vertex p[4];
};

struct render_buffer {
	int object;
	int texid;
	unsigned int vbid;
	struct quad vb[MAX_COMMBINE];
};

void renderbuffer_init(struct render_buffer *rb);
void renderbuffer_upload(struct render_buffer *rb);
void renderbuffer_unload(struct render_buffer *rb);

int renderbuffer_add(struct render_buffer *rb, const struct vertex_pack vb[4], uint32_t color);

static inline void renderbuffer_clear(struct render_buffer *rb) {
	rb->object = 0;
}

struct sprite;

// 0 : ok, 1 full, -1 error (type must be picture)
int renderbuffer_drawsprite(struct render_buffer *rb, struct sprite *s);

#endif
