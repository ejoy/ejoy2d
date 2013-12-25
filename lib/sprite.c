#include "sprite.h"
#include "spritepack.h"
#include "shader.h"
#include "texture.h"
#include "screen.h"
#include "matrix.h"

#include <string.h>
#include <assert.h>

void
sprite_drawquad(struct pack_picture *picture, struct srt *srt,  const struct sprite_trans *arg) {
	struct matrix tmp;
	float vb[16];
	int i,j;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	int *m = tmp.m;
	shader_program(PROGRAM_PICTURE, arg->additive);
	for (i=0;i<picture->n;i++) {
		struct pack_quad *q = &picture->quad[i];
		int glid = texture_glid(q->texid);
		if (glid == 0)
			continue;
		shader_texture(glid);
		for (j=0;j<4;j++) {
			int xx = q->screen_coord[j*2+0];
			int yy = q->screen_coord[j*2+1];
			float vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
			float tx = q->texture_coord[j*2+0];
			float ty = q->texture_coord[j*2+1];

			screen_trans(&vx,&vy);
			texture_coord(q->texid, &tx, &ty);
			vb[j*4+0] = vx;
			vb[j*4+1] = vy;
			vb[j*4+2] = tx;
			vb[j*4+3] = ty;
		}
		shader_draw(vb, arg->color);
	}
}

void 
sprite_drawpolygon(struct pack_polygon *poly, struct srt *srt, const struct sprite_trans *arg) {
	struct matrix tmp;
	int i,j;
	if (arg->mat == NULL) {
		matrix_identity(&tmp);
	} else {
		tmp = *arg->mat;
	}
	matrix_srt(&tmp, srt);
	int *m = tmp.m;
	shader_program(PROGRAM_PICTURE, arg->additive);
	for (i=0;i<poly->n;i++) {
		struct pack_poly *p = &poly->poly[i];
		int glid = texture_glid(p->texid);
		if (glid == 0)
			continue;
		shader_texture(glid);
		int pn = p->n;
		float vb[4*pn];
		for (j=0;j<pn;j++) {
			int xx = p->screen_coord[j*2+0];
			int yy = p->screen_coord[j*2+1];
			float vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
			float vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
			float tx = p->texture_coord[j*2+0];
			float ty = p->texture_coord[j*2+1];

			screen_trans(&vx,&vy);
			texture_coord(p->texid, &tx, &ty);
			vb[j*4+0] = vx;
			vb[j*4+1] = vy;
			vb[j*4+2] = tx;
			vb[j*4+3] = ty;
		}
		shader_drawpolygon(pn, vb, arg->color);
	}
}

int 
sprite_size(struct sprite_pack *pack, int id) {
	if (id < 0 || id >=	pack->n)
		return 0;
	int type = pack->type[id];
	if (type == TYPE_PICTURE || type == TYPE_POLYGON) {
		return sizeof(struct sprite) - sizeof(struct sprite *);
	} else if (type == TYPE_ANIMATION) {
		struct pack_animation * ani = pack->data[id];
		return sizeof(struct sprite) + (ani->component_number - 1) * sizeof(struct sprite *);
	}
	return 0;
}

