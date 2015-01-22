#include "render.h"
#include "opengl.h"
#include "carray.h"
#include "block.h"
#include "log.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define MAX_VB_SLOT 8
#define MAX_ATTRIB 16
#define MAX_TEXTURE 8
#define CHANGE_INDEXBUFFER 0x1
#define CHANGE_VERTEXBUFFER 0x2
#define CHANGE_TEXTURE 0x4
#define CHANGE_BLEND 0x8
#define CHANGE_DEPTH 0x10
#define CHANGE_CULL 0x20
#define CHANGE_TARGET 0x40
#define CHANGE_SCISSOR 0x80

//#define CHECK_GL_ERROR
//#define CHECK_GL_ERROR assert(check_opengl_error());
#define CHECK_GL_ERROR check_opengl_error_debug((struct render *)R, __FILE__, __LINE__);

struct buffer {
	GLuint glid;
	GLenum gltype;
	int n;
	int stride;
};

struct attrib {
	int n;
	struct vertex_attrib a[MAX_ATTRIB];
};

struct target {
	GLuint glid;
	RID tex;
};

struct texture {
	GLuint glid;
	int width;
	int height;
	int mipmap;
	enum TEXTURE_FORMAT format;
	enum TEXTURE_TYPE type;
	int memsize;
};

struct attrib_layout {
	int vbslot;
	GLint size;
 	GLenum type;
 	GLboolean normalized;
	int offset;
};

struct shader {
	GLuint glid;
	int n;
	struct attrib_layout a[MAX_ATTRIB];
	int texture_n;
	int texture_uniform[MAX_TEXTURE];
};

struct rstate {
	RID indexbuffer;
	RID target;
	enum BLEND_FORMAT blend_src;
	enum BLEND_FORMAT blend_dst;
	enum DEPTH_FORMAT depth;
	enum CULL_MODE cull;
	int depthmask;
	int scissor;
	RID texture[MAX_TEXTURE];
};

struct render {
	uint32_t changeflag;
	RID attrib_layout;
	RID vbslot[MAX_VB_SLOT];
	RID program;
	GLint default_framebuffer;
	struct rstate current;
	struct rstate last;
	struct log log;
	struct array buffer;
	struct array attrib;
	struct array target;
	struct array texture;
	struct array shader;
};

static void
check_opengl_error_debug(struct render *R, const char *filename, int line) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR
//		&& error != GL_INVALID_ENUM 
//		&& error != GL_INVALID_VALUE
//		&& error != GL_INVALID_OPERATION
//		&& error != GL_OUT_OF_MEMORY
//		&& error != GL_STACK_OVERFLOW 
//		&& error != GL_STACK_UNDERFLOW
	) {
		log_printf(&R->log, "GL_ERROR (0x%x) @ %s : %d\n", error, filename, line);
		exit(1);
	}
}

// what should be VERTEXBUFFER or INDEXBUFFER
RID 
render_buffer_create(struct render *R, enum RENDER_OBJ what, const void *data, int n, int stride) {
	GLenum gltype;
	switch(what) {
	case VERTEXBUFFER:
		gltype = GL_ARRAY_BUFFER;
		break;
	case INDEXBUFFER:
		gltype = GL_ELEMENT_ARRAY_BUFFER;
		break;
	default:
		return 0;
	}
	struct buffer * buf = (struct buffer *)array_alloc(&R->buffer);
	if (buf == NULL)
		return 0;
	glGenBuffers(1, &buf->glid);
	glBindBuffer(gltype, buf->glid);
	if (data) {
		glBufferData(gltype, n * stride, data, GL_STATIC_DRAW);
		buf->n = n;
	} else {
		buf->n = 0;
	}
	buf->gltype = gltype;
	buf->stride = stride;

	CHECK_GL_ERROR

	return array_id(&R->buffer, buf);
}

void 
render_buffer_update(struct render *R, RID id, const void * data, int n) {
	struct buffer * buf = (struct buffer *)array_ref(&R->buffer, id);
	glBindBuffer(buf->gltype, buf->glid);
	buf->n = n;
	glBufferData(buf->gltype, n * buf->stride, data, GL_DYNAMIC_DRAW);
	CHECK_GL_ERROR
}

