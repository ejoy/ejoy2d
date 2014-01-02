#ifndef ejoy_font_h
#define ejoy_font_h

struct font_context {
  int w;
  int h;
  int ascent;
  void * font;
  int size;
  void * dc;
};

void font_size(const char *str, int unicode, struct font_context * ctx);
void font_glyph(const char * str, int unicode, void * buffer, struct font_context * ctx);
void font_create(int font_size, struct font_context *ctx);
void font_release(struct font_context *ctx);

void* font_write_glyph(const char* str, int size, uint32_t color, int is_edge, int* width, int* height);

#endif
