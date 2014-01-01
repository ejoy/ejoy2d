#include "opengl.h"
#include "visual.h"
#include "log.h"
#include "module.h"

#include "posix/xwindow.h"

#include <GL/glew.h>
#include <GL/glxew.h>

#include <stdbool.h>

static GLXContext g_context = 0;
static struct X_context *g_X=0;

void 
opengl_release()
{
	if (g_context){
		glXMakeCurrent( g_X->display, None, NULL);
		g_context=NULL;
	}
}

void
opengl_swap_buffers()
{
	if (g_context){
		glXSwapBuffers( g_X->display, g_X->wnd);
	}
}

static int	
glx_init(struct X_context *X)
{
	XVisualInfo *vi;

	int attrib[]={
		GLX_RGBA, 
		GLX_DOUBLEBUFFER, 
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 1,
		None
	};

	if (g_context)
		return 0;

	vi = glXChooseVisual( X->display, X->screen_num, attrib);
	
	if (vi==0) {
		return 1;
	}

	g_context = glXCreateContext( X->display, vi ,  NULL , True);

	if (g_context == 0) {
		return 1;
	}

	if (!glXMakeCurrent( X->display, X->wnd, g_context )) {
		g_context=NULL;
		return 1;
	}
/*
	if (!glXIsDirect( X->display, g_context)) {
		printf("undirect\n");
	}
*/	

	g_X=X;

	return 0;
}


static int
gl_change_context(struct X_context *X)
{
	if (g_context == 0) {
		return 1;
	}

	if (!glXMakeCurrent( X->display, X->wnd, g_context )) {
		g_context=NULL;
		return 1;
	}

	g_X=X;

	return 0;
}


int
opengl_create_window(int w,int h,void *ud)
{
	visual_resize(w,h,ud);
	struct X_context *X=visual_device(0,0);
	return gl_change_context(X);
}

const char *
opengl_error_string(const char *src)
{
	return src;
}

int
opengl_init()
{
	USING(visual);
	USING(log);

	if (glx_init((struct X_context *)visual_device(0,0))) {
		return 1;
	}
	if ( glewInit() != GLEW_OK ) {
		return 1;
	}

	log_write(LOG_INFO,"%s %s\n",glGetString(GL_RENDERER),glGetString(GL_VENDOR));

	return 0;
}
