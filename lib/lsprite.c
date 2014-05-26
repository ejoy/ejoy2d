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
	s->parent = NULL;
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
    label.space_w = 0;
    label.space_h = 0;
    label.auto_scale = 0;
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

static int
lgenoutline(lua_State *L) {
  label_gen_outline(lua_toboolean(L, 1));
  return 0;
}

static const char * srt_key[] = {
	"x",
	"y",
	"sx",
	"sy",
	"rot",
	"scale",
};


static void
update_message(struct sprite * s, struct sprite_pack * pack, int parentid, int componentid, int frame) {
	struct pack_animation * ani = (struct pack_animation *)pack->data[parentid];
	if (frame < 0 || frame >= ani->frame_number) {
		return;
	}
	struct pack_frame pframe = ani->frame[frame];
	int i = 0;
	for (; i < pframe.n; i++) {
		if (pframe.part[i].component_id == componentid && pframe.part[i].touchable) {
			s->message = true;
			return;
		}
	}
}

static struct sprite *
newanchor(lua_State *L) {
	int sz = sizeof(struct sprite) + sizeof(struct matrix);
	struct sprite * s = (struct sprite *)lua_newuserdata(L, sz);
	s->parent = NULL;
	s->t.mat = NULL;
	s->t.color = 0xffffffff;
	s->t.additive = 0;
	s->t.program = PROGRAM_DEFAULT;
	s->message = false;
	s->visible = false;	// anchor is invisible by default
	s->name = NULL;
	s->id = ANCHOR_ID;
	s->type = TYPE_ANCHOR;
	s->s.mat = (struct matrix *)(s+1);
	matrix_identity(s->s.mat);

	return s;
}

static struct sprite *
newsprite(lua_State *L, struct sprite_pack *pack, int id) {
	if (id == ANCHOR_ID) {
		return newanchor(L);
	}
	int sz = sprite_size(pack, id);
	if (sz == 0) {
		return NULL;
	}
	struct sprite * s = (struct sprite *)lua_newuserdata(L, sz);
	sprite_init(s, pack, id, sz);
	int i;
	for (i=0;;i++) {
		int childid = sprite_component(s, i);
		if (childid < 0)
			break;
		if (i==0) {
			lua_newtable(L);
			lua_pushvalue(L,-1);
			lua_setuservalue(L, -3);	// set uservalue for sprite
		}
		struct sprite *c = newsprite(L, pack, childid);
		c->name = sprite_childname(s, i);
		sprite_mount(s, i, c);
		update_message(c, pack, id, i, s->frame);
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
	lua_pushinteger(L, s->frame);
	return 1;
}

static int
lsetframe(lua_State *L) {
	struct sprite * s = self(L);
	int frame = (int)luaL_checkinteger(L,2);
	sprite_setframe(s, frame, false);
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
lgetmat(lua_State *L) {
	struct sprite *s = self(L);
	if (s->t.mat == NULL) {
		s->t.mat = &s->mat;
		matrix_identity(&s->mat);
	}
	lua_pushlightuserdata(L, s->t.mat);
	return 1;
}

static int
lgetwmat(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type == TYPE_ANCHOR) {
		lua_pushlightuserdata(L, s->s.mat);
		return 1;
	}
	return luaL_error(L, "Only anchor can get world matrix");
}
static int
lgetwpos(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type == TYPE_ANCHOR) {
		struct matrix* mat = s->s.mat;
		lua_pushnumber(L,mat->m[4] /(float)SCREEN_SCALE);
		lua_pushnumber(L,mat->m[5] /(float)SCREEN_SCALE);
		return 2;
	}
	return luaL_error(L, "Only anchor can get world matrix");
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
lgettype(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushinteger(L, s->type);
	return 1;
}

static int
lgetparentname(lua_State *L) {
	struct sprite *s = self(L);
	if (s->parent == NULL)
		return 0;
	lua_pushstring(L, s->parent->name);
	return 1;
}

static int
lhasparent(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushboolean(L, s->parent != NULL);
	return 1;
}

static int
lsettext(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "Only label can set text");
	}
	lua_settop(L,2);
	if (lua_isnil(L,2)) {
		s->data.text = NULL;
		lua_setuservalue(L,1);
		return 0;
	}
	s->data.text = luaL_checkstring(L,2);
	lua_createtable(L,1,0);
	lua_pushvalue(L, -2);
	lua_rawseti(L, -2, 1);
	lua_setuservalue(L, 1);
	return 0;
}

