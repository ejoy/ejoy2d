#include "sprite.h"
#include "renderbuffer.h"
#include "shader.h"
#include "screen.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <float.h>

static int
ldelbuffer(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)lua_touserdata(L, 1);
    if (rb->vb) {
        free(rb->vb);
        rb->vb = 0;
    }

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

    float sx = scale, sy = scale;
    screen_trans(&sx, &sy);
    float tx = x * SCREEN_SCALE, ty = y * SCREEN_SCALE;
    screen_trans(&tx, &ty);

    float x1 = sx * rb->corner[0] + tx;
    float x2 = sx * rb->corner[1] + tx;
    float y1 = sy * rb->corner[2] + ty;
    float y2 = sy * rb->corner[3] + ty;

    // renderbuffer 屏幕裁剪
    if ((x1 <= 0.0f && x2 <= 0.0f) || (x1 >= 2.0f && x2 >= 2.0f) ||
            (y1 <= -2.0f && y2 <= -2.0f) || (y1 >= 0.0f && y2 >= 0.0f)) {
        return 0;
    }

	shader_drawbuffer(rb, x * SCREEN_SCALE,y * SCREEN_SCALE,scale);

	return 0;
}

static int
lfree_vb(lua_State *L) {
	struct render_buffer *rb = (struct render_buffer *)luaL_checkudata(L, 1, "renderbuffer");
    if (rb->vb) {
        free(rb->vb);
        rb->vb_size = 0;
        rb->vb = NULL;
    }

    return 0;
}

static int
lnewbuffer(lua_State *L) {
    int size = luaL_optinteger(L, 1, MAX_COMMBINE);

	struct render_buffer *rb = (struct render_buffer *)lua_newuserdata(L, sizeof(*rb));

    rb->vb_size = size;
    rb->vb = (struct quad *)malloc(sizeof(struct quad) * size);

    rb->corner[0] = rb->corner[2] = FLT_MAX;
    rb->corner[1] = rb->corner[3] = 0;

	renderbuffer_init(rb);
	if (luaL_newmetatable(L, "renderbuffer")) {
		luaL_Reg l[] = {
			{ "add", laddsprite },
			{ "upload", lupload },
			{ "draw", ldrawbuffer },
            { "free_vb", lfree_vb },
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
