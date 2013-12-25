#include "screen.h"
#include "opengl.h"

struct screen {
	float width;
	float height;
	float scale;
	float invw;
	float invh;
};

static struct screen SCREEN;

void
screen_init(float w, float h, float scale) {
	SCREEN.width = w;
	SCREEN.height = h;
	SCREEN.scale = scale;
	SCREEN.invw = 2.0f / 16.0f / w;
	SCREEN.invh = -2.0f / 16.0f / h;
	glViewport(0,0,w * scale,h * scale);
}

void 
screen_trans(float *x, float *y) {
	*x *= SCREEN.invw;
	*y *= SCREEN.invh;
}


