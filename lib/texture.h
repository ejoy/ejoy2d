#ifndef EJOY_2D_TEXTURE_H
#define EJOY_2D_TEXTURE_H

#include "opengl.h"

#define Texture2DPixelFormat_RGBA8888 1
#define Texture2DPixelFormat_RGBA4444 2
#define Texture2DPixelFormat_PVRTC4 3
#define Texture2DPixelFormat_PVRTC2 4
#define	Texture2DPixelFormat_RGB888 5
#define Texture2DPixelFormat_RGB565 6
#define Texture2DPixelFormat_A8 7

const char * texture_load(int id, int type, int width, int height, void *buffer);
void texture_unload(int id);
GLuint texture_glid(int id);
void texture_coord(int id, float *x, float *y);
void texture_clearall();
void texture_exit();

///创建一个rt，格式rgba
const char* texture_new_rt(int id, int width, int height);
///激活id 的 rt
const char* texture_active_rt(int id);
#endif
