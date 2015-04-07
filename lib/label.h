#ifndef EJOY2D_LABEL_H
#define EJOY2D_LABEL_H

#include "spritepack.h"
#include "matrix.h"

/*
 label align:
 --------------------------------------
 |         |top(v)|bottom(v)|center(v)|
 --------------------------------------
 |left(h)  |  0   |    8    |    16   |
 -------------------------------------
 |right(h) |  1   |    9    |    17   |
 --------------------------------------
 |center(h)|  2   |    10   |    18   |
 --------------------------------------


 */

#define LABEL_ALIGN_H_LEFT_MASK 0
#define LABEL_ALIGN_H_RIGHT_MASK 1
#define LABEL_ALIGN_H_CENTER_MASK 2

#define LABEL_ALIGN_V_TOP_MASK 0
#define LABEL_ALIGN_V_BOTTOM_MASK 8
#define LABEL_ALIGN_V_CENTER_MASK 16

#define LABEL_ALIGN_DEFAULT ( LABEL_ALIGN_H_LEFT_MASK | LABEL_ALIGN_V_TOP_MASK )

#define RL_COLOR 1
#define RL_LINEFEED 2
#define RL_SPRITE 3
#define RL_SPACE 4

struct label_sprite {
	struct sprite *s;
	int w, h, mat, dy;
};

struct label_field {
	struct {
		uint32_t type:8;
		uint32_t start:12;
		uint32_t end:12;
	};
	union {
		uint32_t color;
		int val;
		struct label_sprite *ls;
	};
};

struct rich_text {
    short label_color_enable;
	int count;
	int width;
	int height;
    int sprite_count;
    const char *text;
	struct label_field *fields;
};

struct render;
void label_initrender(struct render *R);
void label_load();
void label_unload();
void label_flush();

void label_draw(const struct rich_text *rich, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg);
void label_draw_sprite(const struct rich_text *rich, struct srt *srt, const struct sprite_trans *arg);
void label_size(const char * str, struct pack_label * l, int* width, int* height);
int label_char_size(struct pack_label* l, const char* chr, int* width, int* height, int* unicode);
uint32_t label_get_color(struct pack_label * l, const struct sprite_trans *arg);

struct font_context {
	int w;
	int h;
	int ascent;
	void * font;
	void * dc;
//    int edge;
};

void font_size(const char *str, int unicode, struct font_context * ctx);
void font_glyph(const char * str, int unicode, void * buffer, struct font_context * ctx);
void font_create(int font_size, struct font_context *ctx);
void font_release(struct font_context *ctx);


#endif
