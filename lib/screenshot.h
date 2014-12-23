#ifndef screenshot_h
#define screenshot_h

#include "sprite.h"

int screenshot(int x, int y, int w, int h, int tex_id, struct sprite* spr, unsigned char* pixels);
void release_screenshot(int tex_id);

#endif
