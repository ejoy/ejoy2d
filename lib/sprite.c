#include "sprite.h"
#include "spritepack.h"
#include "shader.h"
#include "texture.h"
#include "screen.h"
#include "matrix.h"
#include "label.h"
#include "scissor.h"
#include "array.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

void
sprite_drawquad(struct pack_picture *picture, const struct srt *srt,  const struct sprite_trans *arg) {
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
	for (i=0;i<picture->n;i++) {
		struct pack_quad *q = &picture->rect[i];
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
		shader_texture(glid);
		int pn = p->n;

		ARRAY(float, vb, 4 * pn);

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
	if (type == TYPE_ANIMATION) {
		struct pack_animation * ani = (struct pack_animation *)pack->data[id];
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
	s->parent = NULL;
	s->t.mat = NULL;
	s->t.color = 0xffffffff;
	s->t.additive = 0;
	s->t.program = PROGRAM_DEFAULT;
	s->message = false;
	s->visible = true;
	s->name = NULL;
	s->id = id;
	s->type = pack->type[id];
	if (s->type == TYPE_ANIMATION) {
		struct pack_animation * ani = (struct pack_animation *)pack->data[id];
		s->s.ani = ani;
		s->frame = 0;
		sprite_action(s, NULL);
		int i;
		int n = ani->component_number;
		assert(sz >= sizeof(struct sprite) + (n - 1) * sizeof(struct sprite *));
		for (i=0; i<n ;i++) {
			s->data.children[i] = NULL;
		}
	} else {
		s->s.pic = (struct pack_picture *)pack->data[id];
		s->start_frame = 0;
		s->total_frame = 0;
		s->frame = 0;
		s->data.text = NULL;
		assert(sz >= sizeof(struct sprite) - sizeof(struct sprite *));
		if (s->type == TYPE_PANNEL) {
			struct pack_pannel * pp = (struct pack_pannel *)pack->data[id];
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
		child->name = ani->component[index].name;
		child->parent = parent;
		if (oldc && oldc->type == TYPE_ANCHOR)
			child->message = oldc->message;
	}
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

const char *
sprite_childname(struct sprite *s, int index) {
	if (s->type != TYPE_ANIMATION)
		return NULL;
	struct pack_animation *ani = s->s.ani;
	if (index < 0 || index >= ani->component_number)
		return NULL;
	return ani->component[index].name;
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
switch_program(struct sprite_trans *t, int def) {
	int prog = t->program;
	if (prog == PROGRAM_DEFAULT) {
		prog = def;
	}
	shader_program(prog, t->additive);
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

static int
draw_child(struct sprite *s, struct srt *srt, struct sprite_trans * ts) {
	struct sprite_trans temp;
	struct matrix temp_matrix;
	struct sprite_trans *t = trans_mul(&s->t, ts, &temp, &temp_matrix);
	switch (s->type) {
	case TYPE_PICTURE:
		switch_program(t, PROGRAM_PICTURE);
		sprite_drawquad(s->s.pic, srt, t);
		return 0;
	case TYPE_POLYGON:
		switch_program(t, PROGRAM_PICTURE);
		sprite_drawpolygon(s->s.poly, srt, t);
		return 0;
	case TYPE_LABEL:
		if (s->data.text) {
			t->program = PROGRAM_DEFAULT;	// label never set user defined program
			switch_program(t, s->s.label->edge ? PROGRAM_TEXT_EDGE : PROGRAM_TEXT);
			label_draw(s->data.text, s->s.label,srt,t);
		}
		return 0;
	case TYPE_ANCHOR:
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
	struct pack_animation *ani = s->s.ani;
	int frame = real_frame(s) + s->start_frame;
	struct pack_frame * pf = &ani->frame[frame];
	int i;
	int scissor = 0;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = &pf->part[i];
		int index = pp->component_id;
		struct sprite * child = s->data.children[index];
		if (child == NULL || child->visible == false) {
			continue;
		}
		struct sprite_trans temp2;
		struct matrix temp_matrix2;
		struct sprite_trans *ct = trans_mul(&pp->t, t, &temp2, &temp_matrix2);
		scissor += draw_child(child, srt, ct);
	}
	for (i=0;i<scissor;i++) {
		scissor_pop();
	}
	return 0;
}

bool
sprite_child_visible(struct sprite *s, const char * childname) {
	struct pack_animation *ani = s->s.ani;
	int frame = real_frame(s) + s->start_frame;
	struct pack_frame * pf = &ani->frame[frame];
	int i;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = &pf->part[i];
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
	if (s->visible) {
		draw_child(s, srt, NULL);
	}
}

void
sprite_draw_as_child(struct sprite *s, struct srt *srt, struct matrix *mat, uint32_t color) {
	if (s->visible) {
		struct sprite_trans st;
		st.mat = mat;
		st.color = color;
		st.additive = 0;
		st.program = PROGRAM_DEFAULT;
		draw_child(s, srt, &st);
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
	int frame = real_frame(s) + s->start_frame;
	struct pack_frame * pf = &ani->frame[frame];
	int i;
	for (i=0;i<pf->n;i++) {
		struct pack_part *pp = &pf->part[i];
		int index = pp->component_id;
		struct sprite * child = s->data.children[index];
		if (child == NULL || child->visible == false) {
			continue;
		}
		struct matrix temp2;
		struct matrix *ct = mat_mul(pp->t.mat, t, &temp2);
		if (child_aabb(child, srt, ct, aabb))
			break;
	}
	return 0;
}

void
sprite_aabb(struct sprite *s, struct srt *srt, int aabb[4]) {
	int i;
	if (s->visible) {
		aabb[0] = INT_MAX;
		aabb[1] = INT_MAX;
		aabb[2] = INT_MIN;
		aabb[3] = INT_MIN;
		child_aabb(s,srt,NULL,aabb);
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
	struct pack_part *pp = &pf->part[i];
	int index = pp->component_id;
	struct sprite * child = s->data.children[index];
	if (child == NULL || !child->visible) {
		return 0;
	}
	struct matrix temp2;
	struct matrix *ct = mat_mul(pp->t.mat, t, &temp2);
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
	int frame = real_frame(s) + s->start_frame;
	struct pack_frame * pf = &ani->frame[frame];
	int start = pf->n-1;
	do {
		int scissor = -1;
		int i;
		// find scissor and check it first
		for (i=start;i>=0;i--) {
			struct pack_part *pp = &pf->part[i];
			int index = pp->component_id;
			struct sprite * c = s->data.children[index];
			if (c == NULL || !c->visible) {
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
			if (s->message) {
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
		return s->message;
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

int
sprite_setframe(struct sprite *s, int frame, bool force_child) {
	if (s == NULL || s->type != TYPE_ANIMATION)
		return 0;
	s->frame = frame;
	int total_frame = s->total_frame;
	int i;
	struct pack_animation * ani = s->s.ani;
	for (i=0;i<ani->component_number;i++) {
		if (force_child || ani->component[i].name == NULL) {
			int t = sprite_setframe(s->data.children[i],frame, force_child);
			if (t > total_frame) {
				total_frame = t;
			}
		}
	}
	return total_frame;
}