static int
lgettext(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "Only label can get text");
	}
	lua_getuservalue(L, 1);
	if (!lua_istable(L,-1)) {
		return 0;
	}
	lua_rawgeti(L, -1, 1);
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
lsetalpha(lua_State *L) {
	struct sprite *s = self(L);
	uint8_t alpha = luaL_checkunsigned(L, 2);
	s->t.color = (s->t.color >> 8) | (alpha << 24);
	return 0;
}

static int
lgetalpha(lua_State *L) {
	struct sprite *s = self(L);
	lua_pushunsigned(L, s->t.color & 0x000000FF);
	return 1;
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
		{"type", lgettype },
		{"text", lgettext},
		{"color", lgetcolor },
		{"alpha", lgetalpha },
		{"additive", lgetadditive },
		{"message", lgetmessage },
		{"matrix", lgetmat },
		{"world_matrix", lgetwmat },
		{"parent_name", lgetparentname },
		{"has_parent", lhasparent },
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
		{"alpha", lsetalpha},
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
		if (child->parent) {
			return luaL_error(L, "Can't mount sprite %p twice,pre parent:%p: %s", child,child->parent,child->name);
		}
		sprite_mount(s, index, child);
		lua_pushvalue(L, 3);
		lua_rawseti(L, -2, index+1);
	}
	return 0;
}

static void
fill_srt(lua_State *L, struct srt *srt, int idx) {
	if (lua_isnoneornil(L, idx)) {
		srt->offx = 0;
		srt->offy = 0;
		srt->rot = 0;
		srt->scalex = 1024;
		srt->scaley = 1024;
		return;
	}
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
laabb(lua_State *L) {
	struct sprite *s = self(L);
	struct srt srt;
	fill_srt(L,&srt,2);
	int aabb[4];
	sprite_aabb(s, &srt, aabb);
	int i;
	for (i=0;i<4;i++) {
		lua_pushinteger(L, aabb[i]);
	}
	return 4;
}

static int
ltext_size(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_LABEL) {
		return luaL_error(L, "Ony label can get label_size");
	}
	int width = 0, height = 0;
	label_size(s->data.text, s->s.label, &width, &height);
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}


static int
lchild_visible(lua_State *L) {
	struct sprite *s = self(L);
	const char * name = luaL_checkstring(L,2);
	lua_pushboolean(L, sprite_child_visible(s, name));
	return 1;
}

static int
lchildren_name(lua_State *L) {
	struct sprite *s = self(L);
	if (s->type != TYPE_ANIMATION)
		return 0;
	int i;
	int cnt=0;
	struct pack_animation * ani = s->s.ani;
	for (i=0;i<ani->component_number;i++) {
		if (ani->component[i].name != NULL) {
			lua_pushstring(L, ani->component[i].name);
			cnt++;
		}
	}
	return cnt;
}

