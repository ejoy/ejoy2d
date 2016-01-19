#include "sprite.h"
#include "spritepack.h"
#include "shader.h"
#include "texture.h"
#include "screen.h"
#include "matrix.h"
#include "label.h"
#include "scissor.h"
#include "array.h"
#include "particle.h"
#include "material.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

void
sprite_drawquad(struct pack_picture *picture, const struct srt *srt,  const struct sprite_trans *arg) {
	struct matrix tmp;
	struct vertex_pack vb[4];
	int i,j;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	int *m = tmp.m;
	for (i=0;i<picture->n;i++) {
		struct pack_quad *q = &picture->rect[i];
		int glid = texture_glid(q->texid);
		if (glid == 0)
			continue;
		shader_texture(glid, 0);
		for (j=0;j<4;j++) {
			int xx = q->screen_coord[j*2+0];
			int yy = q->screen_coord[j*2+1];
			float vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
			float tx = q->texture_coord[j*2+0];
			float ty = q->texture_coord[j*2+1];

			screen_trans(&vx,&vy);
			vb[j].vx = vx;
			vb[j].vy = vy;
			vb[j].tx = tx;
			vb[j].ty = ty;
		}
		shader_draw(vb, arg->color, arg->additive);
	}
}

void
sprite_drawpolygon(struct pack_polygon *poly, const struct srt *srt, const struct sprite_trans *arg) {
	struct matrix tmp;
	int i,j;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	int *m = tmp.m;
	for (i=0;i<poly->n;i++) {
		struct pack_poly *p = &poly->poly[i];
		int glid = texture_glid(p->texid);
		if (glid == 0)
			continue;
		shader_texture(glid, 0);
		int pn = p->n;

		ARRAY(struct vertex_pack, vb, pn);

		for (j=0;j<pn;j++) {
			int xx = p->screen_coord[j*2+0];
			int yy = p->screen_coord[j*2+1];


			float vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
			float tx = p->texture_coord[j*2+0];
			float ty = p->texture_coord[j*2+1];

			screen_trans(&vx,&vy);

			vb[j].vx = vx;
			vb[j].vy = vy;
			vb[j].tx = tx;
			vb[j].ty = ty;
		}
		shader_drawpolygon(pn, vb, arg->color, arg->additive);
	}
}

int
sprite_size(struct sprite_pack *pack, int id) {
	if (id < 0 || id >=	pack->n)
		return 0;
	int8_t * type_array = OFFSET_TO_POINTER(int8_t, pack, pack->type);
	int type = type_array[id];
	if (type == TYPE_ANIMATION) {
		offset_t *data = OFFSET_TO_POINTER(offset_t, pack, pack->data);
		struct pack_animation * ani = OFFSET_TO_POINTER(struct pack_animation, pack, data[id]);
		return sizeof(struct sprite) + (ani->component_number - 1) * sizeof(struct sprite *);
	} else {
		return sizeof(struct sprite);
	}
	return 0;
}

int
sprite_action(struct sprite *s, const char * action) {
	if (s->type != TYPE_ANIMATION) {
		return -1;
	}
	struct pack_animation *ani = s->s.ani;
	struct pack_action *pa = OFFSET_TO_POINTER(struct pack_action, s->pack, ani->action);
	if (action == NULL) {
		if (ani->action == 0) {
			return -1;
		}
		s->start_frame = pa[0].start_frame;
		s->total_frame = pa[0].number;
		s->frame = 0;
		return s->total_frame;
	} else {
		int i;
		for (i=0;i<ani->action_number;i++) {
			const char *name = OFFSET_TO_STRING(s->pack, pa[i].name);
			if (name) {
				if (strcmp(name, action)==0) {
					s->start_frame = pa[i].start_frame;
					s->total_frame = pa[i].number;
					s->frame = 0;
					return s->total_frame;
				}
			}
		}
		return -1;
	}
}

