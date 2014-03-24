#include "scissor.h"
#include "opengl.h"
#include "screen.h"
#include "shader.h"

#include <assert.h>

#define SCISSOR_MAX 8

struct box {
	int x;
	int y;
	int width;
	int height;
};

struct scissor {
	int depth;
	struct box s[SCISSOR_MAX];
};

static struct scissor S;

void
intersection(struct box * b, int * x, int * y, int * w, int * h) {
	int newx = b->x > *x ? b->x : *x;
	int newy = b->y > *y ? b->y : *y;
  
	int bx = b->x + b->width;
	int by = b->y + b->height;
	int ax = *x + *w;
	int ay = *y + *h;
	int neww = (bx > ax ? ax : bx) - newx;
	int newh = (by > ay ? ay : by) - newy;
  
	*x = newx;
	*y = newy;
	*w = neww;
	*h = newh;
}

void 
scissor_push(int x, int y, int w, int h) {
	assert(S.depth < SCISSOR_MAX);
	shader_flush();
	if (S.depth == 0) {
		glEnable(GL_SCISSOR_TEST);
	}
  
	if (S.depth >= 1) {
		intersection(&S.s[S.depth-1], &x, &y, &w, &h);
	}
  
	struct box * s = &S.s[S.depth++];
	s->x = x;
	s->y = y;
	s->width = w;
	s->height = h;
	screen_scissor(s->x,s->y,s->width,s->height);
}

void 
scissor_pop() {
	assert(S.depth > 0);
	shader_flush();
	--S.depth;
	if (S.depth == 0) {
		glDisable(GL_SCISSOR_TEST);
		return;
	}
	struct box * s = &S.s[S.depth-1];
	screen_scissor(s->x,s->y,s->width,s->height);
}
