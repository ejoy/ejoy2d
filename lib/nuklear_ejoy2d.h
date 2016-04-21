#ifndef NK_EJOY2D_H_
#define NK_EJOY2D_H_

//#include "nuklear.h"

struct render;
struct nk_font_atlas;
enum nk_anti_aliasing;

struct nk_context *nk_ejoy2d_init(struct render *R, int w, int h);
void nk_ejoy2d_font_stash_begin(struct nk_font_atlas **atlas);
void nk_ejoy2d_font_stash_end(void);

//NK_API void nk_ejoy2d_new_frame(void);	// input
void nk_ejoy2d_render(enum nk_anti_aliasing);
void nk_ejoy2d_shutdown(void);

void nk_ejoy2d_newframe();
void nk_ejoy2d_touch(int id, int x, int y, int touch);

//NK_API void nk_ejoy2d_device_destroy(void);
//NK_API void nk_ejoy2d_device_create(void);

//NK_API void nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint);
//NK_API void nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff);

#endif
