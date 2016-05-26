
#include "opengl.h"
#include "ejoy2dgame.h"
#include "fault.h"
#include "screen.h"
#include "winfw.h"

#include <lauxlib.h>

#include <stdlib.h>
#include <stdio.h>

struct WINDOWGAME {
	struct game *game;
	int intouch;
};

static struct WINDOWGAME *G = NULL;

static const char * startscript =
"local path, script = ...\n"
"require(\"ejoy2d.framework\").WorkDir = path..'/'\n"
"assert(script, 'I need a script name')\n"
"script = path..[[/]]..script\n"
"package.path = path .. [[/?.lua;]] .. path .. [[/?/init.lua;./?.lua;./?/init.lua]]\n"
"local f = loadfile(script)\n"
"f(script)\n";

static struct WINDOWGAME *
create_game() {
	struct WINDOWGAME * g = malloc(sizeof(*g));
	g->game = ejoy2d_game();
	g->intouch = 0;
	return g;
}

static int
traceback(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg == NULL) {
	if (luaL_callmeta(L, 1, "__tostring") &&
		lua_type(L, -1) == LUA_TSTRING)
		return 1; 
	else
		msg = lua_pushfstring(L, "(error object is a %s value)",
								luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1); 
	return 1;
}


void
ejoy2d_win_init(int orix, int oriy, int width, int height, float scale, const char* folder) {
	G = create_game();
	lua_State *L = ejoy2d_game_lua(G->game);
	lua_pushcfunction(L, traceback);
	int tb = lua_gettop(L);
	int err = luaL_loadstring(L, startscript);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}

  lua_pushstring(L, folder);
  lua_pushstring(L, "examples/ex09.lua");

	err = lua_pcall(L, 2, 0, tb);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}

	lua_pop(L,1);

	screen_init(width,height,scale);
	ejoy2d_game_start(G->game);
}

void
ejoy2d_win_update() {
	ejoy2d_game_update(G->game, 0.01f);
}

void
ejoy2d_win_frame() {
	ejoy2d_game_drawframe(G->game);
}

void
ejoy2d_win_resume(){
    ejoy2d_game_resume(G->game);
}

void
ejoy2d_win_touch(int x, int y,int touch) {
	switch (touch) {
	case TOUCH_BEGIN:
		G->intouch = 1;
		break;
	case TOUCH_END:
		G->intouch = 0;
		break;
	case TOUCH_MOVE:
		if (!G->intouch) {
			return;
		}
		break;
	}
	// windows only support one touch id (0)
	int id = 0;
	ejoy2d_game_touch(G->game, id, x,y,touch);
}

