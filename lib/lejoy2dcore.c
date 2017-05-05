
#include <lua.h>
#include <lauxlib.h>

#include "opengl.h"
#include "screen.h"
#include "shader.h"
#include "label.h"
#include "ejoy2dgame.h"

static int
lviewport(lua_State *L) {
	int w = luaL_checkinteger(L,1);
	int h = luaL_checkinteger(L,2);
	glViewport(0, 0, w,h);
	screen_init(w,h,1.0f);
	return 0;
}

static int
lbeginframe(lua_State *L) {
	reset_drawcall_count();
	return 0;
}

static int
lendframe(lua_State *L) {
	shader_flush();
	label_flush();
	return 0;
}

int
luaopen_ejoy2d_core(lua_State *L) {
	if ( glewInit() != GLEW_OK ) {
		return luaL_error(L, "init glew failed");
	}
	ejoy2d_init(L);
	luaL_Reg l[] = {
		{ "viewport", lviewport },
		{ "beginframe", lbeginframe },
		{ "endframe", lendframe },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	return 1;
}
