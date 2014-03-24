#include "lmatrix.h"
#include "matrix.h"
#include "spritepack.h"

#include <lua.h>
#include <lauxlib.h>
#include <string.h>

static int
lnew(lua_State *L) {
	lua_settop(L,1);
	struct matrix *m = (struct matrix *)lua_newuserdata(L, sizeof(*m));
	int *mat = m->m;
	if (lua_istable(L,1) && lua_rawlen(L,1)==6) {
		int i;
		for (i=0;i<6;i++) {
			lua_rawgeti(L,1,i+1);
			mat[i] = (int)lua_tointeger(L,-1);
			lua_pop(L,1);
		}
	} else if (lua_isuserdata(L,1)) {
		// It's a matrix
		memcpy(mat, lua_touserdata(L,1), 6 * sizeof(int));
	} else {
		mat[0] = 1024;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = 1024;
		mat[4] = 0;
		mat[5] = 0;
	}
	return 1;
}

static int
lmul(lua_State *L) {
	struct matrix *m1 = (struct matrix *)lua_touserdata(L, 1);
	struct matrix *m2 = (struct matrix *)lua_touserdata(L, 2);
	struct matrix source = *m1;
	matrix_mul(m1, &source, m2);
	lua_settop(L,1);
	return 1;
}

static int
linverse(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);
	struct matrix source = *m;
	if (matrix_inverse(&source, m)) {
		return luaL_error(L, "Invalid matrix");
	}
	lua_settop(L,1);
	return 1;
}

static int
ltrans(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);
	double x = luaL_checknumber(L,2);
	double y = luaL_checknumber(L,3);
	m->m[4] += x * SCREEN_SCALE;
	m->m[5] += y * SCREEN_SCALE;

	lua_settop(L,1);
	return 1;
}

static int
lscale(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);
	double sx = luaL_checknumber(L,2);
	double sy = luaL_optnumber(L,3,sx);
	matrix_scale(m, sx * 1024, sy * 1024);
	
	lua_settop(L,1);
	return 1;
}

static int
lidentity(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L,1);
	if (m == NULL) {
		return luaL_error(L, "Need a matrix");
	}
	int n = lua_gettop(L);
	int *mat = m->m;
	int scale=1024;
	int x=0,y=0;
	switch(n) {
	case 4:
		scale = luaL_checknumber(L,4) * 1024;
		// x,y,scale
		// go though
	case 3:
		// x,y
		x = luaL_checknumber(L,2) * SCREEN_SCALE;
		y = luaL_checknumber(L,3) * SCREEN_SCALE;
		break;
	case 2:
		// scale
		scale = luaL_checknumber(L,2) * 1024;
		break;
	case 1:
		break;
	default:
		return luaL_error(L, "Invalid parameter");
	}
	mat[0] = scale;
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = scale;
	mat[4] = x;
	mat[5] = y;
	lua_settop(L,1);
	return 1;
}

static int
lrot(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);
	double r = luaL_checknumber(L,2);
	matrix_rot(m, r * (1024.0 / 360.0));

	lua_settop(L,1);
	return 1;
}

static int
lsr(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);

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
lrs(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);

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
	matrix_rs(m, sx, sy, r);

	return 0;
}

static int
ltostring(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	lua_pushfstring(L, "Mat(%d,%d,%d,%d,%d,%d)",
		m[0],m[1],m[2],m[3],m[4],m[5]);
	return 1;
}

static int
lexport(lua_State *L) {
	int i;
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	for (i=0;i<6;i++) {
		lua_pushinteger(L, m[i]);
	}
	return 6;
}

static int
limport(lua_State *L) {
	int i;
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	for (i=0;i<6;i++) {
		m[i] = (int)luaL_checkinteger(L,i+2);
	}
	return 0;
}

int 
ejoy2d_matrix(lua_State *L) {
	luaL_Reg l[] = {
		{ "new", lnew },
		{ "scale", lscale },
		{ "trans", ltrans },
		{ "rot", lrot },
		{ "sr", lsr },
		{ "rs", lrs },
		{ "inverse", linverse },
		{ "mul", lmul },
		{ "tostring", ltostring },
		{ "identity", lidentity},
		{ "export", lexport },
		{ "import", limport },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
