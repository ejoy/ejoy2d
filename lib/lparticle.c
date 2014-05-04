
#include "particle.h"
#include "spritepack.h"

#include <lua.h>
#include <lauxlib.h>
#include <string.h>


static float
dict_float(lua_State *L, const char *key) {
	lua_getfield(L, -1, key);
	double v = lua_tonumber(L, -1);
	lua_pop(L,1);
	return (float)v;
}

static int
dict_int(lua_State *L, const char *key) {
	lua_getfield(L, -1, key);
	int v = (int)lua_tointeger(L, -1);
	lua_pop(L,1);
	return v;
}

static uint32_t
color4f(struct color4f *c4f) {
	uint8_t rr = (int)(c4f->r*255);
	uint8_t gg = (int)(c4f->g*255);
	uint8_t bb = (int)(c4f->b*255);
	uint8_t aa = (int)(c4f->a*255);
	return (uint32_t)aa << 24 | (uint32_t)rr << 16 | (uint32_t)gg << 8 | bb;
}

static int
_init_from_table(struct particle_config *ps, struct lua_State *L) {
	ps->angle = -dict_float(L, "angle");
	ps->angleVar = -dict_float(L, "angleVariance");
	ps->duration = dict_float(L, "duration");

	// avoid defined blend func
	// int blendfunc_src = dict_float(L, "blendFuncSource");
	// int blendfunc_dst = dict_float(L, "blendFuncDestination");

	ps->startColor.r = dict_float(L, "startColorRed");
	ps->startColor.g = dict_float(L, "startColorGreen");
	ps->startColor.b = dict_float(L, "startColorBlue");
	ps->startColor.a = dict_float(L, "startColorAlpha");
	ps->startColorVar.r = dict_float(L, "startColorVarianceRed");
	ps->startColorVar.g = dict_float(L, "startColorVarianceGreen");
	ps->startColorVar.b = dict_float(L, "startColorVarianceBlue");
	ps->startColorVar.a = dict_float(L, "startColorVarianceAlpha");
	ps->endColor.r = dict_float(L, "finishColorRed");
	ps->endColor.g = dict_float(L, "finishColorGreen");
	ps->endColor.b = dict_float(L, "finishColorBlue");
	ps->endColor.a = dict_float(L, "finishColorAlpha");
	ps->endColorVar.r = dict_float(L, "finishColorVarianceRed");
	ps->endColorVar.g = dict_float(L, "finishColorVarianceGreen");
	ps->endColorVar.b = dict_float(L, "finishColorVarianceBlue");
	ps->endColorVar.a = dict_float(L, "finishColorVarianceAlpha");

	ps->startSize = dict_float(L, "startParticleSize");
	ps->startSizeVar = dict_float(L, "startParticleSizeVariance");
	ps->endSize = dict_float(L, "finishParticleSize");
	ps->endSizeVar = dict_float(L, "finishParticleSizeVariance");

	ps->sourcePosition.x = dict_float(L, "sourcePositionx");
	ps->sourcePosition.y = dict_float(L, "sourcePositiony");

	ps->posVar.x = dict_float(L, "sourcePositionVariancex");
	ps->posVar.y = dict_float(L, "sourcePositionVariancey");

	ps->startSpin = dict_float(L, "rotationStart");
	ps->startSpinVar = dict_float(L, "rotationStartVariance");
	ps->endSpin = dict_float(L, "rotationEnd");
	ps->endSpinVar = dict_float(L, "rotationEndVariance");

	ps->emitterMode = dict_int(L, "emitterType");

	// Mode A: Gravity + tangential accel + radial accel
	if (ps->emitterMode == PARTICLE_MODE_GRAVITY) {
		ps->mode.A.gravity.x = dict_float(L, "gravityx");
		ps->mode.A.gravity.y = -dict_float(L, "gravityy");

		ps->mode.A.speed = dict_float(L, "speed");
		ps->mode.A.speedVar = dict_float(L, "speedVariance");

		ps->mode.A.radialAccel = dict_float(L, "radialAcceleration");
		ps->mode.A.radialAccelVar = dict_float(L, "radialAccelVariance");

		ps->mode.A.tangentialAccel = dict_float(L, "tangentialAcceleration");
		ps->mode.A.tangentialAccelVar = dict_float(L, "tangentialAccelVariance");

		ps->mode.A.rotationIsDir = dict_int(L, "rotationIsDir");
	}
	// or Mode B: radius movement
	else if (ps->emitterMode == PARTICLE_MODE_RADIUS) {
		ps->mode.B.startRadius = dict_float(L, "maxRadius");
		ps->mode.B.startRadiusVar = dict_float(L, "maxRadiusVariance");
		ps->mode.B.endRadius = dict_float(L, "minRadius");
		ps->mode.B.endRadiusVar = dict_float(L, "minRadiusVariance");
		ps->mode.B.rotatePerSecond = dict_float(L, "rotatePerSecond");
		ps->mode.B.rotatePerSecondVar = dict_float(L, "rotatePerSecondVariance");
	} else {
		return 0;
	}
	ps->positionType = dict_int(L, "positionType");
	ps->life = dict_float(L, "particleLifespan");
	ps->lifeVar = dict_float(L, "particleLifespanVariance");
	ps->emissionRate = ps->life / ps->totalParticles;

	return 1;
}

