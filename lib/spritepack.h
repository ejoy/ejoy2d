#ifndef EJOY_2D_SPRITE_PACK_H
#define EJOY_2D_SPRITE_PACK_H

#include <lua.h>
#include <stdint.h>

#define TYPE_EMPTY 0
#define TYPE_PICTURE 1
#define TYPE_ANIMATION 2
#define TYPE_POLYGON 3
#define TYPE_LABEL 4
#define TYPE_PANNEL 5
#define TYPE_ANCHOR 6

#define ANCHOR_ID 0xffff
#define SCREEN_SCALE 16

// binary package should reserve more bytes for 64bit platform
#define PTR_SIZE_DIFF (8 - sizeof(void *))
#define SIZEOF_POINTER 8

struct matrix;

#define SIZEOF_MATRIX (sizeof(struct matrix))

struct pack_pannel {
	int width;
	int height;
	int scissor;
};

#define SIZEOF_PANNEL (sizeof(struct pack_pannel))

struct pack_label {
	uint32_t color;
	int width;
	int height;
	int align;
	int size;
	int edge;
    int space_h;
    int space_w;
    int auto_scale;
};

#define SIZEOF_LABEL (sizeof(struct pack_label))

struct pack_quad {
	int texid;
	uint16_t texture_coord[8];
	int32_t screen_coord[8];
};

#define SIZEOF_QUAD (sizeof(struct pack_quad))

struct pack_picture {
	int n;
	struct pack_quad rect[1];
};

#define SIZEOF_PICTURE (sizeof(struct pack_picture) - sizeof(struct pack_quad))

struct pack_poly {
	uint16_t *texture_coord;
	int32_t *screen_coord;
	int texid;
	int n;
};

#define SIZEOF_POLY (sizeof(struct pack_poly) + 2 * PTR_SIZE_DIFF)

struct pack_polygon {
	int n;
	int _dummy;		// unused: dummy for align to 64bit
	struct pack_poly poly[1];
};

#define SIZEOF_POLYGON (sizeof(struct pack_polygon) - sizeof(struct pack_poly))

struct sprite_trans {
	struct matrix * mat;
	uint32_t color;
	uint32_t additive;
	int program;
	int _dummy;		// unused: dummy for align to 64bit
};

#define SIZEOF_TRANS (sizeof(struct sprite_trans) + PTR_SIZE_DIFF)

struct pack_part {
	struct sprite_trans t;
	int component_id;
	int touchable;
};

#define SIZEOF_PART (sizeof(struct pack_part) + SIZEOF_TRANS - sizeof(struct sprite_trans))

struct pack_frame {
	struct pack_part *part;
	int n;
	int _dummy;		// unused: dummy for align to 64bit
};

#define SIZEOF_FRAME (sizeof(struct pack_frame) + PTR_SIZE_DIFF)

struct pack_action {
	const char * name;
	int number;
	int start_frame;
};

#define SIZEOF_ACTION (sizeof(struct pack_action) + PTR_SIZE_DIFF)

struct pack_component {
	const char *name;
	int id;
	int _dummy;		// unused: dummy for align to 64bit
};

#define SIZEOF_COMPONENT (sizeof(struct pack_component) + PTR_SIZE_DIFF)

struct pack_animation {
	struct pack_frame *frame;
	struct pack_action *action;
	int frame_number;
	int action_number;
	int component_number;
	int _dummy;		// unused: dummy for align to 64bit
	struct pack_component component[1];
};

#define SIZEOF_ANIMATION (sizeof(struct pack_animation) + 2 * PTR_SIZE_DIFF - sizeof(struct pack_component))

struct sprite_pack {
	uint8_t * type;
	void ** data;
	int n;
	int _dummy;		// unused: dummy for align to 64bit
	int tex[2];
};

#define SIZEOF_PACK (sizeof(struct sprite_pack) + 2 * PTR_SIZE_DIFF - 2 * sizeof(int))

int ejoy2d_spritepack(lua_State *L);
void dump_pack(struct sprite_pack *pack);

#endif
