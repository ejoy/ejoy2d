#ifndef NK_EJOY2D_H_
#define NK_EJOY2D_H_

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#include "nuklear.h"

NK_API struct nk_context *nk_ejoy2d_init(struct render *R, int w, int h);
NK_API void nk_ejoy2d_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_ejoy2d_font_stash_end(void);

//NK_API void nk_ejoy2d_new_frame(void);	// input
NK_API void nk_ejoy2d_render(enum nk_anti_aliasing);
NK_API void nk_ejoy2d_shutdown(void);

//NK_API void nk_ejoy2d_device_destroy(void);
//NK_API void nk_ejoy2d_device_create(void);

//NK_API void nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint);
//NK_API void nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff);

#endif