static struct particle_system *
_new(struct lua_State *L) {
	int maxParticles = dict_int(L,"maxParticles");
	int totalsize = sizeof(struct particle_system) + maxParticles * (sizeof(struct particle) + sizeof(struct matrix)) + sizeof(struct particle_config);
	struct particle_system * ps = (struct particle_system *)lua_newuserdata(L, totalsize);
	lua_insert(L, -2);
	memset(ps, 0, totalsize);
	init_with_particles(ps, maxParticles);
	if (!_init_from_table(ps->config, L)) {
		lua_pop(L,2);
		return NULL;
	}
	lua_pop(L,1);
	return ps;
}

static int
lnew(lua_State *L) {
	luaL_checktype(L,1,LUA_TTABLE);
	struct particle_system * ps = _new(L);
	if (ps == NULL)
		return 0;
	return 1;
}

static int
lreset(lua_State *L) {
	luaL_checktype(L,1,LUA_TUSERDATA);
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	particle_system_reset(ps);

	return 1;
}

static int
lupdate(lua_State *L) {
	luaL_checktype(L,1,LUA_TUSERDATA);
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	float dt = luaL_checknumber(L,2);

/*	if (ps->config->positionType == POSITION_TYPE_GROUPED) {
		struct matrix *m = (struct matrix *)lua_touserdata(L, 3);
		ps->config->sourcePosition.x = m->m[4] / SCREEN_SCALE;
		ps->config->sourcePosition.y = m->m[5] / SCREEN_SCALE;
	} else {
		ps->config->sourcePosition.x = 0;
		ps->config->sourcePosition.y = 0;
	}*/

	if (ps->config->positionType == POSITION_TYPE_GROUPED) {
		ps->config->emitterMatrix = (struct matrix*)lua_touserdata(L, 3);
	} else {
		ps->config->emitterMatrix = NULL;
	}
	ps->config->sourcePosition.x = 0;
	ps->config->sourcePosition.y = 0;
	particle_system_update(ps, dt);

	lua_pushboolean(L, ps->isActive || ps->isAlive);
	return 1;
}

static int
ldata(lua_State *L) {
	luaL_checktype(L,1,LUA_TUSERDATA);
	luaL_checktype(L,2,LUA_TTABLE);
	luaL_checktype(L,3,LUA_TTABLE);
	struct particle_system *ps = (struct particle_system *)lua_touserdata(L, 1);
	int edge = (int)luaL_checkinteger(L,4);
	int n = ps->particleCount;
	int i;
	for (i=0;i<n;i++) {
		struct particle *p = &ps->particles[i];
		calc_particle_system_mat(p,&ps->matrix[i], edge);

		lua_pushlightuserdata(L, &ps->matrix[i]);
		lua_rawseti(L, 2, i+1);

		uint32_t c = color4f(&p->color);
		lua_pushunsigned(L, c);
		lua_rawseti(L, 3, i+1);
	}

	lua_pushinteger(L, n);
	return 1;
}

int
ejoy2d_particle(lua_State *L) {
	luaL_Reg l[] = {
		{ "new", lnew },
		{ "reset", lreset },
		{ "update", lupdate },
		{ "data", ldata },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);

	return 1;
}

