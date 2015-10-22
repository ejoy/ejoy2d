#include "label.h"
#include "dfont.h"
#include "shader.h"
#include "matrix.h"
#include "spritepack.h"
#include "screen.h"
#include "array.h"

#include "render.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#define TEX_HEIGHT 1024
#define TEX_WIDTH 1024
#define FONT_SIZE 31
#define TEX_FMT TEXTURE_A8

static RID Tex;
static struct dfont * Dfont = NULL;
static struct render *R = NULL;

void 
label_initrender(struct render *r) {
	R = r;
}

void
label_load() {
	if (Dfont) return;

	Dfont = dfont_create(TEX_WIDTH, TEX_HEIGHT);

	Tex = render_texture_create(R, TEX_WIDTH, TEX_HEIGHT, TEX_FMT, TEXTURE_2D, 0);
	render_texture_update(R, Tex, TEX_WIDTH, TEX_HEIGHT ,NULL, 0, 0);
}

void
label_unload() {
	render_release(R, TEXTURE, Tex);
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
gen_char(int unicode, const char * utf8, int size, int edge) {
	// todo : use large size when size is large
	struct font_context ctx;
	font_create(FONT_SIZE, &ctx);
	if (ctx.font == NULL) {
		return NULL;
	}

	font_size(utf8, unicode, &ctx);
	const struct dfont_rect * rect = dfont_insert(Dfont, unicode, FONT_SIZE, ctx.w+1, ctx.h+1, edge);
	if (rect == NULL) {
		dfont_flush(Dfont);
		rect = dfont_insert(Dfont, unicode, FONT_SIZE, ctx.w+1, ctx.h+1, edge);
		if (rect == NULL) {
			font_release(&ctx);
			return NULL;
		}
	}
	ctx.w = rect->w ;
	ctx.h = rect->h ;
	int buffer_sz = ctx.w * ctx.h;

	ARRAY(uint8_t, buffer, buffer_sz);
	
#ifdef FONT_EDGE_HASH
  if (edge) {
    ARRAY(uint8_t, tmp, buffer_sz);
    memset(tmp,0,buffer_sz);
    font_glyph(utf8, unicode, tmp, &ctx);
    gen_outline(ctx.w, ctx.h, tmp, buffer);
  } else {
    memset(buffer,0,buffer_sz);
    font_glyph(utf8, unicode, buffer, &ctx);
  }
#else
	ARRAY(uint8_t, tmp, buffer_sz);
	memset(tmp,0,buffer_sz);
	font_glyph(utf8, unicode, tmp, &ctx);
	gen_outline(ctx.w, ctx.h, tmp, buffer);
#endif
	
//	write_pgm(unicode, ctx.w, ctx.h, buffer);
	font_release(&ctx);

	render_texture_subupdate(R, Tex, buffer, rect->x, rect->y, rect->w, rect->h);

	return rect;
}


static inline void
set_point(struct vertex_pack *v, int *m, int xx, int yy,int tx, int ty) {
	v->vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
	v->vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
	screen_trans(&v->vx,&v->vy);

	v->tx = (uint16_t)(tx * (65535.0f/TEX_WIDTH));
	v->ty = (uint16_t)(ty * (65535.0f/TEX_HEIGHT));
}

static void
draw_rect(const struct dfont_rect *rect, int size, struct matrix *mat, uint32_t color, uint32_t additive) {
	struct vertex_pack vb[4];

	int w = (rect->w -1) * size / FONT_SIZE ;
	int h = (rect->h -1) * size / FONT_SIZE ;

	set_point(&vb[0], mat->m, 0,0, rect->x, rect->y);
	set_point(&vb[1], mat->m, w*SCREEN_SCALE,0, rect->x+rect->w-1, rect->y);
	set_point(&vb[2], mat->m, w*SCREEN_SCALE,h*SCREEN_SCALE, rect->x+rect->w-1, rect->y+rect->h-1);
	set_point(&vb[3], mat->m, 0,h*SCREEN_SCALE, rect->x, rect->y+rect->h-1);
	shader_draw(vb, color, additive);
}

static int
draw_size(int unicode, const char *utf8, int size, int edge) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode,FONT_SIZE,edge);
	if (rect == NULL) {
		rect = gen_char(unicode,utf8,size,edge);
	}
	if (rect) {
		return (rect->w -1) * size / FONT_SIZE;
	}
	return 0;
}

