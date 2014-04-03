#include <lua.h>
#include <lauxlib.h>

#include "shader.h"
#include "screen.h"
#include "texture.h"
#include "array.h"

static int
lload(lua_State *L) {
	int prog = (int)luaL_checkinteger(L,1);
	const char *fs = luaL_checkstring(L, 2);
	const char *vs = luaL_checkstring(L, 3);
	shader_load(prog, fs, vs);
	return 0;
}

static int
lunload(lua_State *L) {
	shader_unload();
	return 0;
}

/*
	int texture
	table float[16]  
	uint32_t color
	uint32_t additive
 */
static int
ldraw(lua_State *L) {
	int tex = (int)luaL_checkinteger(L,1);
	int texid = texture_glid(tex);
	if (texid == 0) {
		lua_pushboolean(L,0);
		return 1;
	} 
	luaL_checktype(L, 2, LUA_TTABLE);
	uint32_t color = 0xffffffff;

	if (!lua_isnoneornil(L,3)) {
		color = (uint32_t)lua_tounsigned(L,3);
	}
	uint32_t additive = (uint32_t)luaL_optunsigned(L,4,0);
	shader_program(PROGRAM_PICTURE,additive);
	shader_texture(texid);
	int n = lua_rawlen(L, 2);
	int point = n/4;
	if (point * 4 != n) {
		return luaL_error(L, "Invalid polygon");
	}
	ARRAY(float, vb, n);
	int i;
	for (i=0;i<point;i++) {
		lua_rawgeti(L, 2, i*2+1);
		lua_rawgeti(L, 2, i*2+2);
		lua_rawgeti(L, 2, point*2+i*2+1);
		lua_rawgeti(L, 2, point*2+i*2+2);
		float tx = lua_tonumber(L, -4);
		float ty = lua_tonumber(L, -3);
		float vx = lua_tonumber(L, -2);
		float vy = lua_tonumber(L, -1);
		lua_pop(L,4);
		screen_trans(&vx,&vy);
		texture_coord(tex, &tx, &ty);
		vb[i*4+0] = vx + 1.0f;
		vb[i*4+1] = vy - 1.0f;
		vb[i*4+2] = tx;
		vb[i*4+3] = ty;
	}
	if (point == 4) {
		shader_draw(vb, color);
	} else {
		shader_drawpolygon(point, vb, color);
	}
	return 0;
}

static int
lblend(lua_State *L) {
	if (lua_isnoneornil(L,1)) {
		shader_defaultblend();
	} else {
		int m1 = (int)luaL_checkinteger(L,1);
		int m2 = (int)luaL_checkinteger(L,2);
		shader_blend(m1,m2);
	}
	return 0;
}

static int
lversion(lua_State *L) {
	lua_pushinteger(L, OPENGLES);
	return 1;
}

static int
lclear(lua_State *L) {
	uint32_t c = luaL_optunsigned(L, 1, 0xff000000);
	float a = ((c >> 24) & 0xff ) / 255.0;
	float r = ((c >> 16) & 0xff ) / 255.0;
	float g = ((c >> 8) & 0xff ) / 255.0;
	float b = ((c >> 0) & 0xff ) / 255.0;
	glClearColor(r,g,b,a);
	glClear(GL_COLOR_BUFFER_BIT);

	return 0;
}

int 
ejoy2d_shader(lua_State *L) {
	luaL_Reg l[] = {
		{"load", lload},
		{"unload", lunload},
		{"draw", ldraw},
		{"blend", lblend},
		{"clear", lclear},
		{"version", lversion},
		{NULL,NULL},
	};
	luaL_newlib(L,l);
	return 1;
}