static int
lmatrix_multi_draw(lua_State *L) {
	struct sprite *s = self(L);
	int cnt = (int)luaL_checkinteger(L,3);
	if (cnt == 0)
		return 0;
	luaL_checktype(L,4,LUA_TTABLE);
	luaL_checktype(L,5,LUA_TTABLE);
//    luaL_checktype(L,6,LUA_TTABLE);
	if (lua_rawlen(L, 4) < cnt) {
		return luaL_error(L, "matrix length less then particle count");
	}

	struct matrix *mat = (struct matrix *)lua_touserdata(L, 2);

	if (s->t.mat == NULL) {
		s->t.mat = &s->mat;
		matrix_identity(&s->mat);
	}
	struct matrix *parent_mat = s->t.mat;
	uint32_t parent_color = s->t.color;

	int i;
	if (mat) {
		struct matrix tmp;
		for (i = 0; i < cnt; i++) {
			lua_rawgeti(L, 4, i+1);
			lua_rawgeti(L, 5, i+1);
			struct matrix *m = (struct matrix *)lua_touserdata(L, -2);
			matrix_mul(&tmp, m, mat);
			s->t.mat = &tmp;
			s->t.color = (uint32_t)lua_tounsigned(L, -1);
			lua_pop(L, 2);

			sprite_draw(s, NULL);
		}
	} else {
		for (i = 0; i < cnt; i++) {
			lua_rawgeti(L, 4, i+1);
			lua_rawgeti(L, 5, i+1);
			struct matrix *m = (struct matrix *)lua_touserdata(L, -2);
			s->t.mat = m;
			s->t.color = (uint32_t)lua_tounsigned(L, -1);
			lua_pop(L, 2);

			sprite_draw(s, NULL);
		}
	}

	s->t.mat = parent_mat;
	s->t.color = parent_color;

	return 0;
}

static int
lmulti_draw(lua_State *L) {
	struct sprite *s = self(L);
	int cnt = (int)luaL_checkinteger(L,3);
	if (cnt == 0)
		return 0;
    int n = lua_gettop(L);
	luaL_checktype(L,4,LUA_TTABLE);
	luaL_checktype(L,5,LUA_TTABLE);
	if (lua_rawlen(L, 4) < cnt) {
		return luaL_error(L, "matrix length less then particle count");
	}
    if (n == 6) {
        luaL_checktype(L,6,LUA_TTABLE);
        if (lua_rawlen(L, 6) < cnt) {
            return luaL_error(L, "additive length less then particle count");
        }
    }
	struct srt srt;
	fill_srt(L, &srt, 2);

	if (s->t.mat == NULL) {
		s->t.mat = &s->mat;
		matrix_identity(&s->mat);
	}
	struct matrix *parent_mat = s->t.mat;
	uint32_t parent_color = s->t.color;

	int i;
    if (n == 5) {
        for (i = 0; i < cnt; i++) {
            lua_rawgeti(L, 4, i+1);
            lua_rawgeti(L, 5, i+1);
            s->t.mat = (struct matrix *)lua_touserdata(L, -2);
            s->t.color = (uint32_t)lua_tounsigned(L, -1);
            lua_pop(L, 2);

            sprite_draw_as_child(s, &srt, parent_mat, parent_color);
        }
    }else {
        for (i = 0; i < cnt; i++) {
            lua_rawgeti(L, 4, i+1);
            lua_rawgeti(L, 5, i+1);
            lua_rawgeti(L, 6, i+1);
            s->t.mat = (struct matrix *)lua_touserdata(L, -3);
            s->t.color = (uint32_t)lua_tounsigned(L, -2);
            s->t.additive = (uint32_t)lua_tounsigned(L, -1);
            lua_pop(L, 3);

            sprite_draw_as_child(s, &srt, parent_mat, parent_color);
        }
    }

	s->t.mat = parent_mat;
	s->t.color = parent_color;

	return 0;
}

static struct sprite *
lookup(lua_State *L, struct sprite * spr) {
	int i;
	struct sprite * root = (struct sprite *)lua_touserdata(L, -1);
	lua_getuservalue(L,-1);
	for (i=0;sprite_component(root, i)>=0;i++) {
		struct sprite * child = root->data.children[i];
		if (child) {
			lua_rawgeti(L, -1, i+1);
			if (child == spr) {
				lua_replace(L,-2);
				return child;
			} else {
				lua_pop(L,1);
			}
		}
	}
	lua_pop(L,1);
	return NULL;
}

static int
unwind(lua_State *L, struct sprite *root, struct sprite *spr) {
	int n = 0;
	while (spr) {
		if (spr == root) {
			return n;
		}
		++n;
		lua_checkstack(L,3);
		lua_pushlightuserdata(L, spr);
		spr = spr->parent;
	}
	return -1;
}

