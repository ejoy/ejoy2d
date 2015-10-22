#ifndef EJOY2D_LABEL_H
#define EJOY2D_LABEL_H

#include "spritepack.h"
#include "matrix.h"

#define LABEL_ALIGN_LEFT 0
#define LABEL_ALIGN_RIGHT 1
#define LABEL_ALIGN_CENTER 2

#define RL_COLOR 1
#define RL_LINEFEED 2

struct label_field {
	struct {
		uint32_t type:8;
		uint32_t start:12;
		uint32_t end:12;
	};
	union {
		uint32_t color;
		int val;
	};
};

struct rich_text {
	int count;
	int width;
	int height;
  const char *text;
	struct label_field *fields;
};

struct render;
void label_initrender(struct render *R);
void label_load();
void label_unload();
void label_flush();

void label_rawdraw(const char * str, float x, float y, struct pack_label * l);
int label_rawline(const char * str, struct pack_label *l);
void label_draw(const struct rich_text *rich, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg);
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
