#include "label.h"
#include <X11/Xlib.h>
#include <stdio.h>

Display *dis;

void
font_create(int font_size, struct font_context *ctx) {
    printf(">>>>>>>> font create\n");
    XFontStruct* font = XLoadQueryFont(dis, "*");
    ctx->font = (void*)font;
    ctx->ascent = font->ascent;
}

void
font_release(struct font_context *ctx) {
    XFreeFont(dis, ctx->font);
}

void 
font_size(const char *str, int unicode, struct font_context *ctx) {
    
}

void 
font_glyph(const char * str, int unicode, void * buffer, struct font_context *ctx) {
}
