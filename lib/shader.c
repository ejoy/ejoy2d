#include "shader.h"
#include "opengl.h"
#include "fault.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_COMMBINE 64
#define MAX_PROGRAM 3

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTCOORD 1
#define ATTRIB_COLOR 2

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

struct program {
	GLuint prog;
	GLint additive;
	uint32_t arg;
};

struct vertex {
	float vx;
	float vy;
	float tx;
	float ty;
	uint8_t rgba[4];
};

struct quad {
	struct vertex p[4];
};

struct render_state {
	int current_program;
	struct program program[MAX_PROGRAM];
	int tex;
	int object;
	int blendchange;
	GLuint vertex_buffer;
	GLuint index_buffer;
	struct quad vb[MAX_COMMBINE];
};

static struct render_state *RS = NULL;

void
shader_init() {
	assert(RS == NULL);
	struct render_state * rs = (struct render_state *) malloc(sizeof(*rs));
	memset(rs, 0 , sizeof(*rs));
	rs->current_program = -1;
	rs->blendchange = 0;
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &rs->index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rs->index_buffer);

	GLubyte idxs[6 * MAX_COMMBINE];
	int i;
	for (i=0;i<MAX_COMMBINE;i++) {
		idxs[i*6] = i*4;
		idxs[i*6+1] = i*4+1;
		idxs[i*6+2] = i*4+2;
		idxs[i*6+3] = i*4;
		idxs[i*6+4] = i*4+2;
		idxs[i*6+5] = i*4+3;
	}
	
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*MAX_COMMBINE, idxs, GL_STATIC_DRAW);

	glGenBuffers(1, &rs->vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, rs->vertex_buffer);

	glEnable(GL_BLEND);

	RS = rs;
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
		fault("Can't link program");
	}
}

static void
set_color(GLint addi, uint32_t color) {
	if (addi == -1)
		return;
	if (color == 0) {
		glUniform3f(addi, 0,0,0);
	} else {
		float c[3];
		c[0] = (float)((color >> 16) & 0xff) / 255.0f;
		c[1] = (float)((color >> 8) & 0xff) / 255.0f;
		c[2] = (float)(color & 0xff ) / 255.0f;
		glUniform3f(addi, c[0],c[1],c[2]);
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

	link(p);

	p->additive = glGetUniformLocation(p->prog, "additive");
	p->arg = 0;
	set_color(p->additive, 0);
	
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

static void
rs_commit() {
	if (RS->object == 0)
		return;
	glBindBuffer(GL_ARRAY_BUFFER, RS->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct quad) * RS->object, RS->vb, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(ATTRIB_TEXTCOORD);
	glVertexAttribPointer(ATTRIB_TEXTCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(8));
	glEnableVertexAttribArray(ATTRIB_COLOR);
	glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), BUFFER_OFFSET(16));
	glDrawElements(GL_TRIANGLES, 6 * RS->object, GL_UNSIGNED_BYTE, 0);
	RS->object = 0;
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
shader_program(int n, uint32_t arg) {
	if (RS->current_program != n) {
		rs_commit();
		RS->current_program = n;
		glUseProgram(RS->program[n].prog);
	}
	struct program *p = &RS->program[RS->current_program];
	if (p->arg != arg) {
		rs_commit();
		p->arg = arg;
		set_color(p->additive, arg);
	}
}

void
shader_draw(const float vb[16], uint32_t color) {
	struct quad *q = RS->vb + RS->object;
	int i;
	for (i=0;i<4;i++) {
		q->p[i].vx = vb[i*4+0];
		q->p[i].vy = vb[i*4+1];
		q->p[i].tx = vb[i*4+2];
		q->p[i].ty = vb[i*4+3];
		q->p[i].rgba[0] = (color >> 16) & 0xff;
		q->p[i].rgba[1] = (color >> 8) & 0xff;
		q->p[i].rgba[2] = (color) & 0xff;
		q->p[i].rgba[3] = (color >> 24) & 0xff;
	}
	if (++RS->object >= MAX_COMMBINE) {
		rs_commit();
	}
}

void
shader_drawpolygon(int n, const float *vb, uint32_t color) {
	rs_commit();
	struct vertex p[n];
	int i;
	for (i=0;i<n;i++) {
		p[i].vx = vb[i*4+0];
		p[i].vy = vb[i*4+1];
		p[i].tx = vb[i*4+2];
		p[i].ty = vb[i*4+3];
		p[i].rgba[0] = (color >> 16) & 0xff;
		p[i].rgba[1] = (color >> 8) & 0xff;
		p[i].rgba[2] = (color) & 0xff;
		p[i].rgba[3] = (color >> 24) & 0xff;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, RS->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex) * n, (void*)p, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(ATTRIB_TEXTCOORD);
	glVertexAttribPointer(ATTRIB_TEXTCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(8));
	glEnableVertexAttribArray(ATTRIB_COLOR);
	glVertexAttribPointer(ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), BUFFER_OFFSET(16));
	glDrawArrays(GL_TRIANGLE_FAN, 0, n);
}

void 
shader_flush() {
	rs_commit();
}

void
shader_defaultblend() {
	if (RS->blendchange) {
		rs_commit();
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
