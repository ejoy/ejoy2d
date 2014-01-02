#include "label.h"
#include "dfont.h"
#include "shader.h"
#include "opengl.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#define TEX_HEIGHT 1024
#define TEX_WIDTH 1024

static GLuint Tex;
static struct dfont * Dfont = NULL;

void
label_load() {
	assert(Dfont == NULL);
	Dfont = dfont_create(TEX_HEIGHT, TEX_WIDTH);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	glGenTextures(1, &(Tex));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Tex);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, (GLsizei)TEX_WIDTH, (GLsizei)TEX_HEIGHT, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
}

void
label_unload() {
	glDeleteTextures(1,&Tex);
	dfont_release(Dfont);
	Dfont = NULL;
}

void
label_flush() {
	if (Dfont) {
		dfont_flush(Dfont);
	}
}

static inline int
copystr(char *utf8, const char *str, int n) {
	int i;
	utf8[0] = str[0];
	int unicode = utf8[0] & ((1 << (8-n)) - 1);
	for (i=1;i<n;i++) {
		utf8[i] = str[i];
		unicode = unicode << 6 | (utf8[i] & 0x3f);
	}
	utf8[i] = 0;
	return unicode;
}

static inline int
get_unicode(const char *str, int n) {
	int i;
	int unicode = str[0] & ((1 << (8-n)) - 1);
	for (i=1;i<n;i++) {
		unicode = unicode << 6 | ((uint8_t)str[i] & 0x3f);
	}
	return unicode;
}

static const struct dfont_rect *
gen_char(int unicode, const char * utf8, int size) {
	struct font_context ctx;
	font_create(size, &ctx);
  if (ctx.font == NULL) {
    return NULL;
  }
	font_size(utf8, unicode, &ctx);
	const struct dfont_rect * rect = dfont_insert(Dfont, unicode, size, ctx.w,ctx.h);
	if (rect == NULL) {
		font_release(&ctx);
		return NULL;
	}
	int buffer_sz = rect->w * rect->h;
	uint8_t buffer[buffer_sz];
	memset(buffer,0,buffer_sz);
	font_glyph(utf8, unicode, buffer, &ctx);
	font_release(&ctx);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->w, rect->h, GL_ALPHA, GL_UNSIGNED_BYTE, buffer);

	return rect;
}


static inline void
set_point(float *v, int *m, int xx, int yy,int tx, int ty) {
	v[0] = (xx * m[0] + yy * m[2]) / 1024 + m[4];
	v[1] = (xx * m[1] + yy * m[3]) / 1024 + m[5];
	screen_trans(&v[0],&v[1]);

	v[2] = (float)tx * (1.0f/TEX_WIDTH);
	v[3] = (float)ty * (1.0f/TEX_HEIGHT);
}

static void
draw_rect(const struct dfont_rect *rect, struct matrix *mat, uint32_t color) {
	float vb[16];

	int w = rect->w-1;
	int h = rect->h-1;

	set_point(vb+0, mat->m, 0,0, rect->x, rect->y);
	set_point(vb+4, mat->m, w*SCREEN_SCALE,0, rect->x+w, rect->y);
	set_point(vb+8, mat->m, w*SCREEN_SCALE,h*SCREEN_SCALE, rect->x+w, rect->y+h);
	set_point(vb+12, mat->m, 0,h*SCREEN_SCALE, rect->x, rect->y+h);
	shader_draw(vb, color);
}

static int
draw_size(int unicode, const char *utf8, int size) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode, size);
	if (rect == NULL) {
		rect = gen_char(unicode,utf8,size);
	}
	if (rect) {
		return rect->w - 1;
	}
	return 0;
}

// also defined in sprite.c
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

static int
draw_utf8(int unicode, int cx, int cy, int size, const struct srt *srt, uint32_t color, const struct sprite_trans *arg) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode, size);
	if (rect == NULL) {
		return 0;
	}
	struct matrix tmp;
	struct matrix mat1 = {{ 1024,0,0,1024, cx* SCREEN_SCALE, cy*SCREEN_SCALE }};
	struct matrix *m;

	if (arg->mat) {
		m=&tmp;
		matrix_mul(m, &mat1, arg->mat);
	} else {
		m=&mat1;
	}
	matrix_srt(m, srt);
	draw_rect(rect,m,color);

	return rect->w - 1;
}

void
label_draw(const char *str, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg) {
	shader_texture(Tex);
	uint32_t color;
	if (arg->color == 0xffffffff) {
		color = l->color;
	}
	else if (l->color == 0xffffffff){
		color = arg->color;
	} else {
		color = color_mul(l->color, arg->color);
	}

	char utf8[7];
	int i;
	int w = 0;
	for (i=0;str[i];) {
		int unicode;
		uint8_t c = (uint8_t)str[i];
		if ((c&0x80) == 0) {
			unicode = copystr(utf8,str+i,1);
			i+=1;
		} else if ((c&0xe0) == 0xc0) {
			unicode = copystr(utf8,str+i,2);
			i+=2;
		} else if ((c&0xf0) == 0xe0) {
			unicode = copystr(utf8,str+i,3);
			i+=3;
		} else if ((c&0xf8) == 0xf0) {
			unicode = copystr(utf8,str+i,4);
			i+=4;
		} else if ((c&0xfc) == 0xf8) {
			unicode = copystr(utf8,str+i,5);
			i+=5;
		} else {
			unicode = copystr(utf8,str+i,6);
			i+=6;
		}
		w+=draw_size(unicode, utf8, l->size);
	}

	int cx=0,cy=0;
	switch (l->align) {
	case LABEL_ALIGN_LEFT:
		cx = 0;
		break;
	case LABEL_ALIGN_RIGHT:
		cx = l->width - w;
		break;
	case LABEL_ALIGN_CENTER:
		cx = (l->width - w)/2;
		break;
	}

	for (i=0;str[i];) {
		int unicode;
		uint8_t c = (uint8_t)str[i];
		if ((c&0x80) == 0) {
			unicode = get_unicode(str+i,1);
			i+=1;
		} else if ((c&0xe0) == 0xc0) {
			unicode = get_unicode(str+i,2);
			i+=2;
		} else if ((c&0xf0) == 0xe0) {
			unicode = get_unicode(str+i,3);
			i+=3;
		} else if ((c&0xf8) == 0xf0) {
			unicode = get_unicode(str+i,4);
			i+=4;
		} else if ((c&0xfc) == 0xf8) {
			unicode = get_unicode(str+i,5);
			i+=5;
		} else {
			unicode = get_unicode(str+i,6);
			i+=6;
		}
		// todo: multiline
		cx+=draw_utf8(unicode, cx,cy, l->size, srt, color, arg);
	}
}
