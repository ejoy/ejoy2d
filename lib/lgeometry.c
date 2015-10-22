#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <math.h>

#include "shader.h"
#include "screen.h"
#include "renderbuffer.h"
#include "spritepack.h"
#include "array.h"
#include "scissor.h"

static int PROGRAM = 0;

static int
lsetprogram(lua_State *L) {
	PROGRAM = luaL_checkinteger(L,1);
	return 0;
}

static uint32_t
convert_color(uint32_t c) {
	uint32_t alpha = (c >> 24) & 0xff;
	if (alpha == 0xff) {
		return c;
	}
	if (alpha == 0) {
		return c | 0xff000000;
	}
	uint32_t red = (c >> 16) & 0xff;
	uint32_t green = (c >> 8) & 0xff;
	uint32_t blue = (c) & 0xff;
	red = red * alpha / 255;
	green = green * alpha / 255;
	blue = blue * alpha / 255;

	return alpha << 24 | red << 16 | green << 8 | blue;
}

/*
  float[4] endpointer x1,y1,x2,y2
  uint32_t color
 */
static int
lline(lua_State *L) {
	float x1 = luaL_checknumber(L, 1) * SCREEN_SCALE;
	float y1 = luaL_checknumber(L, 2) * SCREEN_SCALE;
	float x2 = luaL_checknumber(L, 3) * SCREEN_SCALE;
	float y2 = luaL_checknumber(L, 4) * SCREEN_SCALE;
	uint32_t color = convert_color(luaL_checkinteger(L, 5));
	struct vertex_pack vp[4];
	vp[0].vx = x1;
	vp[0].vy = y1;
	vp[1].vx = x2;
	vp[1].vy = y2;
	if (fabs(x1-x2) > fabs(y1-y2)) {
		vp[2].vx = x2;
		vp[2].vy = y2+SCREEN_SCALE;
		vp[3].vx = x1;
		vp[3].vy = y1+SCREEN_SCALE;
	} else {
		vp[2].vx = x2+SCREEN_SCALE;
		vp[2].vy = y2;
		vp[3].vx = x1+SCREEN_SCALE;
		vp[3].vy = y1;
	}

	int i;
	for (i=0;i<4;i++) {
		vp[i].tx = 0;
		vp[i].ty = 0;
		screen_trans(&vp[i].vx, &vp[i].vy);
	}

	shader_program(PROGRAM, NULL);
	shader_draw(vp, color, 0);

	return 0;
}

/*
	float x,y
	float w,h
	uint32_t color 
*/
static int
lbox(lua_State *L) {
	float x = luaL_checknumber(L, 1) * SCREEN_SCALE;
	float y = luaL_checknumber(L, 2) * SCREEN_SCALE;
	float w = luaL_checknumber(L, 3) * SCREEN_SCALE;
	float h = luaL_checknumber(L, 4) * SCREEN_SCALE;
	uint32_t color = convert_color(luaL_checkinteger(L, 5));
	struct vertex_pack vp[4];
	vp[0].vx = x;
	vp[0].vy = y;
	vp[1].vx = x+w;
	vp[1].vy = y;
	vp[2].vx = x+w;
	vp[2].vy = y+h;
	vp[3].vx = x;
	vp[3].vy = y+h;

	int i;
	for (i=0;i<4;i++) {
		vp[i].tx = 0;
		vp[i].ty = 0;
		screen_trans(&vp[i].vx, &vp[i].vy);
	}

	shader_program(PROGRAM, NULL);
	shader_draw(vp, color, 0);

	return 0;
}

/*
	float x,y
	float w,h
	uint32_t color
	float width
 */

