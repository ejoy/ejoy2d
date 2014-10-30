#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#include "shader.h"
#include "screen.h"
#include "texture.h"
#include "array.h"
#include "spritepack.h"
#include "render.h"

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
	shader_program(PROGRAM_PICTURE);
	shader_texture(texid);
	int n = lua_rawlen(L, 2);
	int point = n/4;
	if (point * 4 != n) {
		return luaL_error(L, "Invalid polygon");
	}
	ARRAY(struct vertex_pack, vb, point);
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
		uint16_t u,v;
		lua_pop(L,4);
		screen_trans(&vx,&vy);
		texture_coord(tex, tx, ty, &u, &v);
		vb[i].vx = vx + 1.0f;
		vb[i].vy = vy - 1.0f;
		vb[i].tx = u;
		vb[i].ty = v;
	}
	if (point == 4) {
		shader_draw(vb, color, additive);
	} else {
		shader_drawpolygon(point, vb, color, additive);
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
	lua_pushinteger(L, shader_version());
	return 1;
}

static int
lclear(lua_State *L) {
	uint32_t c = luaL_optunsigned(L, 1, 0xff000000);
	shader_clear(c);

	return 0;
}

static int
lshader_st(lua_State *L) {
    int prog = luaL_checkinteger(L, 1);
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float scale = luaL_optnumber(L, 4, 1.0);

    screen_trans(&x, &y);
    shader_st(prog, x * SCREEN_SCALE, y * SCREEN_SCALE, scale);
    return 0;
}

static int
luniform_bind(lua_State *L) {
	int prog = luaL_checkinteger(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	int n = lua_rawlen(L, 2);
	int i;
	for (i=0;i<n;i++) {
		lua_rawgeti(L, -1, i+1);
		lua_getfield(L, -1, "name");
		const char *name = luaL_checkstring(L, -1);
		lua_getfield(L, -2, "type");
		enum UNIFORM_FORMAT t = luaL_checkinteger(L, -1);
		int loc = shader_adduniform(prog, name, t);
		if (loc != i) {
			return luaL_error(L, "Invalid uniform location %s %d", name, loc);
		}
		lua_pop(L, 3);
	}
	return 0;
}

static int
luniform_set(lua_State *L) {
	int prog = luaL_checkinteger(L, 1);
	shader_program(prog);
	int index = luaL_checkinteger(L, 2);
	enum UNIFORM_FORMAT t = luaL_checkinteger(L, 3);
	float v[16];	// 16 is matrix 4x4
	int n = 0;
	switch(t) {
	case UNIFORM_FLOAT1:
		n = 1;
		break;
	case UNIFORM_FLOAT2:
		n = 2;
		break;
	case UNIFORM_FLOAT3:
		n = 3;
		break;
	case UNIFORM_FLOAT4:
		n = 4;
		break;
	case UNIFORM_FLOAT33:
		n = 9;
		break;
	case UNIFORM_FLOAT44:
		n = 16;
		break;
	default:
		return luaL_error(L, "Invalid uniform format %d", t);
		break;
	}
	int top = lua_gettop(L);
	if (top != n + 3) {
		return luaL_error(L, "Need float %d, only %d passed", n, top - 3);
	}
	int i;
	for (i=0;i<n;i++) {
		v[i] = luaL_checknumber(L, 4+i);
	}
	shader_setuniform(index, t, v);
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
        {"shader_st", lshader_st },
		{"uniform_bind", luniform_bind },
		{"uniform_set", luniform_set },
		{NULL,NULL},
	};
	luaL_newlib(L,l);
	return 1;
}
