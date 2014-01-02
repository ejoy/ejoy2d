#include "spritepack.h"
#include "sprite.h"
#include "label.h"
#include "shader.h"

#include <lua.h>
#include <lauxlib.h>

#define SRT_X 1
#define SRT_Y 2
#define SRT_SX 3
#define SRT_SY 4
#define SRT_ROT 5
#define SRT_SCALE 6

static struct sprite *
newlabel(lua_State *L, struct pack_label *label) {
	int sz = sizeof(struct sprite) + sizeof(struct pack_label);
	struct sprite *s = (struct sprite *)lua_newuserdata(L, sz);
	// label never has a child
	struct pack_label * pl = (struct pack_label *)(s+1);
	*pl = *label;
	s->s.label = pl;
	s->t.mat = NULL;
	s->t.color = 0xffffffff;
	s->t.additive = 0;
	s->t.program = PROGRAM_DEFAULT;
	s->message = false;
	s->visible = true;
	s->name = NULL;
	s->id = 0;
	s->type = TYPE_LABEL;
	s->start_frame = 0;
	s->total_frame = 0;
	s->frame = 0;
	s->data.text = NULL;
	return s;
}

/*
	integer width
	integer height
	integer size
	uinteger color
	string l/r/c

	ret: userdata
 */
static int
lnewlabel(lua_State *L) {
	struct pack_label label;
	label.width = (int)luaL_checkinteger(L,1);
	label.height = (int)luaL_checkinteger(L,2);
	label.size = (int)luaL_checkinteger(L,3);
	label.color = (uint32_t)luaL_optunsigned(L,4,0xffffffff);
	const char * align = lua_tostring(L,5);
	if (align == NULL) {
		label.align = LABEL_ALIGN_LEFT;
	} else {
		switch(align[0]) {
		case 'l':
		case 'L':
			label.align = LABEL_ALIGN_LEFT;
			break;
		case 'r':
		case 'R':
			label.align = LABEL_ALIGN_RIGHT;
			break;
		case 'c':
		case 'C':
			label.align = LABEL_ALIGN_CENTER;
			break;
		default:
			return luaL_error(L, "Align must be left/right/center");
		}
	}
	newlabel(L, &label);
	return 1;
}

static const char * srt_key[] = {
	"x",
	"y",
	"sx",
	"sy",
	"rot",
	"scale",
};

static struct sprite *
newsprite(lua_State *L, struct sprite_pack *pack, int id) {
	int sz = sprite_size(pack, id);
	if (sz == 0) {
		return NULL;
	}
	struct sprite * s = (struct sprite *)lua_newuserdata(L, sz);
	sprite_init(s, pack, id, sz);
	int i;
	for (i=0;;i++) {
		int id = sprite_component(s, i);
		if (id < 0)
			break;
		if (i==0) {
			lua_newtable(L);
			lua_pushvalue(L,-1);
			lua_setuservalue(L, -3);	// set uservalue for sprite
		}
		struct sprite *c = newsprite(L, pack, id);
		c->name = sprite_childname(s, i);
		sprite_mount(s, i, c);
		if (c) {
			lua_rawseti(L, -2, i+1);
		}
	}
	if (i>0) {
		lua_pop(L,1);
	}
	return s;
}

/*
	userdata sprite_pack
	integer id

	ret: userdata sprite
 */
static int
lnew(lua_State *L) {
	struct sprite_pack * pack = (struct sprite_pack *)lua_touserdata(L, 1);
	if (pack == NULL) {
		return luaL_error(L, "Need a sprite pack");
	}
	int id = (int)luaL_checkinteger(L, 2);
	struct sprite * s = newsprite(L, pack, id);
	if (s) {
		return 1;
	}
	return 0;
}

static double
readkey(lua_State *L, int idx, int key, double def) {
	lua_pushvalue(L, lua_upvalueindex(key));
	lua_rawget(L, idx);
	double ret = luaL_optnumber(L, -1, def);
	lua_pop(L,1);
	return ret;
}

static struct sprite *
self(lua_State *L) {
	struct sprite * s = (struct sprite *)lua_touserdata(L, 1);
	if (s == NULL) {
		luaL_error(L, "Need sprite");
	}
	return s;
}

static int
lgetframe(lua_State *L) {
	struct sprite * s = self(L);
	if (s->frame == -1)
		return 0;
	lua_pushinteger(L, s->frame);
	return 1;
}

static int
lsetframe(lua_State *L) {
	struct sprite * s = self(L);
	int frame = (int)luaL_checkinteger(L,2);
	sprite_setframe(s, frame);
	return 0;
}

