#ifndef EJOY2D_LABEL_H
#define EJOY2D_LABEL_H

#include "spritepack.h"
#include "matrix.h"

#define LABEL_ALIGN_LEFT 0
#define LABEL_ALIGN_RIGHT 1
#define LABEL_ALIGN_CENTER 2

void label_load();
void label_unload();
void label_flush();

void label_draw(const char * str, struct pack_label * l, struct srt *srt, const struct sprite_trans *arg);
void label_size(const char * str, struct pack_label * l, int* width, int* height);
void label_gen_outline(int outline);

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
