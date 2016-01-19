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
#define TYPE_MATRIX 7

#define ANCHOR_ID 0xffff
#define SCREEN_SCALE 16

struct matrix;
typedef uint32_t offset_t;
typedef uint16_t uv_t;

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
	uv_t texture_coord[8];
	int32_t screen_coord[8];
};

#define SIZEOF_QUAD (sizeof(struct pack_quad))

struct pack_picture {
	int n;
	struct pack_quad rect[1];
};

#define SIZEOF_PICTURE (sizeof(struct pack_picture) - sizeof(struct pack_quad))

struct pack_poly {
	uv_t *texture_coord;	// uv_t *
	int32_t *screen_coord;	// int32_t *
	int texid;
	int n;
};

struct pack_poly_data {
	offset_t texture_coord;	// uv_t *
	offset_t screen_coord;	// int32_t *
	int texid;
	int n;
};

#define SIZEOF_POLY (sizeof(struct pack_poly_data))

struct pack_polygon_data {
	int n;
	struct pack_poly_data poly[1];
};

#define SIZEOF_POLYGON (sizeof(struct pack_polygon_data) - sizeof(struct pack_poly_data))

struct pack_polygon {
	int n;
	struct pack_poly poly[1];
};

struct sprite_trans {
	struct matrix * mat;
	uint32_t color;
	uint32_t additive;
	int program;
};

struct sprite_trans_data {
	offset_t mat;
	uint32_t color;
	uint32_t additive;
};

#define SIZEOF_TRANS (sizeof(struct sprite_trans_data))

struct pack_part {
	struct sprite_trans_data t;
	int16_t component_id;
	int16_t touchable;
};

#define SIZEOF_PART (sizeof(struct pack_part))

struct pack_frame {
	offset_t part;	// struct pack_part *
	int n;
};

#define SIZEOF_FRAME (sizeof(struct pack_frame))

struct pack_action {
	offset_t name;	// const char *
	int16_t number;
	int16_t start_frame;
};

#define SIZEOF_ACTION (sizeof(struct pack_action))

struct pack_component {
	offset_t name;	// const char *
	int id;
};

#define SIZEOF_COMPONENT (sizeof(struct pack_component))

struct pack_animation {
	offset_t frame;	// struct pack_frame *
	offset_t action;	// struct pack_action *
	int frame_number;
	int action_number;
	int component_number;
	struct pack_component component[1];
};

#define SIZEOF_ANIMATION (sizeof(struct pack_animation) - sizeof(struct pack_component))

struct sprite_pack {
	offset_t type;	// uint8_t *
	offset_t data;	// void **
	int n;
	int tex[2];
};

#define SIZEOF_PACK (sizeof(struct sprite_pack) - 2 * sizeof(int))

int ejoy2d_spritepack(lua_State *L);
void dump_pack(struct sprite_pack *pack);

#define OFFSET_TO_POINTER(t, pack, off) ((off == 0) ? NULL : (t*)((uintptr_t)(pack) + (off)))
#define OFFSET_TO_STRING(pack, off) ((const char *)(pack) + (off))
#define POINTER_TO_OFFSET(pack, ptr) ((ptr == NULL) ? 0 : (offset_t)((uintptr_t)(ptr) - (uintptr_t)pack))

#endif
