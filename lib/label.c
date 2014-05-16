#include "label.h"
#include "dfont.h"
#include "shader.h"
#include "opengl.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"
#include "array.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#define TEX_HEIGHT 1024
#define TEX_WIDTH 1024
#define FONT_SIZE 31
#define TEX_FMT GL_ALPHA

static GLuint Tex;
static struct dfont * Dfont = NULL;
static int Outline = 1;

void
label_load() {
	if (Dfont) return;

	Dfont = dfont_create(TEX_WIDTH, TEX_HEIGHT);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	glGenTextures(1, &(Tex));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Tex);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glTexImage2D(GL_TEXTURE_2D, 0, TEX_FMT, (GLsizei)TEX_WIDTH, (GLsizei)TEX_HEIGHT, 0, TEX_FMT, GL_UNSIGNED_BYTE, NULL);
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

void
label_gen_outline(int outline) {
  Outline = outline;
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

static inline int
max(int a, int b, int c, int d) {
	a = a>b ? a : b;
	a = a>c ? a : c;
	a = a>d ? a : d;
	return a;
}

static void
gen_outline(int w, int h, uint8_t *buffer, uint8_t *dest) {
	int i,j;
	for (i=0;i<h;i++) {
		uint8_t * output = dest + i*w;
		uint8_t * line = buffer + i*w;
		uint8_t * prev;
		uint8_t * next;
		if (i==0) {
			prev = line;
		} else {
			prev = line-w;
		}
		if (i==h-1) {
			next = line;
		} else {
			next = line+w;
		}
		for (j=0;j<w;j++) {
			int left, right;
			if (j==0) {
				left = 0;
			} else {
				left = 1;
			}
			if (j==w-1) {
				right = 0;
			} else {
				right = 1;
			}
			int n1 = max(line[j-left],line[j+right],prev[j],next[j]);
			int n2 = max(prev[j-left],prev[j+right],next[j-left],next[j+right]);
			int edge = (n1*3 + n2) / 4;
			if (line[j] == 0) {
				output[j] = edge / 2;
			} else {
				output[j] = line[j]/2 + 128;
			}
		}
		if (output[0] > 128) {
			output[0]/=2;
		}
		if (output[w-1] > 128) {
			output[w-1]/=2;
		}
	}
}

/*
static void
write_pgm(int unicode, int w, int h, const uint8_t * buffer) {
	char tmp[128];
	sprintf(tmp,"%d.pgm",unicode);
	FILE *f = fopen(tmp, "wb");
	fprintf(f, "P2\n%d %d\n255\n",w,h);
	int i,j;
	for (i=0;i<h;i++) {
		for (j=0;j<w;j++) {
			fprintf(f,"%3d,",buffer[0]);
			++buffer;
		}
		fprintf(f,"\n");
	}
	fclose(f);
}
*/

static const struct dfont_rect *
gen_char(int unicode, const char * utf8, int size, int outline) {
	// todo : use large size when size is large
	struct font_context ctx;
	font_create(FONT_SIZE, &ctx);
	if (ctx.font == NULL) {
		return NULL;
	}

	font_size(utf8, unicode, &ctx);
	const struct dfont_rect * rect = dfont_insert(Dfont, unicode, FONT_SIZE, ctx.w+1, ctx.h+1);
	if (rect == NULL) {
		font_release(&ctx);
		return NULL;
	}
	ctx.w = rect->w ;
	ctx.h = rect->h ;
	int buffer_sz = ctx.w * ctx.h;

	ARRAY(uint8_t, buffer, buffer_sz);
  
  if (outline) {
    ARRAY(uint8_t, tmp, buffer_sz);
    memset(tmp,0,buffer_sz);
    font_glyph(utf8, unicode, tmp, &ctx);
    gen_outline(ctx.w, ctx.h, tmp, buffer);
  } else {
    memset(buffer,0,buffer_sz);
    font_glyph(utf8, unicode, buffer, &ctx);
  }
  
//	write_pgm(unicode, ctx.w, ctx.h, buffer);
	font_release(&ctx);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, rect->x, rect->y, rect->w, rect->h, TEX_FMT, GL_UNSIGNED_BYTE, buffer);

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
draw_rect(const struct dfont_rect *rect, int size, struct matrix *mat, uint32_t color) {
	float vb[16];

	int w = (rect->w -1) * size / FONT_SIZE ;
	int h = (rect->h -1) * size / FONT_SIZE ;

	set_point(vb+0, mat->m, 0,0, rect->x, rect->y);
	set_point(vb+4, mat->m, w*SCREEN_SCALE,0, rect->x+rect->w-1, rect->y);
	set_point(vb+8, mat->m, w*SCREEN_SCALE,h*SCREEN_SCALE, rect->x+rect->w-1, rect->y+rect->h-1);
	set_point(vb+12, mat->m, 0,h*SCREEN_SCALE, rect->x, rect->y+rect->h-1);
	shader_draw(vb, color);
}

static int
draw_size(int unicode, const char *utf8, int size, int gen_new) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode,FONT_SIZE);
	if (rect == NULL && gen_new) {
		rect = gen_char(unicode,utf8,size,Outline);
	}
	if (rect) {
		return (rect->w -1) * size / FONT_SIZE;
	}
	return 0;
}