void
sprite_init(struct sprite * s, struct sprite_pack * pack, int id, int sz) {
	if (id < 0 || id >=	pack->n)
		return;
	s->parent = NULL;
	s->pack = pack;
	s->t.mat = NULL;
	s->t.color = 0xffffffff;
	s->t.additive = 0;
	s->t.program = PROGRAM_DEFAULT;
	s->flags = 0;
	s->name = NULL;
	s->id = id;
	uint8_t *type_array = OFFSET_TO_POINTER(uint8_t, pack, pack->type);
	s->type = type_array[id];
	s->material = NULL;
	offset_t *data = OFFSET_TO_POINTER(offset_t, pack, pack->data);
	if (s->type == TYPE_ANIMATION) {
		struct pack_animation * ani = OFFSET_TO_POINTER(struct pack_animation, pack, data[id]);
		s->s.ani = ani;
		s->frame = 0;
		s->start_frame = 0;
		s->total_frame = 0;
		sprite_action(s, NULL);
		int i;
		int n = ani->component_number;
		assert(sz >= sizeof(struct sprite) + (n - 1) * sizeof(struct sprite *));
		for (i=0; i<n ;i++) {
			s->data.children[i] = NULL;
		}
	} else {
		s->s.pic = OFFSET_TO_POINTER(struct pack_picture, pack, data[id]);
		s->start_frame = 0;
		s->total_frame = 0;
		s->frame = 0;
		memset(&s->data, 0, sizeof(s->data));
		assert(sz >= sizeof(struct sprite) - sizeof(struct sprite *));
		if (s->type == TYPE_PANNEL) {
			struct pack_pannel * pp = OFFSET_TO_POINTER(struct pack_pannel, pack, data[id]);
			s->data.scissor = pp->scissor;
		}
	}
}

void
sprite_mount(struct sprite *parent, int index, struct sprite *child) {
	assert(parent->type == TYPE_ANIMATION);
	struct pack_animation *ani = parent->s.ani;
	assert(index >= 0 && index < ani->component_number);
	struct sprite * oldc = parent->data.children[index];
	if (oldc) {
		oldc->parent = NULL;
		oldc->name = NULL;
	}
	parent->data.children[index] = child;
	if (child) {
		assert(child->parent == NULL);
		if ((child->flags & SPRFLAG_MULTIMOUNT) == 0) {
			child->name = OFFSET_TO_STRING(parent->pack, ani->component[index].name);
			child->parent = parent;
		}
		if (oldc && oldc->type == TYPE_ANCHOR) {
			if(oldc->flags & SPRFLAG_MESSAGE) {
				child->flags |= SPRFLAG_MESSAGE;
			} else {
				child->flags &= ~SPRFLAG_MESSAGE;
			}
		}
	}
}

static inline int
get_frame(struct sprite *s) {
	if (s->type != TYPE_ANIMATION) {
		return s->start_frame;
	}
	if (s->total_frame <= 0) {
		return -1;
	}
	int f = s->frame % s->total_frame;
	if (f < 0) {
		f += s->total_frame;
	}
	return f + s->start_frame;
}

int
sprite_child(struct sprite *s, const char * childname) {
	assert(childname);
	if (s->type != TYPE_ANIMATION)
		return -1;
	struct pack_animation *ani = s->s.ani;
	int i;
	for (i=0;i<ani->component_number;i++) {
		const char *name = OFFSET_TO_STRING(s->pack, ani->component[i].name);
		if (name) {
			if (strcmp(name, childname)==0) {
				return i;
			}
		}
	}
	return -1;
}

int
sprite_child_ptr(struct sprite *s, struct sprite *child) {
	if (s->type != TYPE_ANIMATION)
		return -1;
	struct pack_animation *ani = s->s.ani;
	int i;
	for (i=0;i<ani->component_number;i++) {
		struct sprite * c = s->data.children[i];
		if (c == child)
			return i;
	}
	return -1;
}

int
sprite_component(struct sprite *s, int index) {
	if (s->type != TYPE_ANIMATION)
		return -1;
	struct pack_animation *ani = s->s.ani;
	if (index < 0 || index >= ani->component_number)
		return -1;
	return ani->component[index].id;
}

const char *
sprite_childname(struct sprite *s, int index) {
	if (s->type != TYPE_ANIMATION)
		return NULL;
	struct pack_animation *ani = s->s.ani;
	if (index < 0 || index >= ani->component_number)
		return NULL;
	return OFFSET_TO_STRING(s->pack, ani->component[index].name);
}

// draw sprite