static int
lsetaction(lua_State *L) {
	struct sprite * s = self(L);
	const char * name = lua_tostring(L,2);
	sprite_action(s, name);
	return 0;
}

static int
lgettotalframe(lua_State *L) {
	struct sprite *s = self(L);
	int f = s->total_frame;
	if (f<=0) {
		f = 0;
	}
	lua_pushinteger(L, f);
	return 1;
}

static int
lgetvisible(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushboolean(L, s->visible);
	return 1;
}

static int
lsetvisible(lua_State *L) {
	struct sprite *s = self(L);
	s->visible = lua_toboolean(L, 2);
	return 0;
}

static int
lgetmessage(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushboolean(L, s->message);
	return 1;
}

static int
lsetmessage(lua_State *L) {
	struct sprite *s = self(L);
	s->message = lua_toboolean(L, 2);
	return 0;
}

static int
lsetmat(lua_State *L) {
	struct sprite *s = self(L);
	struct matrix *m = (struct matrix *)lua_touserdata(L, 2);
	if (m == NULL)
		return luaL_error(L, "Need a matrix");
	s->t.mat = &s->mat;
	s->mat = *m;

	return 0;
}

static int
lsetprogram(lua_State *L) {
	struct sprite *s = self(L);
	if (lua_isnoneornil(L,2)) {
		s->t.program = PROGRAM_DEFAULT;
	} else {
		s->t.program = (int)luaL_checkinteger(L,2);
	}
	return 0;
}

static int
lsetscissor(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_PANNEL) {
		return luaL_error(L, "Only pannel can set scissor");
	}
	s->data.scissor = lua_toboolean(L,2);
	return 0;
}

static int
lgetname(lua_State *L) {
	struct sprite *s = self(L);
	if (s->name == NULL)
		return 0;
	lua_pushstring(L, s->name);
	return 1;
}

static int
lsettext(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "Only label can set text");
	}
	lua_settop(L,2);
	s->data.text = luaL_checkstring(L,2);
	lua_setuservalue(L, 1);
	return 0;
}

static int
lgettext(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "Only label can get text");
	}
	lua_settop(L,2);
	lua_getuservalue(L, 1);
	return 1;
}

static int
lgetcolor(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushunsigned(L, s->t.color);
	return 1;
}

static int
lsetcolor(lua_State *L) {
	struct sprite *s = self(L);
	uint32_t color = luaL_checkunsigned(L,2);
	s->t.color = color;
	return 0;
}

static int
lgetadditive(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushunsigned(L, s->t.additive);
	return 1;
}

static int
lsetadditive(lua_State *L) {
	struct sprite *s = self(L);
	uint32_t additive = luaL_checkunsigned(L,2);
	s->t.additive = additive;
	return 0;
}

static void
lgetter(lua_State *L) {
	luaL_Reg l[] = {
		{"frame", lgetframe},
		{"frame_count", lgettotalframe },
		{"visible", lgetvisible },
		{"name", lgetname },
		{"text", lgettext},
		{"color", lgetcolor },
		{"additive", lgetadditive },
		{"message", lgetmessage },
		{NULL, NULL},
	};
	luaL_newlib(L,l);
}

static void
lsetter(lua_State *L) {
	luaL_Reg l[] = {
		{"frame", lsetframe},
		{"action", lsetaction},
		{"visible", lsetvisible},
		{"matrix" , lsetmat},
		{"text", lsettext},
		{"color", lsetcolor},
		{"additive", lsetadditive },
		{"message", lsetmessage },
		{"program", lsetprogram },
		{"scissor", lsetscissor },
		{NULL, NULL},
	};
	luaL_newlib(L,l);
}

static int
lfetch(lua_State *L) {
	struct sprite *s = self(L);
	const char * name = luaL_checkstring(L,2);
	int index = sprite_child(s, name);
	if (index < 0)
		return 0;
	lua_getuservalue(L, 1);
	lua_rawgeti(L, -1, index+1);

	return 1;
}

static int
lmount(lua_State *L) {
	struct sprite *s = self(L);
	const char * name = luaL_checkstring(L,2);
	int index = sprite_child(s, name);
	if (index < 0) {
		return luaL_error(L, "No child name %s", name);
	}
	lua_getuservalue(L, 1);
	struct sprite * child = (struct sprite *)lua_touserdata(L, 3);
	if (child == NULL) {
		sprite_mount(s, index, NULL);
		lua_pushnil(L);
		lua_rawseti(L, -2, index+1);
	} else {
		sprite_mount(s, index, child);
		lua_pushvalue(L, 3);
		lua_rawseti(L, -2, index+1);
	}
	return 0;
}

