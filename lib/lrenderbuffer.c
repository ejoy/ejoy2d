#include "sprite.h"
#include "renderbuffer.h"
#include "shader.h"

#include <lua.h>
#include <lauxlib.h>


static int
ldelbuffer(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)lua_touserdata(L, 1);
	renderbuffer_unload(rb);
	return 0;
}

static int
laddsprite(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)luaL_checkudata(L, 1, "renderbuffer");
	struct sprite * spr = (struct sprite *)lua_touserdata(L, 2);
	if (spr == NULL) {
		return luaL_error(L, "Need sprite");
	}
	int r = renderbuffer_drawsprite(rb, spr);
	if (r < 0) {
		return luaL_error(L, "Add failed");
	}
	lua_pushboolean(L, r == 0);
	return 1;
}

static int
lupload(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)luaL_checkudata(L, 1, "renderbuffer");
	renderbuffer_upload(rb);

	return 0;
}

static int
ldrawbuffer(lua_State *L) {
	// todo: srt
	struct render_buffer *rb = (struct render_buffer *)luaL_checkudata(L, 1, "renderbuffer");
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float scale = luaL_optnumber(L, 4, 1.0);
	shader_drawbuffer(rb, x * SCREEN_SCALE,y * SCREEN_SCALE,scale);
	return 0;
}

static int
lnewbuffer(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)lua_newuserdata(L, sizeof(*rb));
	renderbuffer_init(rb);
	if (luaL_newmetatable(L, "renderbuffer")) {
		luaL_Reg l[] = {
			{ "add", laddsprite },
			{ "upload", lupload },
			{ "draw", ldrawbuffer },
			{ NULL, NULL },
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, ldelbuffer);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	return 1;
}

int
ejoy2d_renderbuffer(lua_State *L) {
	luaL_Reg l[] ={
		{ "new", lnewbuffer },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	return 1;
}