static void
close_buffer(void *p, void *R) {
	struct buffer * buf = (struct buffer *)p;
	glDeleteBuffers(1,&buf->glid);

	CHECK_GL_ERROR
}

RID 
render_register_vertexlayout(struct render *R, int n, struct vertex_attrib * attrib) {
	assert(n <= MAX_ATTRIB);
	struct attrib * a = (struct attrib*)array_alloc(&R->attrib);
	if (a == NULL)
		return 0;

	a->n = n;
	memcpy(a->a, attrib, n * sizeof(struct vertex_attrib));

	RID id = array_id(&R->attrib, a);

	R->attrib_layout = id;

	return id;
}

static GLuint
compile(struct render *R, const char * source, int type) {
	GLint status;
	
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	
	if (status == GL_FALSE) {
		char buf[1024];
		GLint len;
		glGetShaderInfoLog(shader, 1024, &len, buf);

		log_printf(&R->log, "compile failed:%s\n"
			"source:\n %s\n",
			buf, source);
		glDeleteShader(shader);
		return 0;
	}

	CHECK_GL_ERROR

	return shader;
}

static int
link(struct render *R, GLuint prog) {
	GLint status;
	glLinkProgram(prog);
	
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == 0) {
		char buf[1024];
		GLint len;
		glGetProgramInfoLog(prog, 1024, &len, buf);

		log_printf(&R->log, "link failed:%s\n", buf);

		return 0;
	}

	CHECK_GL_ERROR

	return 1;
}

static int
compile_link(struct render *R, struct shader *s, const char * VS, const char *FS) {
	GLuint fs = compile(R, FS, GL_FRAGMENT_SHADER);
	if (fs == 0) {
		log_printf(&R->log, "Can't compile fragment shader");
		return 0;
	} else {
		glAttachShader(s->glid, fs);
	}
	
	GLuint vs = compile(R, VS, GL_VERTEX_SHADER);
	if (vs == 0) {
		log_printf(&R->log, "Can't compile vertex shader");
		return 0;
	} else {
		glAttachShader(s->glid, vs);
	}

	if (R->attrib_layout == 0)
		return 0;

	struct attrib * a = (struct attrib *)array_ref(&R->attrib, R->attrib_layout);
	s->n = a->n;
	int i;
	for (i=0;i<a->n;i++) {
		struct vertex_attrib *va = &a->a[i];
		struct attrib_layout *al = &s->a[i];
		glBindAttribLocation(s->glid, i, va->name);
		al->vbslot = va->vbslot;
		al->offset = va->offset;
		al->size = va->n;
		switch (va->size) {
		case 1:
			al->type = GL_UNSIGNED_BYTE;
			al->normalized = GL_TRUE;
			break;
		case 2:
			al->type = GL_UNSIGNED_SHORT;
			al->normalized = GL_TRUE;
			break;
		case 4:
			al->type = GL_FLOAT;
			al->normalized = GL_FALSE;
			break;
		default:
			return 0;
		}
	}

	return link(R, s->glid);
}

RID 
render_shader_create(struct render *R, struct shader_init_args *args) {
	struct shader * s = (struct shader *)array_alloc(&R->shader);
	if (s == NULL) {
		return 0;
	}
	s->glid = glCreateProgram();
	if (!compile_link(R, s, args->vs, args->fs)) {
		glDeleteProgram(s->glid);
		array_free(&R->shader, s);
		return 0;
	}

	s->texture_n = args->texture;
	int i;
	for (i=0;i<s->texture_n;i++) {
		s->texture_uniform[i] = glGetUniformLocation(s->glid, args->texture_uniform[i]);
	}

	CHECK_GL_ERROR

	return array_id(&R->shader, s);
}

static void
close_shader(void *p, void *R) {
	struct shader * shader = (struct shader *)p;
	glDeleteProgram(shader->glid);

	CHECK_GL_ERROR
}

static void
close_texture(void *p, void *R) {
	struct texture * tex = (struct texture *)p;
	glDeleteTextures(1,&tex->glid);

	CHECK_GL_ERROR
}