static inline uint32_t
color_mul(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 24) & 0xff;
	int g1 = (c1 >> 16) & 0xff;
	int b1 = (c1 >> 8) & 0xff;
	int a1 = (c1) & 0xff;
	int r2 = (c2 >> 24) & 0xff;
	int g2 = (c2 >> 16) & 0xff;
	int b2 = (c2 >> 8) & 0xff;
	int a2 = c2 & 0xff;

	return (r1 * r2 /255) << 24 |
		(g1 * g2 /255) << 16 |
		(b1 * b2 /255) << 8 |
		(a1 * a2 /255) ;
}

static inline unsigned int
clamp(unsigned int c) {
	return ((c) > 255 ? 255 : (c));
}

static inline uint32_t
color_add(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 16) & 0xff;
	int g1 = (c1 >> 8) & 0xff;
	int b1 = (c1) & 0xff;
	int r2 = (c2 >> 16) & 0xff;
	int g2 = (c2 >> 8) & 0xff;
	int b2 = (c2) & 0xff;
	return clamp(r1+r2) << 16 |
		clamp(g1+g2) << 8 |
		clamp(b1+b2);
}

static inline void
sprite_trans_mul_(struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix) {
	if (t->mat == NULL) {
		t->mat = b->mat;
	} else if (b->mat) {
		matrix_mul(tmp_matrix, t->mat, b->mat);
		t->mat = tmp_matrix;
	}
	if (t->color == 0xffffffff) {
		t->color = b->color;
	} else if (b->color != 0xffffffff) {
		t->color = color_mul(t->color, b->color);
	}
	if (t->additive == 0) {
		t->additive = b->additive;
	} else if (b->additive != 0) {
		t->additive = color_add(t->additive, b->additive);
	}
}

struct sprite_trans *
sprite_trans_mul2(struct sprite_pack *pack, struct sprite_trans_data *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix) {
	if (b == NULL && a == NULL) {
		return NULL;
	}
	t->mat = OFFSET_TO_POINTER(struct matrix, pack, a->mat);
	t->color = a->color;
	t->additive = a->additive;
	t->program = PROGRAM_DEFAULT;
	sprite_trans_mul_(b , t, tmp_matrix);
	t->program = b->program;

	return t;
}

struct sprite_trans *
sprite_trans_mul(struct sprite_trans *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix) {
	if (b == NULL) {
		return a;
	}
	*t = *a;
	sprite_trans_mul_(b , t, tmp_matrix);
	if (t->program == PROGRAM_DEFAULT) {
		t->program = b->program;
	}
	return t;
}

static struct matrix *
mat_mul(struct matrix *a, struct matrix *b, struct matrix *tmp) {
	if (b == NULL)
		return a;
	if (a == NULL)
		return b;
	matrix_mul(tmp, a , b);
	return tmp;
}

static void
switch_program(struct sprite_trans *t, int def, struct material *m) {
	int prog = t->program;
	if (prog == PROGRAM_DEFAULT) {
		prog = def;
	}
	shader_program(prog, m);
}

static void
set_scissor(const struct pack_pannel *p, const struct srt *srt, const struct sprite_trans *arg) {
	struct matrix tmp;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	int *m = tmp.m;
	int x[4] = { 0, p->width * SCREEN_SCALE, p->width * SCREEN_SCALE, 0 };
	int y[4] = { 0, 0, p->height * SCREEN_SCALE, p->height * SCREEN_SCALE };
	int minx = (x[0] * m[0] + y[0] * m[2]) / 1024 + m[4];
	int miny = (x[0] * m[1] + y[0] * m[3]) / 1024 + m[5];
	int maxx = minx;
	int maxy = miny;
	int i;
	for (i=1;i<4;i++) {
		int vx = (x[i] * m[0] + y[i] * m[2]) / 1024 + m[4];
		int vy = (x[i] * m[1] + y[i] * m[3]) / 1024 + m[5];
		if (vx<minx) {
			minx = vx;
		} else if (vx > maxx) {
			maxx = vx;
		}
		if (vy<miny) {
			miny = vy;
		} else if (vy > maxy) {
			maxy = vy;
		}
	}
	minx /= SCREEN_SCALE;
	miny /= SCREEN_SCALE;
	maxx /= SCREEN_SCALE;
	maxy /= SCREEN_SCALE;
	scissor_push(minx,miny,maxx-minx,maxy-miny);
}