static void
fill_srt(lua_State *L, struct srt *srt, int idx) {
	luaL_checktype(L,idx,LUA_TTABLE);
	double x = readkey(L, idx, SRT_X, 0);
	double y = readkey(L, idx, SRT_Y, 0);
	double scale = readkey(L, idx, SRT_SCALE, 0);
	double sx;
	double sy;
	double rot = readkey(L, idx, SRT_ROT, 0);
	if (scale > 0) {
		sx = sy = scale;
	} else {
		sx = readkey(L, idx, SRT_SX, 1);
		sy = readkey(L, idx, SRT_SY, 1);
	}
	srt->offx = x*SCREEN_SCALE;
	srt->offy = y*SCREEN_SCALE;
	srt->scalex = sx*1024;
	srt->scaley = sy*1024;
	srt->rot = rot * (1024.0 / 360.0);
}

/*
	userdata sprite
	table { .x .y .sx .sy .rot }
 */
static int
ldraw(lua_State *L) {
	struct sprite *s = self(L);
	struct srt srt;

	fill_srt(L,&srt,2);
	sprite_draw(s, &srt);
	return 0;
}

static int
lmulti_draw(lua_State *L) {
	struct sprite *s = self(L);
	int cnt = (int)luaL_checkinteger(L,3);
	if (cnt == 0)
		return 0;
	luaL_checktype(L,4,LUA_TTABLE);
	luaL_checktype(L,5,LUA_TTABLE);
	if (lua_rawlen(L, 4) < cnt) {
		return luaL_error(L, "matrix length less then particle count");
	}

	struct srt srt;
	fill_srt(L, &srt, 2);

	int i;
	for (i = 0; i < cnt; i++) {
		lua_rawgeti(L, 4, i+1);
		lua_rawgeti(L, 5, i+1);
		struct matrix * mat = (struct matrix *)lua_touserdata(L, -2);
		s->t.mat = mat;
		s->t.color = (uint32_t)lua_tounsigned(L, -1);
		lua_pop(L, 2);

		sprite_draw(s, &srt);
	}

	s->t.mat = NULL;

	return 0;
}

static int
lookup(lua_State *L, struct sprite *root, struct sprite *spr) {
	lua_checkstack(L,2);
	int i;
	lua_getuservalue(L,-1);
	for (i=0;sprite_component(root, i)>=0;i++) {
		struct sprite * child = root->data.children[i];
		if (child && child->name) {
			lua_rawgeti(L, -1, i+1);
			if (child == spr) {
				return 1;
			} else {
				if (lookup(L, child , spr))
					return 1;
				else {
					lua_pop(L,1);
				}
			}
		}
	}
	lua_pop(L,1);
	return 0;
}

static int
ltest(lua_State *L) {
	struct sprite * s = (struct sprite *)lua_touserdata(L, 1);
	if (s == NULL) {
		return luaL_error(L, "Need a sprite");
	}
	struct srt srt;
	fill_srt(L,&srt,2);
	float x = luaL_checknumber(L, 3);
	float y = luaL_checknumber(L, 4);
	struct sprite * m = sprite_test(s, &srt, x*SCREEN_SCALE, y*SCREEN_SCALE);
	if (m == NULL)
		return 0;
	if (m==s) {
		lua_settop(L,1);
		return 1;
	}
	lua_settop(L,1);
	if (!lookup(L, s, m)) {
		return luaL_error(L, "find an invlid sprite");
	}

	return 1;
}

static void
lmethod(lua_State *L) {
	luaL_Reg l[] = {
		{ "fetch", lfetch },
		{ "mount", lmount },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	int i;
	int nk = sizeof(srt_key)/sizeof(srt_key[0]);
	for (i=0;i<nk;i++) {
		lua_pushstring(L, srt_key[i]);
	}
	luaL_Reg l2[] = {
		{ "draw", ldraw },
		{ "multi_draw", lmulti_draw },
		{ "test", ltest },
		{ NULL, NULL, },
	};
	luaL_setfuncs(L,l2,nk);
}

int
ejoy2d_sprite(lua_State *L) {
	luaL_Reg l[] ={
		{ "new", lnew },
		{ "label", lnewlabel },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);

	lmethod(L);
	lua_setfield(L, -2, "method");
	lgetter(L);
	lua_setfield(L, -2, "get");
	lsetter(L);
	lua_setfield(L, -2, "set");

	return 1;
}
