#include "label.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library  library;

static const char* TTFONT = "/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc";

static void _fault(int errcode, const char * msg) {
    printf("err(%d): %s\n\n\t have a look at fterrdef.h\n",errcode, msg);
    exit(1);
}

void
font_create(int font_size, struct font_context *ctx) {
    FT_Face face;
    int err = FT_New_Face(library, TTFONT, 0, &face);
    if (err) {
        if (err == 1)
            _fault(err, "set your own vector font");
        else
            _fault(err, "new face failed");
    }

    err = FT_Set_Pixel_Sizes(face,0,font_size);

    if (err)
        _fault(err, "set char size failed");
    
    ctx->font = face;
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
		printf("cannot find glyph %d\n", unicode);
		exit(1);
	}
    FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
    
    int err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	if (err)
        _fault(err, "render failed");

	FT_Bitmap *bitmap = &(face->glyph->bitmap);

    ctx->w = bitmap->width;
    ctx->h = bitmap->rows;
}

void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx) {
    FT_Face face = ctx->font;
	FT_Bitmap *bitmap = &(face->glyph->bitmap);

    memcpy(buffer, bitmap->buffer, ctx->w * ctx->h);
}

void
font_init() {
    if (FT_Init_FreeType(&library)) {
        printf("font init failed");
    }
}
