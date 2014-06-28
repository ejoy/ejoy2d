#ifndef ejoy_2d_screen_h
#define ejoy_2d_screen_h
#include <stdbool.h>

void screen_init(float w, float h, float scale);
void screen_trans(float *x, float *y);
void screen_scissor(int x, int y, int w, int h);
bool screen_is_visible(float x,float y);
bool screen_is_poly_invisible(const float* points,int len,int stride);
#endif