static void
anchor_update(struct sprite *s, struct srt *srt, struct sprite_trans *arg) {
	struct matrix *r = s->s.mat;
	if (arg->mat == NULL) {
		matrix_identity(r);
	} else {
		*r = *arg->mat;
	}
	matrix_srt(r, srt);
}

static void
label_pos(int m[6], struct pack_label * l, int pos[2]) {
	float c_x = l->width * SCREEN_SCALE / 2.0;
	float c_y = l->height * SCREEN_SCALE / 2.0;
	pos[0] = (int)((c_x * m[0] + c_y * m[2]) / 1024 + m[4])/SCREEN_SCALE;
	pos[1] = (int)((c_x * m[1] + c_y * m[3]) / 1024 + m[5])/SCREEN_SCALE;
}

static void
picture_pos(int m[6], struct pack_picture *picture, int pos[2]) {
	int max_x = INT_MIN;
	int max_y = -INT_MAX;
	int min_x = INT_MAX;
	int min_y = INT_MAX;
	int i,j;
	for (i=0;i<picture->n;i++) {
		struct pack_quad *q = &picture->rect[i];
		for (j=0;j<4;j++) {
			int xx = q->screen_coord[j*2+0];
			int yy = q->screen_coord[j*2+1];

			if (xx > max_x) max_x = xx;
			if (yy > max_y) max_y = yy;
			if (xx < min_x) min_x = xx;
			if (yy < min_y) min_y = yy;
		}
	}

	float c_x = (max_x + min_x) / 2.0;
	float c_y = (max_y + min_y) / 2.0;
	pos[0] = (int)((c_x * m[0] + c_y * m[2]) / 1024 + m[4])/SCREEN_SCALE;
	pos[1] = (int)((c_x * m[1] + c_y * m[3]) / 1024 + m[5])/SCREEN_SCALE;
}

static void
drawparticle(struct sprite *s, struct particle_system *ps, struct pack_picture *pic, const struct srt *srt) {
	int n = ps->particleCount;
	int i;
	struct matrix *old_m = s->t.mat;
	uint32_t old_c = s->t.color;

	shader_blend(ps->config->srcBlend, ps->config->dstBlend);
	for (i=0;i<n;i++) {
		struct particle *p = &ps->particles[i];
		struct matrix *mat = &ps->matrix[i];
		uint32_t color = p->color_val;

		s->t.mat = mat;
		s->t.color = color;
		sprite_drawquad(pic, NULL, &s->t);
	}
	shader_defaultblend();

	s->t.mat = old_m;
	s->t.color = old_c;
}

static int
draw_child(struct sprite *s, struct srt *srt, struct sprite_trans * ts, struct material * material) {
	struct sprite_trans temp;
	struct matrix temp_matrix;
	struct sprite_trans *t = sprite_trans_mul(&s->t, ts, &temp, &temp_matrix);
	if (s->material) {
		material = s->material;
	} 
	switch (s->type) {
	case TYPE_PICTURE:
		switch_program(t, PROGRAM_PICTURE, material);
		sprite_drawquad(s->s.pic, srt, t);
		return 0;
	case TYPE_POLYGON:
		switch_program(t, PROGRAM_PICTURE, material);
		sprite_drawpolygon(s->s.poly, srt, t);
		return 0;
	case TYPE_LABEL:
		if (s->data.rich_text) {
			t->program = PROGRAM_DEFAULT;	// label never set user defined program
			switch_program(t, s->s.label->edge ? PROGRAM_TEXT_EDGE : PROGRAM_TEXT, material);
			label_draw(s->data.rich_text, s->s.label, srt, t);
		}
		return 0;
	case TYPE_ANCHOR:
		if (s->data.anchor->ps){
			switch_program(t, PROGRAM_PICTURE, material);
			drawparticle(s, s->data.anchor->ps, s->data.anchor->pic, srt);
		}
		anchor_update(s, srt, t);
		return 0;
	case TYPE_ANIMATION:
		break;
	case TYPE_PANNEL:
		if (s->data.scissor) {
			// enable scissor
			set_scissor(s->s.pannel, srt, t);
			return 1;
		} else {
			return 0;
		}
	default:
		// todo : invalid type
		return 0;
	}
	// draw animation
	int frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	struct pack_animation *ani = s->s.ani;
	struct pack_frame * pf = OFFSET_TO_POINTER(struct pack_frame, s->pack, ani->frame);
	pf = &pf[frame];
	int i;
	int scissor = 0;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, s->pack, pf->part);
		pp = &pp[i];
		int index = pp->component_id;
		struct sprite * child = s->data.children[index];
		if (child == NULL || (child->flags & SPRFLAG_INVISIBLE)) {
			continue;
		}
		struct sprite_trans temp2;
		struct matrix temp_matrix2;
		struct sprite_trans *ct = sprite_trans_mul2(s->pack, &pp->t, t, &temp2, &temp_matrix2);
		scissor += draw_child(child, srt, ct, material);
	}
	for (i=0;i<scissor;i++) {
		scissor_pop();
	}
	return 0;
}

