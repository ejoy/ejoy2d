#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <stdlib.h>

#include "ejoy2dgame.h"
#include "fault.h"
#include "shader.h"
#include "texture.h"
#include "ppm.h"
#include "spritepack.h"
#include "sprite.h"
#include "lmatrix.h"
#include "label.h"
#include "particle.h"

#define LOGIC_FRAME 30

#define EJOY_INIT "EJOY2D_INIT"
#define EJOY_UPDATE "EJOY2D_UPDATE"
#define EJOY_DRAWFRAME "EJOY2D_DRAWFRAME"
#define EJOY_TOUCH "EJOY2D_TOUCH"

#define TRACEBACK_FUNCTION 1
#define UPDATE_FUNCTION 2
#define DRAWFRAME_FUNCTION 3
#define TOP_FUNCTION 3

struct game {
	lua_State *L;
	float real_time;
	float logic_time;
};

static int
_panic(lua_State *L) {
	const char * err = lua_tostring(L,-1);
	fault("%s", err);
	return 0;
}

static int
linject(lua_State *L) {
	static const char * ejoy_callback[] = {
		EJOY_INIT,
		EJOY_UPDATE,
		EJOY_DRAWFRAME,
		EJOY_TOUCH,
	};
	int i;
	for (i=0;i<sizeof(ejoy_callback)/sizeof(ejoy_callback[0]);i++) {
		lua_getfield(L, lua_upvalueindex(1), ejoy_callback[i]);
		if (!lua_isfunction(L,-1)) {
			return luaL_error(L, "%s is not found", ejoy_callback[i]);
		}
		lua_setfield(L, LUA_REGISTRYINDEX, ejoy_callback[i]);
	}
	return 0;
}

static int
ejoy2d_framework(lua_State *L) {
	luaL_Reg l[] = {
		{ "inject", linject },
		{ NULL, NULL },
	};
	luaL_newlibtable(L, l);
	lua_pushvalue(L,-1);
	luaL_setfuncs(L,l,1);
	return 1;
}

static void
checkluaversion(lua_State *L) {
	const lua_Number *v = lua_version(L);
	if (v != lua_version(NULL))
		fault("multiple Lua VMs detected");
	else if (*v != LUA_VERSION_NUM) {
		fault("Lua version mismatch: app. needs %f, Lua core provides %f",
			LUA_VERSION_NUM, *v);
	}
}

#define STR_VALUE(arg)	#arg
#define _OS_STRING(name) STR_VALUE(name)
#define OS_STRING _OS_STRING(EJOY2D_OS)

struct game *
ejoy2d_game() {
	struct game *G = (struct game *)malloc(sizeof(*G));
	lua_State *L = luaL_newstate();
	checkluaversion(L);
	lua_pushliteral(L, OS_STRING);
	lua_setglobal(L , "OS");

	G->L = L;
	G->real_time = 0;
	G->logic_time = 0;
	lua_atpanic(L, _panic);
	luaL_openlibs(L);
	luaL_requiref(L, "ejoy2d.shader.c", ejoy2d_shader, 0);
	luaL_requiref(L, "ejoy2d.framework", ejoy2d_framework, 0);
	luaL_requiref(L, "ejoy2d.ppm", ejoy2d_ppm, 0);
	luaL_requiref(L, "ejoy2d.spritepack.c", ejoy2d_spritepack, 0);
	luaL_requiref(L, "ejoy2d.sprite.c", ejoy2d_sprite, 0);
	luaL_requiref(L, "ejoy2d.matrix.c", ejoy2d_matrix, 0);
	luaL_requiref(L, "ejoy2d.particle.c", ejoy2d_particle, 0);

	lua_settop(L,0);

	shader_init();
	label_load();

	return G;
}

void
ejoy2d_game_exit(struct game *G) {
	label_unload();
	texture_exit();
	shader_unload();
	lua_close(G->L);
	free(G);
}

lua_State *
ejoy2d_game_lua(struct game *G) {
	return G->L;
}

static int
traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else if (!lua_isnoneornil(L, 1)) {
	if (!luaL_callmeta(L, 1, "__tostring"))
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

void
ejoy2d_game_start(struct game *G) {
	lua_State *L = G->L;
	lua_getfield(L, LUA_REGISTRYINDEX, EJOY_INIT);
	lua_call(L, 0, 0);
	assert(lua_gettop(L) == 0);
	lua_pushcfunction(L, traceback);
	lua_getfield(L,LUA_REGISTRYINDEX, EJOY_UPDATE);
	lua_getfield(L,LUA_REGISTRYINDEX, EJOY_DRAWFRAME);
}

static int
call(lua_State *L, int n, int r) {
	int err = lua_pcall(L, n, r, TRACEBACK_FUNCTION);
	switch(err) {
	case LUA_OK:
		break;
	case LUA_ERRRUN:
		fault("LUA_ERRRUN : %s\n", lua_tostring(L,-1));
		break;
	case LUA_ERRMEM:
		fault("LUA_ERRMEM : %s\n", lua_tostring(L,-1));
		break;
	case LUA_ERRERR:
		fault("LUA_ERRERR : %s\n", lua_tostring(L,-1));
		break;
	case LUA_ERRGCMM:
		fault("LUA_ERRGCMM : %s\n", lua_tostring(L,-1));
		break;
	default:
		fault("Unknown Lua error: %d\n", err);
		break;
	}
	return err;
}

static void
logic_frame(lua_State *L) {
	lua_pushvalue(L, UPDATE_FUNCTION);
	call(L, 0, 0);
	lua_settop(L, TOP_FUNCTION);
}

void
ejoy2d_game_update(struct game *G, float time) {
	if (G->logic_time == 0) {
		G->real_time = 1.0f/LOGIC_FRAME;
	} else {
		G->real_time += time;
	}
	while (G->logic_time < G->real_time) {
		logic_frame(G->L);
		G->logic_time += 1.0f/LOGIC_FRAME;
	}
}

void
ejoy2d_game_drawframe(struct game *G) {
	lua_pushvalue(G->L, DRAWFRAME_FUNCTION);
	call(G->L, 0, 0);
	lua_settop(G->L, TOP_FUNCTION);
	shader_flush();
	label_flush();
}

void
ejoy2d_game_touch(struct game *G, int id, float x, float y, int status) {
	lua_getfield(G->L, LUA_REGISTRYINDEX, EJOY_TOUCH);
	lua_pushnumber(G->L, x);
	lua_pushnumber(G->L, y);
	lua_pushinteger(G->L, status+1);
	lua_pushinteger(G->L, id);
	call(G->L, 4, 0);
}


