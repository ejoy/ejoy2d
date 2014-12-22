#ifndef ejoy_2d_screen_h
#define ejoy_2d_screen_h
#include <stdbool.h>

struct render;
struct vertex_pack;

void screen_initrender(struct render *R);
void screen_init(float w, float h, float scale);
void screen_trans(float *x, float *y);
void screen_scissor(int x, int y, int w, int h);
bool screen_is_visible(float x,float y);
bool screen_is_poly_invisible(const struct vertex_pack* vp, int len);
void screen_get_info(int* w, int* h, float* scale);
#endif
