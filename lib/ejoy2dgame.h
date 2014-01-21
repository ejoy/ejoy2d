#ifndef EJOY_2D_LUASTATE_H
#define EJOY_2D_LUASTATE_H

#include <lua.h>

struct game;

struct game * ejoy2d_game();
void ejoy2d_game_exit(struct game *);
lua_State *  ejoy2d_game_lua(struct game *);
void ejoy2d_game_logicframe(int);
void ejoy2d_game_start(struct game *);
void ejoy2d_game_update(struct game *, float dt);
void ejoy2d_game_drawframe(struct game *);
int ejoy2d_game_touch(struct game *, int id, float x, float y, int status);
void ejoy2d_game_gesture(struct game *, int type,
                         double x1, double y1, double x2, double y2, int s);

#endif