bool
sprite_child_visible(struct sprite *s, const char * childname) {
	struct pack_animation *ani = s->s.ani;
	int frame = get_frame(s);
	if (frame < 0) {
		return false;
	}
	struct pack_frame * pf = OFFSET_TO_POINTER(struct pack_frame, s->pack, ani->frame);
	pf = &pf[frame];
	int i;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, s->pack, pf->part);
		pp = &pp[i];
		int index = pp->component_id;
		struct sprite * child = s->data.children[index];
		if (child->name && strcmp(childname, child->name) == 0) {
			return true;
		}
	}
	return false;
}

void
sprite_draw(struct sprite *s, struct srt *srt) {
	if ((s->flags & SPRFLAG_INVISIBLE) == 0) {
		draw_child(s, srt, NULL, NULL);
	}
}

void
sprite_draw_as_child(struct sprite *s, struct srt *srt, struct matrix *mat, uint32_t color) {
	if ((s->flags & SPRFLAG_INVISIBLE) == 0) {
		struct sprite_trans st;
		st.mat = mat;
		st.color = color;
		st.additive = 0;
		st.program = PROGRAM_DEFAULT;
		draw_child(s, srt, &st, NULL);
	}
}

int
sprite_pos(struct sprite *s, struct srt *srt, struct matrix *m, int pos[2]) {
	struct matrix temp;
	struct matrix *t = mat_mul(s->t.mat, m, &temp);
	matrix_srt(t, srt);
	switch (s->type) {
	case TYPE_PICTURE:
		picture_pos(t->m, s->s.pic, pos);
		return 0;
	case TYPE_LABEL:
		label_pos(t->m, s->s.label, pos);
		return 0;
	case TYPE_ANIMATION:
	case TYPE_PANNEL:
		pos[0] = t->m[4] / SCREEN_SCALE;
		pos[1] = t->m[5] / SCREEN_SCALE;
		return 0;
	default:
		return 1;
	}

}

void
sprite_matrix(struct sprite * self, struct matrix *mat) {
	struct sprite * parent = self->parent;
	if (parent) {
		assert(parent->type == TYPE_ANIMATION);
		sprite_matrix(parent, mat);
		struct matrix tmp;
		struct matrix * parent_mat = parent->t.mat;

		struct matrix * child_mat = NULL;
		struct pack_animation *ani = parent->s.ani;
		int frame = get_frame(parent);
		if (frame < 0) {
			return;
		}
		struct pack_frame * pf = OFFSET_TO_POINTER(struct pack_frame, parent->pack, ani->frame);
		pf = &pf[frame];
		int i;
		for (i=0;i<pf->n;i++) {
			struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, parent->pack, pf->part);
			pp = &pp[i];
			int index = pp->component_id;
			struct sprite * child = parent->data.children[index];
			if (child == self) {
				child_mat = OFFSET_TO_POINTER(struct matrix, parent->pack, pp->t.mat);
				break;
			}
		}

		if (parent_mat == NULL && child_mat == NULL)
			return;

		if (parent_mat) {
			matrix_mul(&tmp, parent_mat, mat);
		} else {
			tmp = *mat;
		}

		if (child_mat) {
			matrix_mul(mat, child_mat, &tmp);
		} else {
			*mat = tmp;
		}
	} else {
		matrix_identity(mat);
	}
}

// aabb

