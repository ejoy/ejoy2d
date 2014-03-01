#include <windows.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <string.h>
#include <stdlib.h>
#include "winfw.h"

#define CLASSNAME L"EJOY"
#define WINDOWNAME L"EJOY"
#define WINDOWSTYLE (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX)

struct ESContext {
	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLSurface egl_surface;
};

static ESContext cnt = { 0 };

static bool 
init_window(HWND hWnd, ESContext* cnt) {
	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLConfig config;
	EGLint context_attribs[] = { 
		EGL_CONTEXT_CLIENT_VERSION, 2, 
		EGL_NONE, EGL_NONE 
	};
	EGLint config_attribs[] = {
		EGL_RED_SIZE, EGL_DONT_CARE,
		EGL_GREEN_SIZE, EGL_DONT_CARE,
		EGL_BLUE_SIZE, EGL_DONT_CARE,
		EGL_ALPHA_SIZE, EGL_DONT_CARE,
		EGL_DEPTH_SIZE, EGL_DONT_CARE,
		EGL_STENCIL_SIZE, EGL_DONT_CARE,
		EGL_SAMPLE_BUFFERS, 0,
		EGL_NONE
	};
	EGLint surface_attribs[] = {
		EGL_POST_SUB_BUFFER_SUPPORTED_NV, EGL_FALSE,
		EGL_NONE, EGL_NONE
	};

	display = eglGetDisplay(GetDC(hWnd));
	if (display == EGL_NO_DISPLAY) {
		return false;
	}

	if (!eglInitialize(display, &majorVersion, &minorVersion)) {
		return false;
	}

	if (!eglGetConfigs(display, NULL, 0, &numConfigs)) {
		return false;
	}

	if (!eglChooseConfig(display, config_attribs, &config, 1, &numConfigs)) {
		return false;
	}

	surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)hWnd, surface_attribs);
	if (surface == EGL_NO_SURFACE) {
		return false;
	}

	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
	if (context == EGL_NO_CONTEXT) {
		return false;
	}

	if (!eglMakeCurrent(display, surface, surface, context)) {
		return false;
	}

	if (cnt) {
		cnt->egl_display = display;
		cnt->egl_surface = surface;
		cnt->egl_context = context;
	}

	return true;
}

static void
get_xy(LPARAM lParam, int *x, int *y) {
	*x = (short)(lParam & 0xffff); 
	*y = (short)((lParam>>16) & 0xffff); 
}

LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		SetTimer(hWnd,0,10,NULL);
		break;
	case WM_PAINT: {
		if (GetUpdateRect(hWnd, NULL, FALSE)) {
			ejoy2d_win_frame();
			eglSwapBuffers(cnt.egl_display, cnt.egl_surface);
			ValidateRect(hWnd, NULL);
		}
		return 0;
	}
	case WM_TIMER : {
		ejoy2d_win_update();
		InvalidateRect(hWnd, NULL , FALSE);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONUP: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_END);
		break;
	}
	case WM_LBUTTONDOWN: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_BEGIN);
		break;
	}
	case WM_MOUSEMOVE: {
		int x,y;
		get_xy(lParam, &x, &y); 
		ejoy2d_win_touch(x,y,TOUCH_MOVE);
		break;
	}
	}
	return DefWindowProcW(hWnd, message, wParam, lParam);
}

static void
register_class() {
	WNDCLASSW wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = GetModuleHandleW(0);
	wndclass.hIcon = 0;
	wndclass.hCursor = 0;
	wndclass.hbrBackground = 0;
	wndclass.lpszMenuName = 0; 
	wndclass.lpszClassName = CLASSNAME;

	RegisterClassW(&wndclass);
}

static HWND
create_window(int w, int h) {
	RECT rect;

	rect.left=0;
	rect.right=w;
	rect.top=0;
	rect.bottom=h;

	AdjustWindowRect(&rect,WINDOWSTYLE,0);

	HWND wnd=CreateWindowExW(0,CLASSNAME,WINDOWNAME,
		WINDOWSTYLE, CW_USEDEFAULT,0,
		rect.right-rect.left,rect.bottom-rect.top,
		0,0,
		GetModuleHandleW(0),
		0);

	if (!init_window(wnd, &cnt)) {
		exit(1);
	}
	return wnd;
}

int
main(int argc, char *argv[]) {
	register_class();
	HWND wnd = create_window(WIDTH,HEIGHT);
	ejoy2d_win_init(argc, argv);
	
	ShowWindow(wnd, SW_SHOWDEFAULT);
	UpdateWindow(wnd);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