static int
draw_height(int unicode, const char *utf8, int size, int edge) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode,FONT_SIZE,edge);
	if (rect == NULL) {
		rect = gen_char(unicode,utf8,size,edge);
	}
	if (rect) {
		return rect->h * size / FONT_SIZE;
	}
	return 0;
}

static struct font_context
char_size(int unicode, const char *utf8, int size, int edge) {
	const struct dfont_rect * rect = dfont_lookup(Dfont,unicode,FONT_SIZE,edge);
	struct font_context ctx;
	font_create(FONT_SIZE, &ctx);
	if (ctx.font == NULL) {
		ctx.w = 0;
		ctx.h = 0;
		return ctx;
	}

	if (rect == NULL) {
		font_size(utf8, unicode, &ctx);
		//see gen_char
		ctx.w += 1;
		ctx.h += 1;
		ctx.w = (ctx.w -1) * size / FONT_SIZE;
		ctx.h = ctx.h * size / FONT_SIZE;
	} else {
		ctx.w = (rect->w -1) * size / FONT_SIZE;
		ctx.h = rect->h * size / FONT_SIZE;
	}
	// font_release should not reset ctx.w/ctx.h
	font_release(&ctx);
	return ctx;
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
draw_utf8(int unicode, float cx, int cy, int size, const struct srt *srt,
	uint32_t color, const struct sprite_trans *arg, int edge) {
	const struct dfont_rect * rect = dfont_lookup(Dfont, unicode, FONT_SIZE, edge);
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
	draw_rect(rect,size,m,color,arg->additive);

	return (rect->w-1) * size / FONT_SIZE ;
}

static uint32_t
get_rich_field_color(const struct rich_text *rich, int idx) {
  int i;
  for (i=0;i<rich->count;i++) {
    struct label_field *field = (struct label_field*)(rich->fields+i);
    if (idx >= field->start && idx <= field->end && field->type	== RL_COLOR) {
      return field->color;
    }
  }
  return 0;
}

static bool
get_rich_filed_lf(const struct rich_text *rich, int idx, float * offset) {
	int i;
	for (i=0; i<rich->count; i++) {
		struct label_field *field = (struct label_field*)(rich->fields+i);
		if (idx==field->start && idx==field->end && field->type==RL_LINEFEED) {
			*offset = (float)(field->val / 1000.0);
			return true;
		}
	}
	return false;
}

static int
unicode_len(const char chr) {
	uint8_t c = (uint8_t)chr;
	if ((c&0x80) == 0) {
		return 1;
	} else if ((c&0xe0) == 0xc0) {
		return 2;
	} else if ((c&0xf0) == 0xe0) {
		return 3;
	} else if ((c&0xf8) == 0xf0) {
		return 4;
	} else if ((c&0xfc) == 0xf8) {
		return 5;
	} else {
		return 6;
	}
}

static void
draw_line(const struct rich_text *rich, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg,
          uint32_t color, int cy, int w, int start, int end, int *pre_char_cnt, float space_scale) {
    const char *str = rich->text;
		float cx;
    int j;
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
            cx = 0.0;
            break;
        case LABEL_ALIGN_RIGHT:
            cx = l->width - w;
            break;
        case LABEL_ALIGN_CENTER:
            cx = (l->width - w)/2;
            break;
    }
  
    int char_cnt = 0;
    for (j=start; j<end;) {
        int unicode;
        char_cnt++;
			
				int len = unicode_len(str[j]);
				unicode = get_unicode(str+j, len);
				j+=len;
			
        if(unicode != '\n') {
            uint32_t field_color = get_rich_field_color(rich, *pre_char_cnt+char_cnt);
						if (field_color == 0) {
							field_color = color;
						} else {
							field_color = color_mul(field_color,  color | 0xffffff);
						}
            cx+=(draw_utf8(unicode, cx, cy, size, srt, field_color, arg, l->edge) + l->space_w)*space_scale;
        }
    }
    *pre_char_cnt += char_cnt;
}

int
label_char_size(struct pack_label* l, const char* chr, int* width, int* height, int* unicode) {
	char utf8[7];
	int len = unicode_len(chr[0]);
	*unicode = copystr(utf8, chr, len);
	struct font_context ct = char_size(*unicode, utf8, l->size, l->edge);
	*width = ct.w + l->space_w;
	*height = ct.h + l->space_h;
	return len;
}