static void
poly_aabb(int n, const int32_t * point, struct srt *srt, struct matrix *ts, int aabb[4]) {
	struct matrix mat;
	if (ts == NULL) {
		matrix_identity(&mat);
	} else {
		mat = *ts;
	}
	matrix_srt(&mat, srt);
	int *m = mat.m;

	int i;
	for (i=0;i<n;i++) {
		int x = point[i*2];
		int y = point[i*2+1];

		int xx = (x * m[0] + y * m[2]) / 1024 + m[4];
		int yy = (x * m[1] + y * m[3]) / 1024 + m[5];

		if (xx < aabb[0])
			aabb[0] = xx;
		if (xx > aabb[2])
			aabb[2] = xx;
		if (yy < aabb[1])
			aabb[1] = yy;
		if (yy > aabb[3])
			aabb[3] = yy;
	}
}

static inline void
quad_aabb(struct pack_picture * pic, struct srt *srt, struct matrix *ts, int aabb[4]) {
	int i;
	for (i=0;i<pic->n;i++) {
		poly_aabb(4, pic->rect[i].screen_coord, srt, ts, aabb);
	}
}

static inline void
polygon_aabb(struct pack_polygon * polygon, struct srt *srt, struct matrix *ts, int aabb[4]) {
	int i;
	for (i=0;i<polygon->n;i++) {
		struct pack_poly * poly = &polygon->poly[i];
		poly_aabb(poly->n, poly->screen_coord, srt, ts, aabb);
	}
}

static inline void
label_aabb(struct pack_label *label, struct srt *srt, struct matrix *ts, int aabb[4]) {
	int32_t point[] = {
		0,0,
		label->width * SCREEN_SCALE, 0,
		0, label->height * SCREEN_SCALE,
		label->width * SCREEN_SCALE, label->height * SCREEN_SCALE,
	};
	poly_aabb(4, point, srt, ts, aabb);
}

static inline void
panel_aabb(struct pack_pannel *panel, struct srt *srt, struct matrix *ts, int aabb[4]) {
	int32_t point[] = {
		0,0,
		panel->width * SCREEN_SCALE, 0,
		0, panel->height * SCREEN_SCALE,
		panel->width * SCREEN_SCALE, panel->height * SCREEN_SCALE,
	};
	poly_aabb(4, point, srt, ts, aabb);
}

static int
child_aabb(struct sprite *s, struct srt *srt, struct matrix * mat, int aabb[4]) {
	struct matrix temp;
	struct matrix *t = mat_mul(s->t.mat, mat, &temp);
	switch (s->type) {
	case TYPE_PICTURE:
		quad_aabb(s->s.pic, srt, t, aabb);
		return 0;
	case TYPE_POLYGON:
		polygon_aabb(s->s.poly, srt, t, aabb);
		return 0;
	case TYPE_LABEL:
		label_aabb(s->s.label, srt, t, aabb);
		return 0;
	case TYPE_ANIMATION:
		break;
	case TYPE_PANNEL:
		panel_aabb(s->s.pannel, srt, t, aabb);
		return s->data.scissor;
	default:
		// todo : invalid type
		return 0;
	}
	// draw animation
	struct pack_animation *ani = s->s.ani;
	int frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	struct pack_frame * pf = OFFSET_TO_POINTER(struct pack_frame, s->pack, ani->frame);
	pf = &pf[frame];
	int i;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, s->pack, pf->part);
		pp = &pp[i];
		int index = pp->component_id;
		struct sprite * child = s->data.children[index];
		if (child == NULL || (child->flags & SPRFLAG_INVISIBLE)) {
			continue;
		}
		struct matrix temp2;
		struct matrix *ct = mat_mul(OFFSET_TO_POINTER(struct matrix, s->pack, pp->t.mat), t, &temp2);
		if (child_aabb(child, srt, ct, aabb))
			break;
	}
	return 0;
}

