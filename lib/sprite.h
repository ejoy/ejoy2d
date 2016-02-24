#ifndef EJOY_2D_SPRITE_H
#define EJOY_2D_SPRITE_H

#include "spritepack.h"
#include "matrix.h"
#include "renderbuffer.h"

#include <lua.h>
#include <stdint.h>
#include <stdbool.h>

#define SPRFLAG_INVISIBLE           (1)
#define SPRFLAG_MESSAGE             (2)
#define SPRFLAG_MULTIMOUNT          (4)
#define SPRFLAG_FORCE_INHERIT_FRAME (8)

struct material;
struct vertex_pack;

struct anchor_data {
	struct particle_system *ps;
	struct pack_picture *pic;
	struct matrix mat;
};

struct cache_vp {
    struct vertex_pack vb[4];
    int texglid;
};

struct sprite {
	struct sprite * parent;
	struct sprite_pack * pack;
	uint16_t type;
	uint16_t id;
    bool cache_dirty;
	struct sprite_trans t;
	union {
		struct pack_animation *ani;
		struct pack_picture *pic;
		struct pack_polygon_data *poly;
		struct pack_label *label;
		struct pack_pannel *pannel;
		struct matrix *mat;
	} s;
	struct matrix mat;
	int start_frame;
	int total_frame;
	int frame;
	int flags;
	const char *name;	// name for parent
	struct material *material;
	union {
		struct sprite * children[1];
		struct rich_text * rich_text;
		int scissor;
		struct anchor_data *anchor;
        struct cache_vp* vp_cache;
	} data;
};

struct sprite_trans * sprite_trans_mul(struct sprite_trans *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix);
struct sprite_trans * sprite_trans_mul2(struct sprite_pack *pack, struct sprite_trans_data *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix);

void sprite_drawquad(struct pack_picture *picture, const struct srt *srt, struct sprite_trans *arg, struct material * material);
int sprite_drawquad_ex(struct pack_picture *picture, const struct srt *srt,
                       const struct sprite_trans *arg, struct vertex_pack * vp, int tex_glid);
void sprite_drawpolygon(struct sprite_pack *pack, struct pack_polygon_data *poly, const struct srt *srt, const struct sprite_trans *arg);
//=======
//void sprite_drawquad(struct pack_picture *picture, const struct srt *srt, const struct sprite_trans *arg);
//int sprite_drawquad_ex(struct pack_picture *picture, const struct srt *srt, const struct sprite_trans *arg, struct vertex_pack * vp, int tex_glid);
//void sprite_drawpolygon(struct sprite_pack *pack, struct pack_polygon_data *poly, const struct srt *srt, const struct sprite_trans *arg);
//>>>>>>> ejoy2d_farm
int sprite_draw_child(struct sprite *s, struct srt *srt, struct sprite_trans * ts, struct material * material);
int sprite_draw_child_cache(struct sprite *s, struct sprite_trans * parent_ts, struct sprite_trans * frame_ts, struct material * material);

// sprite_size must be call before sprite_init
int sprite_size(struct sprite_pack *pack, int id);
void sprite_init(struct sprite *, struct sprite_pack * pack, int id, int sz);

// return action frame number, -1 means action is not exist
int sprite_action(struct sprite *, const char * action);
int sprite_has_action(struct sprite *, const char* action);

void sprite_draw(struct sprite *, struct srt *srt);
void sprite_draw_as_child(struct sprite *, struct srt *srt, struct matrix *mat, uint32_t color);
struct sprite * sprite_test(struct sprite *, struct srt *srt, int x, int y);

// return child index, -1 means not found
int sprite_child(struct sprite *, const char * childname, int start_idx);
int sprite_child_ptr(struct sprite *, struct sprite *child);
// return sprite id in pack, -1 for end
int sprite_component(struct sprite *, int index);
const char * sprite_childname(struct sprite *, int index);
int sprite_setframe(struct sprite *, int frame, bool force_child);
void sprite_mount(struct sprite *, int index, struct sprite *);

void sprite_aabb(struct sprite *s, struct srt *srt, bool world_aabb, int aabb[4]);
int sprite_pos(struct sprite *s, struct srt *srt, struct matrix *m, int pos[2]);	// todo: maybe unused, use sprite_matrix instead
// calc the sprite's world matrix
void sprite_matrix(struct sprite *s, struct matrix *mat);

void sprite_label_only(int lable_only);
void sprite_screen_visible_test(bool enable);
void sprite_cur_dtex_id(int dtex_id);
bool sprite_child_visible(struct sprite *s, const char * childname);
void sprite_set_child_visible(struct sprite *s, const char * childname, bool visible);
void sprite_set_texture_wrap(struct sprite *s, int mode, float* uv_mul);
int sprite_material_size(struct sprite *s);

int ejoy2d_sprite(lua_State *L);
void screen_draw_scene_begin();
void screen_draw_scene_end();
int set_viewport_srt(struct srt *srt);
void sprite_drawscene_st(float s, float x, float y);

#endif