static void
close_target(void *p, void *R) {
	struct target * tar = (struct target *)p;
	glDeleteFramebuffers(1, &tar->glid);

	CHECK_GL_ERROR
}

void 
render_release(struct render *R, enum RENDER_OBJ what, RID id) {
	switch (what) {
	case VERTEXBUFFER:
	case INDEXBUFFER: {
		struct buffer * buf = (struct buffer *)array_ref(&R->buffer, id);
		if (buf) {
			close_buffer(buf, R);
			array_free(&R->buffer, buf);
		}
		break;
	}
	case SHADER: {
		struct shader * shader = (struct shader *)array_ref(&R->shader, id);
		if (shader) {
			close_shader(shader, R);
			array_free(&R->shader, shader);
		}
		break;
	}
	case TEXTURE : {
		struct texture * tex = (struct texture *) array_ref(&R->texture, id);
		if (tex) {
			close_texture(tex, R);
			array_free(&R->texture, tex);
		}
		break;
	}
	case TARGET : {
		struct target * tar = (struct target *)array_ref(&R->target, id);
		if (tar) {
			close_target(tar, R);
			array_free(&R->target, tar);
		}
		break;
	}
	default:
		assert(0);
		break;
	}
}

void 
render_set(struct render *R, enum RENDER_OBJ what, RID id, int slot) {
	switch (what) {
	case VERTEXBUFFER:
		assert(slot >= 0 && slot < MAX_VB_SLOT);
		R->vbslot[slot] = id;
		R->changeflag |= CHANGE_VERTEXBUFFER;
		break;
	case INDEXBUFFER:
		R->current.indexbuffer = id;
		R->changeflag |= CHANGE_INDEXBUFFER;
		break;
	case VERTEXLAYOUT:
		R->attrib_layout = id;
		break;
	case TEXTURE:
		assert(slot >= 0 && slot < MAX_TEXTURE);
		R->current.texture[slot] = id;
		R->changeflag |= CHANGE_TEXTURE;
		break;
	case TARGET:
		R->current.target = id;
		R->changeflag |= CHANGE_TARGET;
		break;
	default:
		assert(0);
		break;
	}
}

static void
apply_texture_uniform(struct shader *s) {
	int i;
	for (i=0;i<s->texture_n;i++) {
		int loc = s->texture_uniform[i];
		if (loc >= 0) {
			glUniform1i(loc, i);
		}
	}
}

void
render_shader_bind(struct render *R, RID id) {
	R->program = id;
	R->changeflag |= CHANGE_VERTEXBUFFER;
	struct shader * s = (struct shader *)array_ref(&R->shader, id);
	if (s) {
		glUseProgram(s->glid);
		apply_texture_uniform(s);
	} else {
		glUseProgram(0);
	}

	CHECK_GL_ERROR
}

int 
render_size(struct render_init_args *args) {
	return sizeof(struct render) + 
		array_size(args->max_buffer, sizeof(struct buffer)) +
		array_size(args->max_layout, sizeof(struct attrib)) +
		array_size(args->max_target, sizeof(struct target)) +
		array_size(args->max_texture, sizeof(struct texture)) +
		array_size(args->max_shader, sizeof(struct shader));
}

static void
new_array(struct block *B, struct array *A, int n, int sz) {
	int s = array_size(n, sz);
	void * buffer = block_slice(B, s);
	array_init(A, buffer, n, sz);
}

struct render * 
render_init(struct render_init_args *args, void * buffer, int sz) {
	struct block B;
	block_init(&B, buffer, sz);
	struct render * R = (struct render *)block_slice(&B, sizeof(struct render));
	memset(R, 0, sizeof(*R));
	log_init(&R->log, stderr);
	new_array(&B, &R->buffer, args->max_buffer, sizeof(struct buffer));
	new_array(&B, &R->attrib, args->max_layout, sizeof(struct attrib));
	new_array(&B, &R->target, args->max_target, sizeof(struct target));
	new_array(&B, &R->texture, args->max_texture, sizeof(struct texture));
	new_array(&B, &R->shader, args->max_shader, sizeof(struct shader));

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &R->default_framebuffer);

	CHECK_GL_ERROR

	return R;
}