void
sprite_aabb(struct sprite *s, struct srt *srt, bool world_aabb, int aabb[4]) {
	int i;
	if ((s->flags & SPRFLAG_INVISIBLE) == 0) {
		struct matrix tmp;
		if (world_aabb) {
			sprite_matrix(s, &tmp);
		} else {
			matrix_identity(&tmp);
		}
		aabb[0] = INT_MAX;
		aabb[1] = INT_MAX;
		aabb[2] = INT_MIN;
		aabb[3] = INT_MIN;
		child_aabb(s,srt,&tmp,aabb);
		for (i=0;i<4;i++)
			aabb[i] /= SCREEN_SCALE;
	} else {
		for (i=0;i<4;i++)
			aabb[i] = 0;
	}
}

// test

static int
test_quad(struct pack_picture * pic, int x, int y) {
	int p;
	for (p=0;p<pic->n;p++) {
		struct pack_quad *pq = &pic->rect[p];
		int maxx,maxy,minx,miny;
		minx= maxx = pq->screen_coord[0];
		miny= maxy = pq->screen_coord[1];
		int i;
		for (i=2;i<8;i+=2) {
			int x = pq->screen_coord[i];
			int y = pq->screen_coord[i+1];
			if (x<minx)
				minx = x;
			else if (x>maxx)
				maxx = x;
			if (y<miny)
				miny = y;
			else if (y>maxy)
				maxy = y;
		}
		if (x>=minx && x<=maxx && y>=miny && y<=maxy)
			return 1;
	}
	return 0;
}

static int
test_polygon(struct pack_polygon * poly,  int x, int y) {
	int p;
	for (p=0;p<poly->n;p++) {
		struct pack_poly *pp = &poly->poly[p];
		int maxx,maxy,minx,miny;
		minx= maxx = pp->screen_coord[0];
		miny= maxy = pp->screen_coord[1];
		int i;
		for (i=1;i<pp->n;i++) {
			int x = pp->screen_coord[i*2+0];
			int y = pp->screen_coord[i*2+1];
			if (x<minx)
				minx = x;
			else if (x>maxx)
				maxx = x;
			if (y<miny)
				miny = y;
			else if (y>maxy)
				maxy = y;
		}
		if (x>=minx && x<=maxx && y>=miny && y<=maxy) {
			return 1;
		}
	}
	return 0;
}

static int
test_label(struct pack_label *label, int x, int y) {
	x /= SCREEN_SCALE;
	y /= SCREEN_SCALE;
	return x>=0 && x<label->width && y>=0 && y<label->height;
}

static int
test_pannel(struct pack_pannel *pannel, int x, int y) {
	x /= SCREEN_SCALE;
	y /= SCREEN_SCALE;
	return x>=0 && x<pannel->width && y>=0 && y<pannel->height;
}

static int test_child(struct sprite *s, struct srt *srt, struct matrix * ts, int x, int y, struct sprite ** touch);

static int
check_child(struct sprite *s, struct srt *srt, struct matrix * t, struct pack_frame * pf, int i, int x, int y, struct sprite ** touch) {
	struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, s->pack, pf->part);
	pp = &pp[i];
	int index = pp->component_id;
	struct sprite * child = s->data.children[index];
	if (child == NULL || (child->flags & SPRFLAG_INVISIBLE)) {
		return 0;
	}
	struct matrix temp2;
	struct matrix *ct = mat_mul(OFFSET_TO_POINTER(struct matrix, s->pack, pp->t.mat), t, &temp2);
	struct sprite *tmp = NULL;
	int testin = test_child(child, srt, ct, x, y, &tmp);
	if (testin) {
		// if child capture message, return it
		*touch = tmp;
		return 1;
	}
	if (tmp) {
		// if child not capture message, but grandson (tmp) capture it, mark it
		*touch = tmp;
	}
	return 0;
}

/*
	return 1 : test succ
		0 : test failed, but *touch capture the message
 */
