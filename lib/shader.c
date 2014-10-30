#include "shader.h"
#include "fault.h"
#include "array.h"
#include "renderbuffer.h"
#include "texture.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"
#include "label.h"

#include "render.h"
#include "blendmode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_PROGRAM 16

#define BUFFER_OFFSET(f) ((int)&(((struct vertex *)NULL)->f))

#define MAX_UNIFORM 16

struct uniform {
	int loc;
	enum UNIFORM_FORMAT type;
};

struct program {
	RID prog;
	int mask;
	int st;
	int uniform_number;
	struct uniform uniform[MAX_UNIFORM];
};

struct render_state {
	struct render * R;
	int current_program;
	struct program program[MAX_PROGRAM];
	RID tex;
	int blendchange;
	int drawcall;
	RID vertex_buffer;
	RID index_buffer;
	RID layout;
	struct render_buffer vb;
};

static struct render_state *RS = NULL;

void
shader_init() {
	if (RS) return;

	struct render_state * rs = (struct render_state *) malloc(sizeof(*rs));
	memset(rs, 0 , sizeof(*rs));

	struct render_init_args RA;
	// todo: config these args
	RA.max_buffer = 128;
	RA.max_layout = 4;
	RA.max_target = 128;
	RA.max_texture = 256;
	RA.max_shader = MAX_PROGRAM;

	int rsz = render_size(&RA);
	rs->R = (struct render *)malloc(rsz);
	rs->R = render_init(&RA, rs->R, rsz);
	texture_initrender(rs->R);
	screen_initrender(rs->R);
	label_initrender(rs->R);
	renderbuffer_initrender(rs->R);

	rs->current_program = -1;
	rs->blendchange = 0;
	render_setblend(rs->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);

	uint16_t idxs[6 * MAX_COMMBINE];
	int i;
	for (i=0;i<MAX_COMMBINE;i++) {
		idxs[i*6] = i*4;
		idxs[i*6+1] = i*4+1;
		idxs[i*6+2] = i*4+2;
		idxs[i*6+3] = i*4;
		idxs[i*6+4] = i*4+2;
		idxs[i*6+5] = i*4+3;
	}
	
	rs->index_buffer = render_buffer_create(rs->R, INDEXBUFFER, idxs, 6 * MAX_COMMBINE, sizeof(uint16_t));
	rs->vertex_buffer = render_buffer_create(rs->R, VERTEXBUFFER, NULL,  4 * MAX_COMMBINE, sizeof(struct vertex));

	struct vertex_attrib va[4] = {
		{ "position", 0, 2, sizeof(float), BUFFER_OFFSET(vp.vx) },
		{ "texcoord", 0, 2, sizeof(uint16_t), BUFFER_OFFSET(vp.tx) },
		{ "color", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(rgba) },
		{ "additive", 0, 4, sizeof(uint8_t), BUFFER_OFFSET(add) },
	};
	rs->layout = render_register_vertexlayout(rs->R, sizeof(va)/sizeof(va[0]), va);
	render_set(rs->R, VERTEXLAYOUT, rs->layout, 0);
	render_set(rs->R, INDEXBUFFER, rs->index_buffer, 0);
	render_set(rs->R, VERTEXBUFFER, rs->vertex_buffer, 0);

	RS = rs;
}