void 
render_exit(struct render * R) {
	array_exit(&R->buffer, close_buffer, R);
	array_exit(&R->shader, close_shader, R);
	array_exit(&R->texture, close_texture, R);
	array_exit(&R->target, close_target, R);
}

void 
render_setviewport(struct render *R, int x, int y, int width, int height) {
	glViewport(x, y, width, height);
}

void 
render_setscissor(struct render *R, int x, int y, int width, int height ) {
	glScissor(x,y,width,height);
}

static void
apply_vb(struct render *R) {
	RID prog = R->program;
	struct shader * s = (struct shader *)array_ref(&R->shader, prog);
	if (s) {
		int i;
		RID last_vb = 0;
		int stride = 0;
		for (i=0;i<s->n;i++) {
			struct attrib_layout *al = &s->a[i];
			int vbidx = al->vbslot;
			RID vb = R->vbslot[vbidx];
			if (last_vb != vb) {
				struct buffer * buf = (struct buffer *)array_ref(&R->buffer, vb);
				if (buf == NULL) {
					continue;
				}
				glBindBuffer(GL_ARRAY_BUFFER, buf->glid);
				last_vb = vb;
				stride = buf->stride;
			}
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, al->size, al->type, al->normalized, stride, (const GLvoid *)(ptrdiff_t)(al->offset));
		}
	}

	CHECK_GL_ERROR
}

// texture

static int
calc_texture_size(enum TEXTURE_FORMAT format, int width, int height) {
	switch( format ) {
	case TEXTURE_RGBA8 :
		return width * height * 4;
	case TEXTURE_RGB565:
	case TEXTURE_RGBA4 :
		return width * height * 2;
	case TEXTURE_RGB:
		return width * height * 3;
	case TEXTURE_A8 :
	case TEXTURE_DEPTH :
		return width * height;
	case TEXTURE_PVR2 :
		return width * height / 4;
	case TEXTURE_PVR4 :
	case TEXTURE_ETC1 :
		return width * height / 2;
	default:
		return 0;
	}
}

RID 
render_texture_create(struct render *R, int width, int height, enum TEXTURE_FORMAT format, enum TEXTURE_TYPE type, int mipmap) {
	struct texture * tex = (struct texture *)array_alloc(&R->texture);
	if (tex == NULL)
		return 0;
	glGenTextures(1, &tex->glid);
	tex->width = width;
	tex->height = height;
	tex->format = format;
	tex->type = type;
	assert(type == TEXTURE_2D || type == TEXTURE_CUBE);
	tex->mipmap = mipmap;
	int size = calc_texture_size(format, width, height);
	if (mipmap) {
		size += size / 3;
	}
	if (type == TEXTURE_CUBE) {
		size *= 6;
	}
	tex->memsize = size;

	CHECK_GL_ERROR
	return array_id(&R->texture, tex);
}

static void
bind_texture(struct render *R, struct texture * tex, int slice, GLenum *type, int *target) {
	if (tex->type == TEXTURE_2D) {
		*type = GL_TEXTURE_2D;
		*target = GL_TEXTURE_2D;
	} else {
		assert(tex->type == TEXTURE_CUBE);
		*type = GL_TEXTURE_CUBE_MAP;
		*target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice;
	}
	glActiveTexture( GL_TEXTURE7 );
	R->changeflag |= CHANGE_TEXTURE;
	R->last.texture[7] = 0;	// use last texture slot
	glBindTexture( *type, tex->glid);
}

// return compressed
static int
texture_format(struct texture * tex, GLint *pf, GLenum *pt) {
	GLuint format = 0;
	GLenum itype = 0;
	int compressed = 0;
	switch(tex->format) {
	case TEXTURE_RGBA8 :
		format = GL_RGBA;
		itype = GL_UNSIGNED_BYTE;
		break;
	case TEXTURE_RGB :
		format = GL_RGB;
		itype = GL_UNSIGNED_BYTE;
		break;
	case TEXTURE_RGBA4 :
		format = GL_RGBA;
		itype = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	case TEXTURE_RGB565:
		format = GL_RGB;
		itype = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case TEXTURE_A8 :
	case TEXTURE_DEPTH :
		format = GL_ALPHA;
		itype = GL_UNSIGNED_BYTE;
		break;
#ifdef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 
	case TEXTURE_PVR2 :
		format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		compressed = 1;
		break;
#endif
#ifdef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 
	case TEXTURE_PVR4 :
		format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		compressed = 1;
		break;
#endif
#ifdef GL_ETC1_RGB8_OES
	case TEXTURE_ETC1 :
		format = GL_ETC1_RGB8_OES;
		compressed = 1;
		break;
#endif
	default:
		assert(0);
		return -1;
	}
	*pf = format;
	*pt = itype;
	return compressed;
}

