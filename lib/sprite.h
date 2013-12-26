#ifndef EJOY_2D_SPRITE_H
#define EJOY_2D_SPRITE_H

#include "spritepack.h"
#include "matrix.h"

#include <lua.h>
#include <stdint.h>

struct sprite {
	uint16_t type;
	uint16_t id;
	struct sprite_trans t;
	union {
		struct pack_animation *ani;
		struct pack_picture *pic;
		struct pack_polygon *poly;
		struct pack_label *label;
	} s;
	struct matrix mat;
	int start_frame;
	int total_frame; 
	int frame;	
	int visible;
	const char *name;	// name for parent
	union {
		struct sprite * children[1];
		const char * text;
	} data;
};

void sprite_drawquad(struct pack_picture *picture, const struct srt *srt, const struct sprite_trans *arg);
void sprite_drawpolygon(struct pack_polygon *poly, const struct srt *srt, const struct sprite_trans *arg);

// sprite_size must be call before sprite_init
int sprite_size(struct sprite_pack *pack, int id);
void sprite_init(struct sprite *, struct sprite_pack * pack, int id, int sz);

// return action frame number, -1 means action is not exist
int sprite_action(struct sprite *, const char * action);

void sprite_draw(struct sprite *, struct srt *srt);

// return child index, -1 means not found
int sprite_child(struct sprite *, const char * childname);
// return sprite id in pack, -1 for end
int sprite_component(struct sprite *, int index);
void sprite_setframe(struct sprite *, int frame);
void sprite_mount(struct sprite *, int index, struct sprite *);

int ejoy2d_sprite(lua_State *L);

#endif