void
label_size(const char *str, struct pack_label * l, int* width, int* height) {
	char utf8[7];
	int i;
	int w=0, max_w=0, h=0, max_h=0;
	for (i=0; str[i];) {
		int unicode;
		int len = unicode_len(str[i]);
		unicode = copystr(utf8, str+i, len);
		i+=len;
		struct font_context ct = char_size(unicode, utf8, l->size, l->edge);
		w += ct.w + l->space_w;
		if (h==0) {
			h = ct.h + l->space_h;
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

uint32_t
label_get_color(struct pack_label * l, const struct sprite_trans *arg) {
    uint32_t color;
	if (arg->color == 0xffffffff) {
		color = l->color;
	}
	else if (l->color == 0xffffffff){
		color = arg->color;
	} else {
		color = color_mul(l->color, arg->color);
	}
    return color;
}

int
label_rawline(const char * str, struct pack_label *l) {
	char utf8[7];
	int i;
	int w=0;
	for (i=0; str[i];) {
		int len = unicode_len(str[i]);
		int unicode = copystr(utf8, str+i, len);
		struct font_context ct = char_size(unicode, utf8, l->size, l->edge);
		if (w+ct.w > l->width) {
			break;
		}
		i+=len;
		w += ct.w + l->space_w;
	}
	return i;
}

static int
raw_width(const char * str,  struct pack_label * l) {
	char utf8[7];
	int i;
	int w=0;
	for (i=0; str[i];) {
		int len = unicode_len(str[i]);
		int unicode = copystr(utf8, str+i, len);
		i+=len;
		struct font_context ct = char_size(unicode, utf8, l->size, l->edge);
		w += ct.w + l->space_w;
	}
	return w;
}

void
label_rawdraw(const char * str, float fx, float y, struct pack_label * l) {
	shader_texture(Tex, 0);
	uint32_t color = l->color;
	int edge = l->edge;
	int size = l->size;
	int width = raw_width(str, l);
	int x = (int)fx;

	switch (l->align) {
	case LABEL_ALIGN_LEFT:
		break;
	case LABEL_ALIGN_RIGHT:
		x += l->width - width;
		break;
	case LABEL_ALIGN_CENTER:
		x += (l->width - width)/2;
		break;
	}

	char utf8[7];
	int i;
	struct matrix mat = {{ 1024,0,0,1024,0,y * SCREEN_SCALE}};
	for (i=0; str[i];) {
		int len = unicode_len(str[i]);
		int unicode = copystr(utf8, str+i, len);
		i+=len;
		const struct dfont_rect * rect = dfont_lookup(Dfont, unicode, FONT_SIZE, edge);
		if (rect == NULL) {
			rect = gen_char(unicode,utf8,FONT_SIZE,edge);
			if (rect == NULL)
				continue;
		}
		mat.m[4]=x*SCREEN_SCALE;
		draw_rect(rect,size,&mat,color,0);

		x += (rect->w-1) * size / FONT_SIZE + l->space_w;
	}
}

void
label_draw(const struct rich_text *rich, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg) {
	shader_texture(Tex, 0);
	uint32_t color = label_get_color(l, arg);
	const char *str = rich->text;

	char utf8[7];
	int i;
	int ch = 0, w = 0, cy = 0, pre = 0, char_cnt = 0, idx = 0;
	for (i=0; str && str[i];) {
		int unicode;
		int len = unicode_len(str[i]);
		unicode = copystr(utf8, str+i, len);
		i+=len;
		w += draw_size(unicode, utf8, l->size, l->edge) + l->space_w;
		if (ch == 0) {
				ch = draw_height(unicode, utf8, l->size, l->edge) + l->space_h;
		}
		
		float space_scale=1.0;
		uint32_t lf = get_rich_filed_lf(rich, idx, &space_scale);
		if((l->auto_scale == 0 && lf) || unicode == '\n') {
				draw_line(rich, l, srt, arg, color, cy, w, pre, i, &char_cnt, space_scale);
				cy += ch;
				pre = i;
				w = 0; ch = 0;
		}
		idx++;
	}
    
	draw_line(rich, l, srt, arg, color, cy, w, pre, i, &char_cnt, 1.0);
}

