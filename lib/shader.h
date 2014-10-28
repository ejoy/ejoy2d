#ifndef EJOY_2D_SHADER_H
#define EJOY_2D_SHADER_H

#include "renderbuffer.h"

#include <stdint.h>
#include <lua.h>

#define PROGRAM_DEFAULT -1
#define PROGRAM_PICTURE 0
#define PROGRAM_RENDERBUFFER 1
#define PROGRAM_TEXT 2
#define PROGRAM_TEXT_EDGE 3

void shader_init();
void shader_load(int prog, const char *fs, const char *vs);
void shader_unload();
void shader_blend(int m1,int m2);
void shader_defaultblend();
void shader_texture(int id);
void shader_mask(float x, float y);
void shader_st(int prog, float x, float y, float s);
void shader_draw(const struct vertex_pack vb[4],uint32_t color,uint32_t additive);
void shader_drawpolygon(int n, const struct vertex_pack *vb, uint32_t color, uint32_t additive);
void shader_program(int n);
void shader_flush();

// 还原当前的环境，比如rt渲染之后
void shader_reset();

int ejoy2d_shader(lua_State *L);


void reset_drawcall_count();
int drawcall_count();

void shader_drawbuffer(struct render_buffer * rb, float x, float y, float s);

#endif