void
render_texture_update(struct render *R, RID id, int width, int height, const void *pixels, int slice, int miplevel) {
	struct texture * tex = (struct texture *)array_ref(&R->texture, id);
	if (tex == NULL)
		return;

	GLenum type;
	int target;
	bind_texture(R, tex, slice, &type, &target);

	if (tex->mipmap) {
		glTexParameteri( type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	} else {
		glTexParameteri( type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	}
	glTexParameteri( type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	GLint format = 0;
	GLenum itype = 0;
	int compressed = texture_format(tex, &format, &itype);
	if (compressed) {
		glCompressedTexImage2D(target, miplevel, format,
			(GLsizei)tex->width, (GLsizei)tex->height, 0, 
			calc_texture_size(tex->format, width, height), pixels);
	} else {
		glTexImage2D(target, miplevel, format, (GLsizei)width, (GLsizei)height, 0, format, itype, pixels);
	}

	CHECK_GL_ERROR
}

void 
render_texture_subupdate(struct render *R, RID id, const void *pixels, int x, int y, int w, int h) {
	struct texture * tex = (struct texture *)array_ref(&R->texture, id);
	if (tex == NULL)
		return;

	GLenum type;
	int target;
	bind_texture(R, tex, 0, &type, &target);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	GLint format = 0;
	GLenum itype = 0;
	int compressed = texture_format(tex, &format, &itype);
	if (compressed) {
		glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 
			x, y, w, h,	format, 
			calc_texture_size(tex->format, w, h), pixels);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, format, itype, pixels);
	}

	CHECK_GL_ERROR
}

// blend mode
void 
render_setblend(struct render *R, enum BLEND_FORMAT src, enum BLEND_FORMAT dst) {
	R->current.blend_src = src;
	R->current.blend_dst = dst;
	R->changeflag |= CHANGE_BLEND;
}

// depth
void 
render_enabledepthmask(struct render *R, int enable) {
	R->current.depthmask = enable;
	R->changeflag |= CHANGE_DEPTH;
}

// depth
void 
render_enablescissor(struct render *R, int enable) {
	R->current.scissor = enable;
	R->changeflag |= CHANGE_SCISSOR;
}

void 
render_setdepth(struct render *R, enum DEPTH_FORMAT d) {
	R->current.depth = d;
	R->changeflag |= CHANGE_DEPTH;
}

// cull
void 
render_setcull(struct render *R, enum CULL_MODE c) {
	R->current.cull = c;
	R->changeflag |= CHANGE_CULL;
}

// render target
static RID
create_rt(struct render *R, RID texid) {
	struct target *tar = (struct target *)array_alloc(&R->target);
	if (tar == NULL)
		return 0;
	tar->tex = texid;
	struct texture * tex = (struct texture *)array_ref(&R->texture, texid);
	if (tex == NULL)
		return 0;
	glGenFramebuffers(1, &tar->glid);
	glBindFramebuffer(GL_FRAMEBUFFER, tar->glid);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->glid, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		close_target(tar, R);
		return 0;
	}
	CHECK_GL_ERROR

	return array_id(&R->target, tar);
}

RID 
render_target_create(struct render *R, int width, int height, enum TEXTURE_FORMAT format) {
	RID tex = render_texture_create(R, width, height, format, TEXTURE_2D, 0);
	if (tex == 0)
		return 0;
	render_texture_update(R, tex, width, height, NULL, 0, 0);
	RID rt = create_rt(R, tex);
	glBindFramebuffer(GL_FRAMEBUFFER, R->default_framebuffer);
	R->last.target = 0;
	R->changeflag |= CHANGE_TARGET;

	if (rt == 0) {
		render_release(R, TEXTURE, tex);
	}
	CHECK_GL_ERROR
	return rt;
}

