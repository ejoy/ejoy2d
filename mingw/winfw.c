#include "opengl.h"
#include "ejoy2dgame.h"
#include "fault.h"
#include "screen.h"
#include "winfw.h"

#include <lauxlib.h>

#include <stdlib.h>
#include <stdio.h>

static struct game *G = NULL;

static const char * startscript =
"local path, script = ...\n"
"assert(script, 'I need a script name')\n"
"path = string.match(path,[[(.*)\\[^\\]*$]])\n"
"package.path = path .. [[\\?.lua;]] .. path .. [[\\?\\init.lua;.\\?.lua;.\\?\\init.lua]]\n"
"dofile(script)\n";

void
ejoy2d_win_init(int argc, char *argv[]) {
	G = ejoy2d_game();
	lua_State *L = ejoy2d_game_lua(G);
	int err = luaL_loadstring(L, startscript);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}
	int i;
	for (i=0;i<argc;i++) {
		lua_pushstring(L, argv[i]);
	}

	err = lua_pcall(L, argc, 0,0);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}

	screen_init(WIDTH,HEIGHT,1.0f);
	ejoy2d_game_start(G);
}

void 
ejoy2d_win_update() {
	ejoy2d_game_update(G, 0.01f);
}

void 
ejoy2d_win_frame() {
	glClear(GL_COLOR_BUFFER_BIT);
	ejoy2d_game_drawframe(G);
}
