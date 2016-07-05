#ifndef EJOY_2D_LUASTATE_H
#define EJOY_2D_LUASTATE_H


//changelog
//ver 1: alphamap shader
//       add flush_scene
//       unitmap drawground logic
//ver 2: sprite vb cache
//ver 3: revert unitmap drawground logic
#define _EJOY_VER_ (3)

#include <lua.h>

struct game {
	lua_State *L;
	float real_time;
	float logic_time;
    float vp_logic_time;
    float vp_real_time;
    long update_count;
    int last_draw_call;
    int last_obj_count;
    int cur_fps;
};


struct game * ejoy2d_game();
lua_State * ejoy2d_lua_init();
void ejoy2d_game_exit(struct game *);
void ejoy2d_close_lua(struct game *);
lua_State *  ejoy2d_game_lua(struct game *);
void ejoy2d_handle_error(lua_State *L, const char *err_type, const char *msg);
void ejoy2d_game_logicframe(int);
void ejoy2d_game_vpframe(int);
void ejoy2d_game_start(struct game *);
void ejoy2d_game_update(struct game *, float dt);
void ejoy2d_game_drawframe(struct game *);
int ejoy2d_game_touch(struct game *, int id, float x, float y, int status);
void ejoy2d_game_gesture(struct game *, int type,
                         double x1, double y1, double x2, double y2, int s);
void
ejoy2d_game_message(struct game* G,int id_, const char* state, const char* data, lua_Number n);
void ejoy2d_game_pause(struct game* G);
void ejoy2d_game_resume(struct game* G);

void
ejoy2d_call_lua(lua_State *L, int n, int r);

void ejoy2d_init(lua_State *L);

#endif