void
shader_reset() {
	struct render_state *rs = RS;
	render_state_reset(rs->R);
	render_setblend(rs->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	if (RS->current_program != -1) {
		render_shader_bind(rs->R, RS->program[RS->current_program].prog);
	}
	render_set(rs->R, VERTEXLAYOUT, rs->layout, 0);
	render_set(rs->R, TEXTURE, RS->tex, 0);
	render_set(rs->R, INDEXBUFFER, RS->index_buffer,0);
	render_set(rs->R, VERTEXBUFFER, RS->vertex_buffer,0);
}

static void
program_init(struct program * p, const char *FS, const char *VS) {
	struct render *R = RS->R;
	p->prog = render_shader_create(R, VS, FS);
	render_shader_bind(R, p->prog);

	p->mask = render_shader_locuniform(R, "mask");
	p->st = render_shader_locuniform(R, "st");
	render_shader_bind(R, 0);
}

void 
shader_load(int prog, const char *fs, const char *vs) {
	struct render_state *rs = RS;
	assert(prog >=0 && prog < MAX_PROGRAM);
	struct program * p = &rs->program[prog];
	if (p->prog) {
		render_release(RS->R, SHADER, p->prog);
		p->prog = 0;
	}
	program_init(p, fs, vs);
	RS->current_program = -1;
}

void 
shader_unload() {
	if (RS == NULL) {
		return;
	}
	struct render *R = RS->R;
	texture_initrender(NULL);
	screen_initrender(NULL);
	label_initrender(NULL);
	renderbuffer_initrender(NULL);

	render_exit(R);
	free(R);
	free(RS);
	RS = NULL;
}

void
reset_drawcall_count() {
	if (RS) {
		RS->drawcall = 0;
	}
}

int
drawcall_count() {
	if (RS) {
		return RS->drawcall;
	} else {
		return 0;
	}
}

static void 
renderbuffer_commit(struct render_buffer * rb) {
	struct render *R = RS->R;
	render_draw(R, DRAW_TRIANGLE, 0, 6 * rb->object);
}

static void
rs_commit() {
	struct render_buffer * rb = &(RS->vb);
	if (rb->object == 0)
		return;
	RS->drawcall++;
	struct render *R = RS->R;
	render_buffer_update(R, RS->vertex_buffer, rb->vb, 4 * rb->object);
	renderbuffer_commit(rb);

	rb->object = 0;
}

void 
shader_drawbuffer(struct render_buffer * rb, float tx, float ty, float scale) {
	rs_commit();

	RID glid = texture_glid(rb->texid);
	if (glid == 0)
		return;
	shader_texture(glid);
	shader_program(PROGRAM_RENDERBUFFER);
	RS->drawcall++;
	render_set(RS->R, VERTEXBUFFER, rb->vbid, 0);

	float sx = scale;
	float sy = scale;
	screen_trans(&sx, &sy);
	screen_trans(&tx, &ty);
	struct program *p = &RS->program[RS->current_program];
	float v[4] = { sx, sy, tx, ty };
	render_shader_setuniform(RS->R, p->st, UNIFORM_FLOAT4, v);

	renderbuffer_commit(rb);

	render_set(RS->R, VERTEXBUFFER, RS->vertex_buffer, 0);
}

void
shader_texture(int id) {
	if (RS->tex != id) {
		rs_commit();
		RS->tex = id;
		render_set(RS->R, TEXTURE, id, 0);
	}
}

void
shader_program(int n) {
	if (RS->current_program != n) {
		rs_commit();
		RS->current_program = n;
		render_shader_bind(RS->R, RS->program[RS->current_program].prog);
	}
}

void
shader_mask(float x, float y) {
	struct program *p = &RS->program[RS->current_program];
	if (!p || p->mask == -1)
		return;
	float v[2] = { x, y};
	render_shader_setuniform(RS->R, p->mask, UNIFORM_FLOAT2, v);
}

void
shader_st(int prog, float x, float y, float scale) {
	rs_commit();
    shader_program(prog);
    struct program *p = &RS->program[prog];

    if (!p || p->st == -1)
        return;
	float v[4] = { scale, scale, x, y };
	render_shader_setuniform(RS->R, p->mask, UNIFORM_FLOAT4, v);
}

void
shader_draw(const struct vertex_pack vb[4], uint32_t color, uint32_t additive) {
	if (renderbuffer_add(&RS->vb, vb, color, additive)) {
		rs_commit();
	}
}

static void
draw_quad(const struct vertex_pack *vbp, uint32_t color, uint32_t additive, int max, int index) {
	struct vertex_pack vb[4];
	int i;
	vb[0] = vbp[0];	// first point
	for (i=1;i<4;i++) {
		int j = i + index;
		int n = (j <= max) ? j : max;
		vb[i] = vbp[n];
	}
	shader_draw(vb, color, additive);
}

void
shader_drawpolygon(int n, const struct vertex_pack *vb, uint32_t color, uint32_t additive) {
	int i = 0;
	--n;
	do {
		draw_quad(vb, color, additive, n, i);
		i+=2;
	} while (i<n-1);
}

void 
shader_flush() {
	rs_commit();
}

void
shader_defaultblend() {
	if (RS->blendchange) {
		rs_commit();
		RS->blendchange = 0;
		render_setblend(RS->R, BLEND_ONE, BLEND_ONE_MINUS_SRC_ALPHA);
	}
}

void
shader_blend(int m1, int m2) {
	if (m1 != BLEND_GL_ONE || m2 != BLEND_GL_ONE_MINUS_SRC_ALPHA) {
		rs_commit();
		RS->blendchange = 1;
		enum BLEND_FORMAT src = blend_mode(m1);
		enum BLEND_FORMAT dst = blend_mode(m2);
		render_setblend(RS->R, src, dst);
	}
}

void 
shader_clear(unsigned long argb) {
	render_clear(RS->R, MASKC, argb);
}

int 
shader_version() {
	return render_version(RS->R);
}

void 
shader_scissortest(int enable) {
	render_enablescissor(RS->R, enable);
}

void 
shader_setuniform(int index, enum UNIFORM_FORMAT t, float *v) {
	rs_commit();
	int prog = RS->current_program;
	assert(prog >=0 && prog < MAX_PROGRAM);
	struct program * p = &RS->program[prog];
	assert(index >= 0 && index < p->uniform_number);
	struct uniform *u = &p->uniform[index];
	assert(t == u->type);
	render_shader_setuniform(RS->R, u->loc, t, v);
}

int 
shader_adduniform(int prog, const char * name, enum UNIFORM_FORMAT t) {
	// reset current_program
	assert(prog >=0 && prog < MAX_PROGRAM);
	shader_program(prog);
	struct program * p = &RS->program[prog];
	assert(p->uniform_number < MAX_UNIFORM);
	int loc = render_shader_locuniform(RS->R, name);
	if (loc < 0)
		return -1;
	int index = p->uniform_number++;
	struct uniform * u = &p->uniform[index];
	u->loc = loc;
	u->type = t;
	return index;
}