static int
ltest(lua_State *L) {
	struct sprite *s = self(L);
	struct srt srt;
	fill_srt(L,&srt,4);
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	struct sprite * m = sprite_test(s, &srt, x*SCREEN_SCALE, y*SCREEN_SCALE);
	if (m == NULL)
		return 0;
	if (m==s) {
		lua_settop(L,1);
		return 1;
	}
	lua_settop(L,1);
	int depth = unwind(L, s , m);
	if (depth < 0) {
		return luaL_error(L, "Unwind an invalid sprite");
	}
	int i;
	lua_pushvalue(L,1);
	for (i=depth+1;i>1;i--) {
		struct sprite * tmp = (struct sprite *)lua_touserdata(L, i);
		tmp = lookup(L, tmp);
		if (tmp == NULL) {
			return luaL_error(L, "find an invalid sprite");
		}
		lua_replace(L, -2);
	}

	return 1;
}

static int
lps(lua_State *L) {
	struct sprite *s = self(L);
	struct matrix *m = &s->mat;
	if (s->t.mat == NULL) {
		matrix_identity(m);
		s->t.mat = m;
	}
	int *mat = m->m;
	int n = lua_gettop(L);
	int x,y,scale;
	switch (n) {
	case 4:
		// x,y,scale
		x = luaL_checknumber(L,2) * SCREEN_SCALE;
		y = luaL_checknumber(L,3) * SCREEN_SCALE;
		scale = luaL_checknumber(L,4) * 1024;
		mat[0] = scale;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = scale;
		mat[4] = x;
		mat[5] = y;
		break;
	case 3:
		// x,y
		x = luaL_checknumber(L,2) * SCREEN_SCALE;
		y = luaL_checknumber(L,3) * SCREEN_SCALE;
		mat[4] = x;
		mat[5] = y;
		break;
	case 2:
		// scale
		scale = luaL_checknumber(L,2) * 1024;
		mat[0] = scale;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = scale;
		break;
	default:
		return luaL_error(L, "Invalid parm");
	}
	return 0;
}

static int
lsr(lua_State *L) {
	struct sprite *s = self(L);
	struct matrix *m = &s->mat;
	if (s->t.mat == NULL) {
		matrix_identity(m);
		s->t.mat = m;
	}
	int sx=1024,sy=1024,r=0;
	int n = lua_gettop(L);
	switch (n) {
	case 4:
		// sx,sy,rot
		r = luaL_checknumber(L,4) * (1024.0 / 360.0);
		// go through
	case 3:
		// sx, sy
		sx = luaL_checknumber(L,2) * 1024;
		sy = luaL_checknumber(L,3) * 1024;
		break;
	case 2:
		// rot
		r = luaL_checknumber(L,2) * (1024.0 / 360.0);
		break;
	}
	matrix_sr(m, sx, sy, r);

	return 0;
}

static int
lrecursion_frame(lua_State *L) {
	struct sprite * s = self(L);
	int frame = (int)luaL_checkinteger(L,2);
	int f = sprite_setframe(s, frame, true);
	lua_pushinteger(L, f);
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
		{ "ps", lps },
		{ "sr", lsr },
		{ "draw", ldraw },
		{ "recursion_frame", lrecursion_frame },
		{ "multi_draw", lmulti_draw },
		{ "matrix_multi_draw", lmatrix_multi_draw },
		{ "test", ltest },
		{ "aabb", laabb },
		{ "text_size", ltext_size},
		{ "child_visible", lchild_visible },
		{ "children_name", lchildren_name },
		{ "world_pos", lgetwpos },
		{ NULL, NULL, },
	};
	luaL_setfuncs(L,l2,nk);
}

int
ejoy2d_sprite(lua_State *L) {
	luaL_Reg l[] ={
		{ "new", lnew },
		{ "label", lnewlabel },
		{ "label_gen_outline", lgenoutline },
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
