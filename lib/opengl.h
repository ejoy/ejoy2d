#ifndef ejoy2d_opengl_h
#define ejoy2d_opengl_h

#if defined( __APPLE__ ) && !defined(__MACOSX)

#define OPENGLES 2
#include <OpenGLES/ES2/gl.h>

#else

#define OPENGLES 0
#include <GL/glew.h>

#endif

#endif