RID 
render_target_texture(struct render *R, RID rt) {
	struct target *tar = (struct target *)array_ref(&R->target, rt);
	if (tar) {
		return tar->tex;
	} else {
		return 0;
	}
}

// render state

static void
render_state_commit(struct render *R) {
	if (R->changeflag & CHANGE_INDEXBUFFER) {
		RID ib = R->current.indexbuffer;
		if (ib != R->last.indexbuffer) {
			R->last.indexbuffer = ib;
			struct buffer * b = (struct buffer *)array_ref(&R->buffer, ib);
			if (b) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b->glid);
				CHECK_GL_ERROR
			}
		}
	}

	if (R->changeflag & CHANGE_VERTEXBUFFER) {
		apply_vb(R);
	}

	if (R->changeflag & CHANGE_TEXTURE) {
		static GLenum mode[] = {
			GL_TEXTURE_2D,
			GL_TEXTURE_CUBE_MAP,
		};
		int i;
		for (i=0;i<MAX_TEXTURE;i++) {
			RID id = R->current.texture[i];
			RID lastid = R->last.texture[i];
			if (id != lastid) {
				R->last.texture[i] = id;
				struct texture * tex = (struct texture *)array_ref(&R->texture, id);
				if (tex) {
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(mode[tex->type], tex->glid);
				}
			}
		}
		CHECK_GL_ERROR
	}

	if (R->changeflag & CHANGE_TARGET) {
		RID crt = R->current.target;
		if (R->last.target != crt) {
			GLuint rt = R->default_framebuffer;
			if (crt != 0) {
				struct target * tar = (struct target *)array_ref(&R->target, crt);
				if (tar) {
					rt = tar->glid;
				} else {
					crt = 0;
				}
			}
			glBindFramebuffer(GL_FRAMEBUFFER, rt);
			R->last.target = crt;
			CHECK_GL_ERROR
		}
	}

	if (R->changeflag & CHANGE_BLEND) {
		if (R->last.blend_src != R->current.blend_src || R->last.blend_dst != R->current.blend_dst) {
			if (R->current.blend_src == BLEND_DISABLE) {
				glDisable(GL_BLEND);
			} else if (R->last.blend_src == BLEND_DISABLE) {
				glEnable(GL_BLEND);
			}
			static GLenum blend[] = {
				0,
				GL_ZERO,
				GL_ONE,
				GL_SRC_COLOR,
				GL_ONE_MINUS_SRC_COLOR,
				GL_SRC_ALPHA,
				GL_ONE_MINUS_SRC_ALPHA,
				GL_DST_ALPHA,
				GL_ONE_MINUS_DST_ALPHA,
				GL_DST_COLOR,
				GL_ONE_MINUS_DST_COLOR,
				GL_SRC_ALPHA_SATURATE,
			};

			enum BLEND_FORMAT src = R->current.blend_src;
			enum BLEND_FORMAT dst = R->current.blend_dst;
			glBlendFunc(blend[src], blend[dst]);

			R->last.blend_src = src;
			R->last.blend_dst = dst;
		}
	}

	if (R->changeflag & CHANGE_DEPTH) {
		if (R->last.depth != R->current.depth) {
			if (R->last.depth == DEPTH_DISABLE) {
				glEnable( GL_DEPTH_TEST);
			} 
			if (R->current.depth == DEPTH_DISABLE) {
				glDisable( GL_DEPTH_TEST);
			} else {
				static GLenum depth[] = {
					0,
					GL_LEQUAL, 
					GL_LESS,
					GL_EQUAL, 
					GL_GREATER,
					GL_GEQUAL,
					GL_ALWAYS,
				};
				glDepthFunc( depth[R->current.depth] );
			}
			R->last.depth = R->current.depth;
		}
		if (R->last.depthmask != R->current.depthmask) {
			glDepthMask(R->current.depthmask ? GL_TRUE : GL_FALSE);
			R->last.depthmask = R->current.depthmask;
		}
	}

	if (R->changeflag & CHANGE_CULL) {
		if (R->last.cull != R->current.cull) {
			if (R->last.cull == CULL_DISABLE) {
				glEnable(GL_CULL_FACE);
			}
			if (R->current.cull == CULL_DISABLE) {
				glDisable(GL_CULL_FACE);
			} else {
				glCullFace(R->current.cull == CULL_FRONT ? GL_FRONT : GL_BACK);
			}
			R->last.cull = R->current.cull;
		}
	}

	if (R->changeflag & CHANGE_SCISSOR) {
		if (R->last.scissor != R->current.scissor) {
			if (R->current.scissor) {
				glEnable(GL_SCISSOR_TEST);
			} else {
				glDisable(GL_SCISSOR_TEST);
			}
			R->last.scissor = R->current.scissor;
		}
	}

	R->changeflag = 0;

	CHECK_GL_ERROR
}