static int
test_animation(struct sprite *s, struct srt *srt, struct matrix * t, int x, int y, struct sprite ** touch) {
	struct pack_animation *ani = s->s.ani;
	int frame = get_frame(s);
	if (frame < 0) {
		return 0;
	}
	struct pack_frame * pf = OFFSET_TO_POINTER(struct pack_frame, s->pack, ani->frame);
	pf = &pf[frame];
	int start = pf->n-1;
	do {
		int scissor = -1;
		int i;
		// find scissor and check it first
		for (i=start;i>=0;i--) {
			struct pack_part *pp = OFFSET_TO_POINTER(struct pack_part, s->pack, pf->part);
			pp = &pp[i];
			int index = pp->component_id;
			struct sprite * c = s->data.children[index];
			if (c == NULL || (c->flags & SPRFLAG_INVISIBLE)) {
				continue;
			}
			if (c->type == TYPE_PANNEL && c->data.scissor) {
				scissor = i;
				break;
			}

		}
		if (scissor >=0) {
			struct sprite *tmp = NULL;
			check_child(s, srt, t, pf, scissor, x, y, &tmp);
			if (tmp == NULL) {
				start = scissor - 1;
				continue;
			}
		} else {
			scissor = 0;
		}
		for (i=start;i>=scissor;i--) {
			int hit = check_child(s, srt, t,  pf, i, x, y, touch);
			if (hit)
				return 1;
		}
		start = scissor - 1;
	} while(start>=0);
	return 0;
}

static int
test_child(struct sprite *s, struct srt *srt, struct matrix * ts, int x, int y, struct sprite ** touch) {
	struct matrix temp;
	struct matrix *t = mat_mul(s->t.mat, ts, &temp);
	if (s->type == TYPE_ANIMATION) {
		struct sprite *tmp = NULL;
		int testin = test_animation(s , srt, t, x,y, &tmp);
		if (testin) {
			*touch = tmp;
			return 1;
		} else if (tmp) {
			if (s->flags & SPRFLAG_MESSAGE) {
				*touch = s;
				return 1;
			} else {
				*touch = tmp;
				return 0;
			}
		}
	}
	struct matrix mat;
	if (t == NULL) {
		matrix_identity(&mat);
	} else {
		mat = *t;
	}
	matrix_srt(&mat, srt);
	struct matrix imat;
	if (matrix_inverse(&mat, &imat)) {
		// invalid matrix
		*touch = NULL;
		return 0;
	}
	int *m = imat.m;

	int xx = (x * m[0] + y * m[2]) / 1024 + m[4];
	int yy = (x * m[1] + y * m[3]) / 1024 + m[5];

	int testin;
	struct sprite * tmp = s;
	switch (s->type) {
	case TYPE_PICTURE:
		testin = test_quad(s->s.pic, xx, yy);
		break;
	case TYPE_POLYGON:
		testin = test_polygon(s->s.poly, xx, yy);
		break;
	case TYPE_LABEL:
		testin = test_label(s->s.label, xx, yy);
		break;
	case TYPE_PANNEL:
		testin = test_pannel(s->s.pannel, xx, yy);
		break;
	case TYPE_ANCHOR:
		*touch = NULL;
		return 0;
	default:
		// todo : invalid type
		*touch = NULL;
		return 0;
	}

	if (testin) {
		*touch = tmp;
		return s->flags & SPRFLAG_MESSAGE;
	} else {
		*touch = NULL;
		return 0;
	}
}

struct sprite *
sprite_test(struct sprite *s, struct srt *srt, int x, int y) {
	struct sprite *tmp = NULL;
	int testin = test_child(s, srt, NULL, x, y, &tmp);
	if (testin) {
		return tmp;
	}
	if (tmp) {
		return s;
	}
	return NULL;
}

static inline int
propagate_frame(struct sprite *s, int i, bool force_child) {
	struct sprite *child = s->data.children[i];
	if (child == NULL || child->type != TYPE_ANIMATION) {
		return 0;
	}
	if (child->flags & SPRFLAG_FORCE_INHERIT_FRAME) {
		return 1;
	}
	struct pack_animation * ani = s->s.ani;
	if (ani->component[i].id == ANCHOR_ID) {
		return 0;
	}
	if (force_child) {
		return 1;
	}
	if (ani->component[i].name == 0) {
		return 1;
	}
	return 0;
}

int
sprite_setframe(struct sprite *s, int frame, bool force_child) {
	if (s == NULL || s->type != TYPE_ANIMATION)
		return 0;
	s->frame = frame;
	int total_frame = s->total_frame;
	int i;
	struct pack_animation * ani = s->s.ani;
	for (i=0;i<ani->component_number;i++) {
		if (propagate_frame(s, i, force_child)) {
			int t = sprite_setframe(s->data.children[i],frame, force_child);
			if (t > total_frame) {
				total_frame = t;
			}
		}
	}
	return total_frame;
}