static int
draw_height(int unicode, const char *utf8, int size, int gen_new) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode,FONT_SIZE);
	if (rect == NULL && gen_new) {
		rect = gen_char(unicode,utf8,size,Outline);
	}
	if (rect) {
		return rect->h * size / FONT_SIZE;
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
	int g2 = (c2 >> 16) & 0xff;
	int b2 = (c2 >> 8) & 0xff;
	int a2 = c2 & 0xff;

	return (r1 * r2 /255) << 24 |
		(g1 * g2 /255) << 16 |
		(b1 * b2 /255) << 8 |
		(a1 * a2 /255) ;
}

static int
draw_utf8(int unicode, int cx, int cy, int size, const struct srt *srt,
	uint32_t color, const struct sprite_trans *arg) {
	const struct dfont_rect * rect = dfont_lookup(Dfont, unicode, FONT_SIZE);
	if (rect == NULL) {
		return 0;
	}
	struct matrix tmp;
	struct matrix mat1 = {{ 1024,0,0,1024, cx * SCREEN_SCALE, cy*SCREEN_SCALE }};
	struct matrix *m;

	if (arg->mat) {
		m=&tmp;
		matrix_mul(m, &mat1, arg->mat);
	} else {
		m=&mat1;
	}
	matrix_srt(m, srt);
	draw_rect(rect,size,m,color);

	return (rect->w-1) * size / FONT_SIZE ;
}

static void
draw_line(const char *str, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg,
          uint32_t color, int cy, int w, int start, int end) {
    int cx, j;
    int size = l->size;
    if (l->auto_scale != 0 && w > l->width)
    {
        float scale = l->width * 1.0f / w;
        size = scale * size;
        cy = cy + (l->size - size) / 2;
        w = l->width;
    }

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
    
    for (j=start; j<end;) {
        int unicode;
        uint8_t c = (uint8_t)str[j];
        if ((c&0x80) == 0) {
            unicode = get_unicode(str+j,1);
            j+=1;
        } else if ((c&0xe0) == 0xc0) {
            unicode = get_unicode(str+j,2);
            j+=2;
        } else if ((c&0xf0) == 0xe0) {
            unicode = get_unicode(str+j,3);
            j+=3;
        } else if ((c&0xf8) == 0xf0) {
            unicode = get_unicode(str+j,4);
            j+=4;
        } else if ((c&0xfc) == 0xf8) {
            unicode = get_unicode(str+j,5);
            j+=5;
        } else {
            unicode = get_unicode(str+j,6);
            j+=6;
        }
        
        if(unicode != '\n')
            cx+=draw_utf8(unicode, cx, cy, size, srt, color, arg) + l->space_w;
    }
}

void
label_size(const char *str, struct pack_label * l, int* width, int* height) {
	char utf8[7];
	int i;
	int w=0, max_w=0, h=0, max_h=0;
	for (i=0; str[i];) {
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
		w += draw_size(unicode, utf8, l->size, 0) + l->space_w;
		if (h == 0) {
            h = draw_height(unicode, utf8, l->size, 0) + l->space_h;
		}
        
		if((l->auto_scale == 0 && w > l->width) || unicode == '\n') {
			max_h += h;
			h = 0;
			if (w > max_w) max_w = w;
			w = 0;
		}
	}

	max_h += h;
	if (w > max_w) max_w = w;
    if (l->auto_scale > 0 && max_w > l->width)
        max_w = l->width;
   
	*width = max_w;
	*height = max_h;
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
    int ch = 0, w = 0, cy = 0, pre = 0;
	for (i=0; str[i];) {
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
		w += draw_size(unicode, utf8, l->size, 1) + l->space_w;
        if (ch == 0) {
            ch = draw_height(unicode, utf8, l->size, 1) + l->space_h;
        }
        
        if((l->auto_scale == 0 && w > l->width) || unicode == '\n') {
            draw_line(str, l, srt, arg, color, cy, w, pre, i);
            cy += ch;
            pre = i;
            w = 0; ch = 0;
        }
	}
    
    draw_line(str, l, srt, arg, color, cy, w, pre, i);
}