void 
render_state_reset(struct render *R) {
	R->changeflag = ~0;
	memset(&R->last, 0 , sizeof(R->last));
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glDepthMask(GL_FALSE);
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glBindFramebuffer(GL_FRAMEBUFFER, R->default_framebuffer);

	CHECK_GL_ERROR
}

// draw
void 
render_draw(struct render *R, enum DRAW_MODE mode, int fromidx, int ni) {
	static int draw_mode[] = {
		GL_TRIANGLES,
		GL_LINES,
	};
	assert(mode < sizeof(draw_mode)/sizeof(int));
	render_state_commit(R);
	RID ib = R->current.indexbuffer;
	struct buffer * buf = (struct buffer *)array_ref(&R->buffer, ib);
	if (buf) {
		assert(fromidx + ni <= buf->n);
		int offset = fromidx;
		GLenum type = GL_UNSIGNED_SHORT;
		if (buf->stride == 1) {
			type = GL_UNSIGNED_BYTE;
		} else {
			offset *= sizeof(short);
		}
		glDrawElements(draw_mode[mode], ni, type, (char *)0 + offset);
		CHECK_GL_ERROR
	}
}

void
render_clear(struct render *R, enum CLEAR_MASK mask, unsigned long c) {
	GLbitfield m = 0;
	if (mask & MASKC) {
		m |= GL_COLOR_BUFFER_BIT;
		float a = ((c >> 24) & 0xff ) / 255.0;
		float r = ((c >> 16) & 0xff ) / 255.0;
		float g = ((c >> 8) & 0xff ) / 255.0;
		float b = ((c >> 0) & 0xff ) / 255.0;
		glClearColor(r,g,b,a);
	}
	if (mask & MASKD) {
		m |= GL_DEPTH_BUFFER_BIT;
	}
	if (mask & MASKS) {
		m |= GL_STENCIL_BUFFER_BIT;
	}
	render_state_commit(R);
	glClear(m);

	CHECK_GL_ERROR
}

// uniform
int 
render_shader_locuniform(struct render *R, const char * name) {
	struct shader * s = (struct shader *)array_ref(&R->shader, R->program);
	if (s) {
		int loc = glGetUniformLocation( s->glid, name);
		CHECK_GL_ERROR
		return loc;
	} else {
		return -1;
	}
}

void 
render_shader_setuniform(struct render *R, int loc, enum UNIFORM_FORMAT format, const float *v) {
	switch(format) {
	case UNIFORM_FLOAT1:
		glUniform1f(loc, v[0]);
		break;
	case UNIFORM_FLOAT2:
		glUniform2f(loc, v[0], v[1]);
		break;
	case UNIFORM_FLOAT3:
		glUniform3f(loc, v[0], v[1], v[2]);
		break;
	case UNIFORM_FLOAT4:
		glUniform4f(loc, v[0], v[1], v[2], v[3]);
		break;
	case UNIFORM_FLOAT33:
		glUniformMatrix3fv(loc, 1, GL_FALSE, v);
		break;
	case UNIFORM_FLOAT44:
		glUniformMatrix4fv(loc, 1, GL_FALSE, v);
		break;
	default:
		assert(0);
		return;
	}
	CHECK_GL_ERROR
}

int 
render_version(struct render *R) {
	return OPENGLES;
}
