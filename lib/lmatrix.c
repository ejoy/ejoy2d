#include "lmatrix.h"
#include "matrix.h"

#include <lua.h>
#include <lauxlib.h>

static int
lnew(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_newuserdata(L, sizeof(*m));
	int *mat = m->m;
	if (lua_istable(L,1) && lua_rawlen(L,1)==6) {
		int i;
		for (i=0;i<6;i++) {
			lua_rawgeti(L,1,i+1);
			mat[i] = (int)lua_tointeger(L,-1);
			lua_pop(L,1);
		}
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
	matrix_inverse(&source, m);
	lua_settop(L,1);
	return 1;
}

static int
ltrans(lua_State *L) {
	struct matrix *m = (struct matrix *)lua_touserdata(L, 1);
	double x = luaL_checknumber(L,2);
	double y = luaL_checknumber(L,3);
	m->m[4] += x * 8;
	m->m[5] += y * 8;

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
    int *mat = m->m;
    mat[0] = 1024;
    mat[1] = 0;
    mat[2] = 0;
    mat[3] = 1024;
    mat[4] = 0;
    mat[5] = 0;
	
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
ltostring(lua_State *L) {
	struct matrix *mat = (struct matrix *)lua_touserdata(L, 1);
	int *m = mat->m;
	lua_pushfstring(L, "Mat(%d,%d,%d,%d,%d,%d)",
		m[0],m[1],m[2],m[3],m[4],m[5]);
	return 1;
}

int 
ejoy2d_matrix(lua_State *L) {
	luaL_Reg l[] = {
		{ "new", lnew },
		{ "scale", lscale },
		{ "trans", ltrans },
		{ "rot", lrot },
		{ "inverse", linverse },
		{ "mul", lmul },
		{ "tostring", ltostring },
        {"identity", lidentity},
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
