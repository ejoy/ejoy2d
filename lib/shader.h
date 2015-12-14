#ifndef EJOY_2D_SHADER_H
#define EJOY_2D_SHADER_H

#include "renderbuffer.h"
#include "render.h"

#include <stdint.h>
#include <lua.h>

#define PROGRAM_DEFAULT -1
#define PROGRAM_PICTURE 0
#define PROGRAM_RENDERBUFFER 1
#define PROGRAM_TEXT 2
#define PROGRAM_TEXT_EDGE 3
#define PROGRAM_GUI_TEXT 4
#define PROGRAM_GUI_EDGE 5

struct material;

void shader_init();
void shader_load(int prog, const char *fs, const char *vs, int texture, const char ** texture_uniform_name);
void shader_unload();
void shader_blend(int m1,int m2);
void shader_defaultblend();
void shader_texture(int id, int channel);
void shader_draw(const struct vertex_pack vb[4],uint32_t color,uint32_t additive);
void shader_drawpolygon(int n, const struct vertex_pack *vb, uint32_t color, uint32_t additive);
void shader_program(int n, struct material *);
void shader_flush();
void shader_clear(unsigned long argb);
int shader_version();
void shader_scissortest(int enable);

int ejoy2d_shader(lua_State *L);

void shader_drawbuffer(struct render_buffer * rb, float x, float y, float s);

int shader_adduniform(int prog, const char * name, enum UNIFORM_FORMAT t);
void shader_setuniform(int prog, int index, enum UNIFORM_FORMAT t, float *v);
int shader_uniformsize(enum UNIFORM_FORMAT t);

// these api may deprecated later
void shader_reset();
void shader_mask(float x, float y);
void reset_drawcall_count();
int drawcall_count();

#endif
