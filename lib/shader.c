#include "shader.h"
#include "opengl.h"
#include "fault.h"
#include "array.h"
#include "renderbuffer.h"
#include "texture.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_PROGRAM 9

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTCOORD 1
#define ATTRIB_COLOR 2
#define ATTRIB_ADDITIVE 3

#define BUFFER_OFFSET(f) ((void *)&(((struct vertex *)NULL)->f))

struct program {
	GLuint prog;
	
	GLint mask;
	GLint st;
  float arg_mask_x;
  float arg_mask_y;
};

struct render_state {
	int current_program;
	struct program program[MAX_PROGRAM];
	int tex;
	int blendchange;
	int drawcall;
	GLuint vertex_buffer;
	GLuint index_buffer;
	struct render_buffer vb;
};

static struct render_state *RS = NULL;

void
shader_init() {
	if (RS) return;

	struct render_state * rs = (struct render_state *) malloc(sizeof(*rs));
	memset(rs, 0 , sizeof(*rs));
	rs->current_program = -1;
	rs->blendchange = 0;
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &rs->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rs->index_buffer);

	GLushort idxs[6 * MAX_COMMBINE];
	int i;
	for (i=0;i<MAX_COMMBINE;i++) {
		idxs[i*6] = i*4;
		idxs[i*6+1] = i*4+1;
		idxs[i*6+2] = i*4+2;
		idxs[i*6+3] = i*4;
		idxs[i*6+4] = i*4+2;
		idxs[i*6+5] = i*4+3;
	}
	
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

	glGenBuffers(1, &rs->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, rs->vertex_buffer);

	glEnable(GL_BLEND);

	RS = rs;
}
void
shader_reset()
{
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	if (RS->current_program != -1)
	{
		glUseProgram(RS->program[RS->current_program].prog);
	}

	glBindTexture(GL_TEXTURE_2D, RS->tex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RS->index_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, RS->vertex_buffer);
}

static GLuint
compile(const char * source, int type) {
	GLint status;
	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	
	if (status == GL_FALSE) {
		char buf[1024];
		GLint len;
		glGetShaderInfoLog(shader, 1024, &len, buf);

		fault("compile failed:%s\n"
			"source:\n %s\n",
			buf, source);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static void
link(struct program *p) {
	GLint status;
	glLinkProgram(p->prog);
	
	glGetProgramiv(p->prog, GL_LINK_STATUS, &status);
	if (status == 0) {
		char buf[1024];
		GLint len;
		glGetProgramInfoLog(p->prog, 1024, &len, buf);

		fault("link failed:%s\n", buf);
	}
}

static void
program_init(struct program * p, const char *FS, const char *VS) {
	// Create shader program.
	p->prog = glCreateProgram();
	
	GLuint fs = compile(FS, GL_FRAGMENT_SHADER);
	if (fs == 0) {
		fault("Can't compile fragment shader");
	} else {
		glAttachShader(p->prog, fs);
	}
	
	GLuint vs = compile(VS, GL_VERTEX_SHADER);
	if (vs == 0) {
		fault("Can't compile vertex shader");
	} else {
		glAttachShader(p->prog, vs);
	}

	glBindAttribLocation(p->prog, ATTRIB_VERTEX, "position");
	glBindAttribLocation(p->prog, ATTRIB_TEXTCOORD, "texcoord");
	glBindAttribLocation(p->prog, ATTRIB_COLOR, "color");
	glBindAttribLocation(p->prog, ATTRIB_ADDITIVE, "additive");

	link(p);
	
	p->mask = glGetUniformLocation(p->prog, "mask");
	p->arg_mask_x = 0.0f;
	p->arg_mask_y = 0.0f;
	if (p->mask != -1) {
		glUniform2f(p->mask, 0.0f, 0.0f);
	}
	p->st = glGetUniformLocation(p->prog, "st");
		
	glDetachShader(p->prog, fs);
	glDeleteShader(fs);
	glDetachShader(p->prog, vs);
	glDeleteShader(vs);
}

void 
shader_load(int prog, const char *fs, const char *vs) {
	struct render_state *rs = RS;
	assert(prog >=0 && prog < MAX_PROGRAM);
	struct program * p = &rs->program[prog];
	assert(p->prog == 0);
	program_init(p, fs, vs);
}

void 
shader_unload() {
	if (RS == NULL) {
		return;
	}
	int i;

	for (i=0;i<MAX_PROGRAM;i++) {
		struct program * p = &RS->program[i];
		if (p->prog) {
			glDeleteProgram(p->prog);
		}
	}

	glDeleteBuffers(1,&RS->vertex_buffer);
	glDeleteBuffers(1,&RS->index_buffer);
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
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(vp.vx));
	glEnableVertexAttribArray(ATTRIB_TEXTCOORD);
	glVertexAttribPointer(ATTRIB_TEXTCOORD, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(struct vertex), BUFFER_OFFSET(vp.tx));
	glEnableVertexAttribArray(ATTRIB_COLOR);
	glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), BUFFER_OFFSET(rgba));
	glEnableVertexAttribArray(ATTRIB_ADDITIVE);
	glVertexAttribPointer(ATTRIB_ADDITIVE, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), BUFFER_OFFSET(add));
	glDrawElements(GL_TRIANGLES, 6 * rb->object, GL_UNSIGNED_SHORT, 0);
}

static void
rs_commit() {
	struct render_buffer * rb = &(RS->vb);
	if (rb->object == 0)
		return;
	RS->drawcall++;
	glBindBuffer(GL_ARRAY_BUFFER, RS->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct quad) * rb->object, rb->vb, GL_DYNAMIC_DRAW);

	renderbuffer_commit(rb);

	rb->object = 0;
}

void 
shader_drawbuffer(struct render_buffer * rb, float tx, float ty, float scale) {
	rs_commit();
	int glid = texture_glid(rb->texid);
	if (glid == 0)
		return;
	shader_texture(glid);
	shader_program(PROGRAM_RENDERBUFFER);
	RS->drawcall++;
	glBindBuffer(GL_ARRAY_BUFFER, rb->vbid);

	float sx = scale;
	float sy = scale;
	screen_trans(&sx, &sy);
	screen_trans(&tx, &ty);
	struct program *p = &RS->program[RS->current_program];
	glUniform4f(p->st, sx, sy, tx, ty);

	renderbuffer_commit(rb);
}

void
shader_texture(int id) {
	if (RS->tex != id) {
		rs_commit();
		RS->tex = (GLuint)id;
		glBindTexture(GL_TEXTURE_2D, id);
	}
}

void
shader_program(int n) {
	if (RS->current_program != n) {
		rs_commit();
		RS->current_program = n;
		glUseProgram(RS->program[n].prog);
	}
}

void
shader_mask(float x, float y) {
	struct program *p = &RS->program[RS->current_program];
	if (!p || p->mask == -1)
		return;
	if (p->arg_mask_x == x && p->arg_mask_y == y)
		return;
	p->arg_mask_x = x;
	p->arg_mask_y = y;
//  rs_commit();
	glUniform2f(p->mask, x, y);
}

void
shader_st(int prog, float x, float y, float scale) {
	rs_commit();
    shader_program(prog);
    struct program *p = &RS->program[prog];

    if (!p || p->st == -1)
        return;

    glUniform4f(p->st, scale, scale, x, y);
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
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void
shader_blend(int m1, int m2) {
	if (m1 != GL_ONE || m2 != GL_ONE_MINUS_SRC_ALPHA) {
		rs_commit();
		RS->blendchange = 1;
		glBlendFunc(m1,m2);
	}
}
