#include "label.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

FT_Library  library;

#ifdef __APPLE__
static const char* TTFONT = "/System/Library/Fonts/STHeiti Light.ttc";
#else
static const char* TTFONT = "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc";
#endif

#define _fault(errcode, msg) __fault(errcode, msg, __FILE__, __LINE__)

static void __fault(int errcode, const char * msg, const char *file, int line) {
    if (errcode) 
        printf("err(%d): %s, error occured in file %s, line %d\n\n\t have a look at fterrdef.h\n",errcode, msg, file, line);
    else
        printf("err: %s, occured in file %s, line %d\n", msg, file, line);
    exit(1);
}
struct bitmap {
    void * buf;
    int w;
    int h;
    int pitch;
};

/* x,y is offset of target */
static void
cpy_bitmap(struct bitmap * target, struct bitmap * src, int x, int y) {
    unsigned char * sbuf = src->buf;
    unsigned char * tbuf = target->buf;

    int x0,y0,x1,y1;
    x0 = x > 0 ? x : 0;
    y0 = y > 0 ? y : 0;
    x1 = (x + src->w > target->w) ? target->w : (x+src->w);
    y1 = (y + src->h > target->h) ? target->h : (y+src->h);

    if (x1 <= x0 || y1 <= y0)
        return;
    if (x0 >= target->w || y0 >= target->h)
        return;

    int w = x1 - x0;
    int h = y1 - y0;

    tbuf += y0 * target->pitch + x0;
    sbuf += (y0 - y)*src->pitch + x0 - x;
    int i,j;
    for (i=0;i<h;i++) {
        for (j=0;j<w;j++) 
            tbuf[j] = sbuf[j];
        sbuf += src->pitch;
        tbuf += target->pitch;
    }
}

void
font_create(int font_size, struct font_context *ctx) {
    FT_Face face;
    int err = FT_New_Face(library, TTFONT, 0, &face);
    if (err) {
        if (err == 1)
            _fault(err, "set your own vector font resource path");
        else
            _fault(err, "new face failed");
    }

    err = FT_Set_Pixel_Sizes(face,0,font_size);

    if (err)
        _fault(err, "set char size failed");
    
    ctx->font = face;
    ctx->ascent = face->size->metrics.ascender >>6;
    ctx->h = face->size->metrics.height >> 6;
}

void
font_release(struct font_context *ctx) {
    FT_Done_Face(ctx->font);
}

void 
font_size(const char *str, int unicode, struct font_context *ctx) {
    FT_Face face = ctx->font;
    FT_UInt glyph_index = FT_Get_Char_Index(face, unicode);
	if (!glyph_index) {
		ctx->w = 0;
		return;
	}
    FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
    
    int err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	if (err)
        _fault(err, "render failed");

    FT_GlyphSlot slot = face->glyph;

    ctx->w = slot->advance.x >> 6;
}


void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx) {
    FT_Face face = ctx->font;
	FT_GlyphSlot slot = face->glyph;
    FT_Bitmap *bitmap = &(slot->bitmap);

    int offx = slot->bitmap_left;
    int offy = ctx->ascent - slot->bitmap_top;

    struct bitmap src;
    struct bitmap target;
    src.buf = bitmap->buffer;
    src.pitch = bitmap->pitch;
    src.w = bitmap->width;
    src.h = bitmap->rows;
    target.buf = buffer;
    target.w = target.pitch = ctx->w;
    target.h = ctx->h;

    cpy_bitmap(&target, &src, offx, offy);
}

void
font_init() {
    if (FT_Init_FreeType(&library)) {
        printf("font init failed");
    }
}
