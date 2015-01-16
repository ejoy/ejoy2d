#include "texture.h"
#include "shader.h"

#define MAX_TEXTURE 128

struct texture {
	int width;
	int height;
	float invw;
	float invh;
	RID id;
	RID fb; /// rt 's frame buffer
};

struct texture_pool {
	int count;
	struct texture tex[MAX_TEXTURE];
};

static struct texture_pool POOL;
static struct render *R = NULL;

void 
texture_initrender(struct render *r) {
	R = r;
}

static inline uint32_t
average4(uint32_t c[4]) {
	int i;
	uint32_t hi = 0;
	uint32_t low = 0;
	for (i=0;i<4;i++) {
		uint32_t v = c[i];
		low += v & 0x00ff00ff;
		hi += (v & 0xff00ff00) >> 8;
	}
	hi = (hi/4) & 0x00ff00ff;
	low = (low/4) & 0x00ff00ff;

	return hi << 8 | low; 
}

static void 
texture_reduce(enum TEXTURE_FORMAT type, int *width, int *height, void *buffer) {
	int w = *width;
	int h = *height;
	if (w%2 == 1 || h%2 == 1)
		return;
	// only support RGBA8888 now
	if (type != TEXTURE_RGBA8) {
		return;
	}
	uint32_t *src = (uint32_t*)buffer;
	uint32_t *dst = (uint32_t*)buffer;
	int i,j;
	for (i=0;i<h;i+=2) {
		for (j=0;j<w;j+=2) {
			uint32_t c[4] = { src[j], src[j+1], src[j+w], src[j+w+1] };
			*dst = average4(c);
			++dst;
		}
		src += w*2;
	}
	*width = w/2;
	*height = h/2;
}

const char * 
texture_load(int id, enum TEXTURE_FORMAT pixel_format, int pixel_width, int pixel_height, void *data, int reduce) {
	if (id >= MAX_TEXTURE) {
		return "Too many texture";
	}
	struct texture * tex = &POOL.tex[id];
	if (id >= POOL.count) {
		POOL.count = id + 1;
	} 
	tex->fb = 0;
	tex->width = pixel_width;
	tex->height = pixel_height;
	tex->invw = 1.0f / (float)pixel_width;
	tex->invh = 1.0f / (float)pixel_height;
	if (tex->id == 0) {
		tex->id = render_texture_create(R, pixel_width, pixel_height, pixel_format, TEXTURE_2D, 0);
	}
	if (data == NULL) {
		// empty texture
		return NULL;
	}

	if (reduce) {
		texture_reduce(pixel_format, &pixel_width, &pixel_height, data);
	}
	render_texture_update(R, tex->id, pixel_width, pixel_height, data, 0, 0);

	return NULL;
}

const char*
texture_new_rt(int id, int w, int h){
	if (id >= MAX_TEXTURE) {
		return "Too many texture";
	}

	struct texture * tex = &POOL.tex[id];
	if (id >= POOL.count) {
		POOL.count = id + 1;
	}

	tex->width = w;
	tex->height = h;
	tex->invw = 1.0f / (float) w;
	tex->invh = 1.0f / (float) h;
	if (tex->id == 0) {
		tex->fb = render_target_create(R, w, h, TEXTURE_RGBA8);
		tex->id = render_target_texture(R, tex->fb);
	}

	return NULL;
}

const char*
texture_active_rt(int id) {
	if (id < 0 || id >= POOL.count)
		return "Invalid rt id";
	struct texture *tex = &POOL.tex[id];

	render_set(R, TARGET, tex->fb, 0);

	return NULL;
}

int
texture_coord(int id, float x, float y, uint16_t *u, uint16_t *v) {
	if (id < 0 || id >= POOL.count) {
		*u = (uint16_t)x;
		*v = (uint16_t)y;
		return 1;
	}
	struct texture *tex = &POOL.tex[id];
	if (tex->invw == 0) {
		// not load the texture
		*u = (uint16_t)x;
		*v = (uint16_t)y;
		return 1;
	}
//	x = (x+0.5f) * tex->invw;
//	y = (y+0.5f) * tex->invh;
	x *= tex->invw;
	y *= tex->invh;
	if (x > 1.0f)
		x = 1.0f;
	if (y > 1.0f)
		y = 1.0f;

	x *= 0xffff;
	y *= 0xffff;

	*u = (uint16_t)x;
	*v = (uint16_t)y;

	return 0;
}

void 
texture_unload(int id) {
	if (id < 0 || id >= POOL.count)
		return;
	struct texture *tex = &POOL.tex[id];
	if (tex->id == 0)
		return;
	render_release(R, TEXTURE, tex->id);
	if (tex->fb != 0)
		render_release(R, TARGET, tex->fb);
	tex->id = 0;
	tex->fb = 0;
}

RID
texture_glid(int id) {
	if (id < 0 || id >= POOL.count)
		return 0;
	struct texture *tex = &POOL.tex[id];
	return tex->id;
}

void 
texture_clearall() {
	int i;
	for (i=0;i<POOL.count;i++) {
		texture_unload(i);
	}
}

void 
texture_exit() {
	texture_clearall();
	POOL.count = 0;
}

void
texture_set_inv(int id, float invw, float invh) {
   if (id < 0 || id >= POOL.count)
       return ;
    
    struct texture *tex = &POOL.tex[id];
    tex->invw = invw;
    tex->invh = invh;
}

void
texture_swap(int ida, int idb) {
    if (ida < 0 || idb < 0 || ida >= POOL.count || idb >= POOL.count)
        return ;
    
    struct texture tex = POOL.tex[ida];
    POOL.tex[ida] = POOL.tex[idb];
    POOL.tex[idb] = tex;
}

void
texture_size(int id, int *width, int *height) {
    if (id < 0 || id >= POOL.count) {
        *width = *height = 0;
        return ;
    }
    
    struct texture *tex = &POOL.tex[id];
    *width = tex->width;
    *height = tex->height;
}

void
texture_delete_framebuffer(int id) {
    if (id < 0 || id >= POOL.count) {
        return;
    }
    
    struct texture *tex = &POOL.tex[id];
    if (tex->fb != 0) {
		render_release(R, TARGET, tex->fb);
        tex->fb = 0;
    }
}

const char * 
texture_update(int id, int pixel_width, int pixel_height, void *data) {
	if (id >= MAX_TEXTURE) {
		return "Too many texture";
	}

	if(data == NULL){
		return "no content";
	}
	struct texture * tex = &POOL.tex[id];
	if(tex->id == 0){
		return "not a valid texture";
	}
	render_texture_update(R, tex->id, pixel_width, pixel_height, data, 0, 0);

	return NULL;
}