int 
sprite_action(struct sprite *s, const char * action) {
	if (s->type != TYPE_ANIMATION) {
		return -1;
	}
	struct pack_animation *ani = s->s.ani;
	if (action == NULL) {
		s->start_frame = ani->action[0].start_frame;
		s->total_frame = ani->action[0].number;
		s->frame = 0;
		return s->total_frame;
	} else {
		int i;
		for (i=0;i<ani->action_number;i++) {
			const char *name = ani->action[i].name;
			if (name) {
				if (strcmp(name, action)==0) {
					s->start_frame = ani->action[i].start_frame;
					s->total_frame = ani->action[i].number;
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
	s->t.mat = NULL;
	s->t.color = 0xffffffff;
	s->t.additive = 0;
	s->visible = 1;
	s->name = NULL;
	s->id = id;
	s->type = pack->type[id];
	if (s->type == TYPE_PICTURE || s->type == TYPE_POLYGON) {
		s->s.pic = pack->data[id];
		s->start_frame = 0;
		s->total_frame = 0;
		s->frame = 0;
		assert(sz >= sizeof(struct sprite) - sizeof(struct sprite *));
	} else if (s->type == TYPE_ANIMATION) {
		struct pack_animation * ani = pack->data[id];
		s->s.ani = ani;
		s->frame = 0;
		sprite_action(s, NULL);
		int i;
		int n = ani->component_number;
		assert(sz >= sizeof(struct sprite) + (n - 1) * sizeof(struct sprite *));
		for (i=0; i<n ;i++) {
			s->children[i] = NULL;
		}
	} else {
		// todo : invalid type
		s->visible = 0;
	}
}

void 
sprite_mount(struct sprite *parent, int index, struct sprite *child) {
	assert(parent->type == TYPE_ANIMATION);
	struct pack_animation *ani = parent->s.ani;
	assert(index >= 0 && index < ani->component_number);
	parent->children[index] = child;
}

static int
real_frame(struct sprite *s) {
	if (s->type != TYPE_ANIMATION) {
		return 0;
	}
	int f = s->frame % s->total_frame;
	if (f < 0) {
		f += s->total_frame;
	}
	return f;
}

int 
sprite_child(struct sprite *s, const char * childname) {
	assert(childname);
	if (s->type != TYPE_ANIMATION)
		return -1;
	struct pack_animation *ani = s->s.ani;
	int i;
	for (i=0;i<ani->component_number;i++) {
		const char *name = ani->component[i].name;
		if (name) {
			if (strcmp(name, childname)==0) {
				return i;
			}
		}
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


// draw sprite

static inline uint32_t
color_mul(uint32_t c1, uint32_t c2) {
	int r1 = (c1 >> 24) & 0xff;
	int g1 = (c1 >> 16) & 0xff;
	int b1 = (c1 >> 8) & 0xff;
	int a1 = (c1) & 0xff;
	int r2 = (c2 >> 24) & 0xff;
	int g2 = (c2 >> 24) & 0xff;
	int b2 = (c2 >> 24) & 0xff;
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

static struct sprite_trans *
trans_mul(struct sprite_trans *a, struct sprite_trans *b, struct sprite_trans *t, struct matrix *tmp_matrix) {
	if (b == NULL) {
		return a;
	}
	*t = *a;
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
	return t;
}

static void 
draw_child(struct sprite *s, struct srt *srt, struct sprite_trans * ts) {
	struct sprite_trans temp;
	struct matrix temp_matrix;
	struct sprite_trans *t = trans_mul(&s->t, ts, &temp, &temp_matrix);
	switch (s->type) {
	case TYPE_PICTURE:
		sprite_drawquad(s->s.pic, srt, t);
		return;
	case TYPE_POLYGON:
		sprite_drawpolygon(s->s.poly, srt, t);
		return;
	case TYPE_ANIMATION:
		break;
	default:
		// todo : invalid type
		return;
	}
	struct pack_animation *ani = s->s.ani;
	int frame = real_frame(s) + s->start_frame;
	struct pack_frame * pf = &ani->frame[frame];
	int i;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = &pf->part[i];
		int index = pp->component_id;
		struct sprite * child = s->children[index];
		if (child == NULL || child->visible == 0) {
			continue;
		}
		struct sprite_trans temp2;
		struct matrix temp_matrix2;
		struct sprite_trans *ct = trans_mul(&pp->t, t, &temp2, &temp_matrix2);
		draw_child(child, srt, ct);
	}
}

void
sprite_draw(struct sprite *s, struct srt *srt) {
	if (s->visible) {
		draw_child(s, srt, NULL);
	}
}

void
sprite_setframe(struct sprite *s, int frame) {
	if (s == NULL || s->type != TYPE_ANIMATION)
		return;
	s->frame = frame;
	int i;
	struct pack_animation * ani = s->s.ani;
	for (i=0;i<ani->component_number;i++) {
		if (ani->component[i].name == NULL) {
			sprite_setframe(s->children[i],frame);
		}
	}
}
