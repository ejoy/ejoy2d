#ifndef ejoy_2d_screen_h
#define ejoy_2d_screen_h

void screen_init(float w, float h, float scale);
void screen_trans(float *x, float *y);
void screen_scissor(int x, int y, int w, int h);

#endif