static int
lframe(lua_State *L) {
	float x = luaL_checknumber(L, 1) * SCREEN_SCALE;
	float y = luaL_checknumber(L, 2) * SCREEN_SCALE;
	float w = luaL_checknumber(L, 3) * SCREEN_SCALE;
	float h = luaL_checknumber(L, 4) * SCREEN_SCALE;
	uint32_t color = convert_color(luaL_checkinteger(L, 5));
	float width = luaL_optnumber(L, 6, 1.0f) * SCREEN_SCALE;
	struct vertex_pack vp[6];
	int i;
	for (i=0;i<6;i++) {
		vp[i].tx = 0;
		vp[i].ty = 0;
	}
	shader_program(PROGRAM, NULL);

	/*
		0 (x,y)                      1 (x+w,y)
		+----------------------------+
		|\                          /|
		| +------------------------+ |
		| |3                      2| |
		| |                        | |
		| |                        | |
		| |                        | |
		| |4                     3'| |
		| +------------------------+ |
		|/                          \|
		+----------------------------+
		5                             0'
	*/

	vp[0].vx = x;
	vp[0].vy = y;
	vp[1].vx = x+w;
	vp[1].vy = y;

	vp[2].vx = x+w-width;
	vp[2].vy = y+width;
	vp[3].vx = x+width;
	vp[3].vy = y+width;

	vp[4].vx = x+width;
	vp[4].vy = y+h-width;
	vp[5].vx = x;
	vp[5].vy = y+h;

	for (i=0;i<6;i++) {
		screen_trans(&vp[i].vx, &vp[i].vy);
	}
	shader_drawpolygon(6, vp, color, 0);

	vp[0].vx = x+w;
	vp[0].vy = y+h;
	vp[1].vx = x+w;
	vp[1].vy = y;

	vp[2].vx = x+w-width;
	vp[2].vy = y+width;
	vp[3].vx = x+w-width;
	vp[3].vy = y+h-width;

	vp[4].vx = x+width;
	vp[4].vy = y+h-width;
	vp[5].vx = x;
	vp[5].vy = y+h;

	for (i=0;i<6;i++) {
		screen_trans(&vp[i].vx, &vp[i].vy);
	}
	shader_drawpolygon(6, vp, color, 0);

	return 0;
}


/*
	table float[]
	uint32_t color
 */

static int
lpolygon(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	uint32_t color = convert_color(luaL_checkinteger(L, 2));
	int n = lua_rawlen(L, 1);
	int point = n/2;
	if (point * 2 != n) {
		return luaL_error(L, "Invalid polygon");
	}
	ARRAY(struct vertex_pack, vb, point);
	int i;
	for (i=0;i<point;i++) {
		lua_rawgeti(L, 1, i*2+1);
		lua_rawgeti(L, 1, i*2+2);
		float vx = lua_tonumber(L, -2) * SCREEN_SCALE;
		float vy = lua_tonumber(L, -1) * SCREEN_SCALE;
		lua_pop(L,2);
		screen_trans(&vx,&vy);
		vb[i].vx = vx;
		vb[i].vy = vy;
		vb[i].tx = 0;
		vb[i].ty = 0;
	}
	shader_program(PROGRAM, NULL);
	if (point == 4) {
		shader_draw(vb, color, 0);
	} else {
		shader_drawpolygon(point, vb, color, 0);
	}
	return 0;
}

static int
lscissor(lua_State *L) {
	if (lua_gettop(L) == 0) {
		scissor_pop();
	} else {
		int x = luaL_checkinteger(L,1);
		int y = luaL_checkinteger(L,2);
		int w = luaL_checkinteger(L,3);
		int h = luaL_checkinteger(L,4);
		scissor_push(x,y,w,h);
	}
	return 0;
}

int
ejoy2d_geometry(lua_State *L) {
	luaL_Reg l[] = {
		{"setprogram", lsetprogram},
		{"line", lline },
		{"box", lbox },
		{"frame", lframe },
		{"polygon", lpolygon },
		{"scissor", lscissor },

		{NULL,NULL},
	};
	luaL_newlib(L,l);

	return 1;
}
