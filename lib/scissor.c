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
scissor_push(int x, int y, int w, int h) {
	assert(S.depth < SCISSOR_MAX);
	shader_flush();
	if (S.depth == 0) {
		glEnable(GL_SCISSOR_TEST);
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
	struct box * s = &S.s[S.depth];
	screen_scissor(s->x,s->y,s->width,s->height);
}
