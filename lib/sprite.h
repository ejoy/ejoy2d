#ifndef EJOY_2D_SPRITE_H
#define EJOY_2D_SPRITE_H

#include "spritepack.h"
#include "matrix.h"

#include <lua.h>
#include <stdint.h>
#include <stdbool.h>

struct sprite {
	struct sprite * parent;
	uint16_t type;
	uint16_t id;
	struct sprite_trans t;
	union {
		struct pack_animation *ani;
		struct pack_picture *pic;
		struct pack_polygon *poly;
		struct pack_label *label;
		struct pack_pannel *pannel;
		struct matrix *mat;
	} s;
	struct matrix mat;
	int start_frame;
	int total_frame;
	int frame;
	bool visible;
	bool message;
	const char *name;	// name for parent
	union {
		struct sprite * children[1];
		const char * text;
		int scissor;
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
void sprite_draw_as_child(struct sprite *, struct srt *srt, struct matrix *mat, uint32_t color);
struct sprite * sprite_test(struct sprite *, struct srt *srt, int x, int y);

// return child index, -1 means not found
int sprite_child(struct sprite *, const char * childname);
// return sprite id in pack, -1 for end
int sprite_component(struct sprite *, int index);
const char * sprite_childname(struct sprite *, int index);
int sprite_setframe(struct sprite *, int frame, bool force_child);
void sprite_mount(struct sprite *, int index, struct sprite *);

void sprite_aabb(struct sprite *s, struct srt *srt, int aabb[4]);

bool sprite_child_visible(struct sprite *s, const char * childname);

int ejoy2d_sprite(lua_State *L);

#endif
