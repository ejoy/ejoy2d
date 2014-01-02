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

#define SCREEN_SCALE 16

struct matrix;

struct pack_pannel {
	int width;
	int height;
	int scissor;
};

struct pack_label {
	uint32_t color;
	int width;
	int height;
	int align;
	int size;
};

struct pack_quad {
	int texid;
	uint16_t texture_coord[8];
	int32_t screen_coord[8];
};

struct pack_picture {
	int n;
	struct pack_quad rect[1];
};

struct pack_poly {
	int texid;
	int n;
	uint16_t *texture_coord;
	int32_t *screen_coord;
};

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

struct pack_part {
	int component_id;
	struct sprite_trans t;
};

struct pack_frame {
	int n;
	struct pack_part *part;
};

struct pack_action {
	const char * name;
	int number;
	int start_frame;
};

struct pack_component {
	int id;
	const char *name;
};

struct pack_animation {
	int frame_number;
	int action_number;
	int component_number;
	struct pack_frame *frame;
	struct pack_action *action;
	struct pack_component component[1];
};

struct sprite_pack {
	int n;
	uint8_t * type;
	void ** data;
	int tex[1];
};

int ejoy2d_spritepack(lua_State *L);
void dump_pack(struct sprite_pack *pack);

#endif
