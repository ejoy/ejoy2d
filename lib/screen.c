#include "screen.h"
#include "opengl.h"
#include "spritepack.h"

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
	SCREEN.invw = 2.0f / SCREEN_SCALE / w;
	SCREEN.invh = -2.0f / SCREEN_SCALE / h;
	glViewport(0,0,w * scale,h * scale);
}

void 
screen_trans(float *x, float *y) {
	*x *= SCREEN.invw;
	*y *= SCREEN.invh;
}


