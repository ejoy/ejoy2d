#ifndef dynamic_font_h
#define dynamic_font_h
#include <stdlib.h>

struct dfont;

struct dfont_rect {
	int x;
	int y;
	int w;
	int h;
};

struct dfont * dfont_create(int width, int height);
void dfont_release(struct dfont *);
const struct dfont_rect * dfont_lookup(struct dfont *, int c, int font, int edge);
const struct dfont_rect * dfont_insert(struct dfont *, int c, int font, int width, int height, int edge);
void dfont_remove(struct dfont *, int c, int font, int edge);
void dfont_flush(struct dfont *);
void dfont_dump(struct dfont *); // for debug

size_t dfont_data_size(int width, int height);
void dfont_init(void* d, int width, int height);

#endif
