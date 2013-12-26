#ifndef dynamic_font_h
#define dynamic_font_h

struct dfont;

struct dfont_rect {
	int x;
	int y;
	int w;
	int h;
};

struct dfont * dfont_create(int width, int height);
void dfont_release(struct dfont *);
const struct dfont_rect * dfont_lookup(struct dfont *, int c, int font);
const struct dfont_rect * dfont_insert(struct dfont *, int c, int font, int width, int height);
void dfont_flush(struct dfont *);
void dfont_dump(struct dfont *); // for debug

#endif
