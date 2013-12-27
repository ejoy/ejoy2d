#include "particle.h"
#include "matrix.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

#include <math.h>
#include <string.h>

/** @def CC_DEGREES_TO_RADIANS
 converts degrees to radians
 */
#define CC_DEGREES_TO_RADIANS(__ANGLE__) ((__ANGLE__) * 0.01745329252f) // PI / 180

/** @def CC_RADIANS_TO_DEGREES
 converts radians to degrees
 */
#define CC_RADIANS_TO_DEGREES(__ANGLE__) ((__ANGLE__) * 57.29577951f) // PI * 180

struct point {
	float x;
	float y;
};

struct color4f {
	float r;
	float g;
	float b;
	float a;
};

struct particle {
	struct point pos;
	struct point startPos;

	struct color4f color;
	struct color4f deltaColor;

	float size;
	float deltaSize;

	float rotation;
	float deltaRotation;

	float timeToLive;

	union {
		//! Mode A: gravity, direction, radial accel, tangential accel
		struct {
			struct point dir;
			float radialAccel;
			float tangentialAccel;
		} A;

		//! Mode B: radius mode
		struct {
			float angle;
			float degreesPerSecond;
			float radius;
			float deltaRadius;
		} B;
	} mode;
};

#define PARTICLE_MODE_GRAVITY 0
#define PARTICLE_MODE_RADIUS 1

#define POSITION_TYPE_FREE 0
#define POSITION_TYPE_RELATIVE 1
#define POSTTION_TYPE_GROUPED 2

/** The Particle emitter lives forever */
#define DURATION_INFINITY (-1)

/** The starting size of the particle is equal to the ending size */
#define START_SIZE_EQUAL_TO_END_SIZE (-1)

/** The starting radius of the particle is equal to the ending radius */
#define START_RADIUS_EQUAL_TO_END_RADIUS (-1)


struct particle_system {
	//! time elapsed since the start of the system (in seconds)
	float elapsed;

	/** Switch between different kind of emitter modes:
	 - kParticleModeGravity: uses gravity, speed, radial and tangential acceleration
	 - kParticleModeRadius: uses radius movement + rotation
	 */
	int emitterMode;

	union {
		// Different modes
		//! Mode A:Gravity + Tangential Accel + Radial Accel
		struct {
			/** Gravity value. Only available in 'Gravity' mode. */
			struct point gravity;
			/** speed of each particle. Only available in 'Gravity' mode.  */
			float speed;
			/** speed variance of each particle. Only available in 'Gravity' mode. */
			float speedVar;
			/** tangential acceleration of each particle. Only available in 'Gravity' mode. */
			float tangentialAccel;
			/** tangential acceleration variance of each particle. Only available in 'Gravity' mode. */
			float tangentialAccelVar;
			/** radial acceleration of each particle. Only available in 'Gravity' mode. */
			float radialAccel;
			/** radial acceleration variance of each particle. Only available in 'Gravity' mode. */
			float radialAccelVar;
			/** set the rotation of each particle to its direction Only available in 'Gravity' mode. */
			bool rotationIsDir;
		} A;

		//! Mode B: circular movement (gravity, radial accel and tangential accel don't are not used in this mode)
		struct {
			/** The starting radius of the particles. Only available in 'Radius' mode. */
			float startRadius;
			/** The starting radius variance of the particles. Only available in 'Radius' mode. */
			float startRadiusVar;
			/** The ending radius of the particles. Only available in 'Radius' mode. */
			float endRadius;
			/** The ending radius variance of the particles. Only available in 'Radius' mode. */
			float endRadiusVar;
			/** Number of degrees to rotate a particle around the source pos per second. Only available in 'Radius' mode. */
			float rotatePerSecond;
			/** Variance in degrees for rotatePerSecond. Only available in 'Radius' mode. */
			float rotatePerSecondVar;
		} B;
	} mode;

	//! Array of particles
	struct particle *particles;
	struct matrix *matrix;

	//Emitter name
//    std::string _configName;

	// color modulate
	//    BOOL colorModulate;

	//! How many particles can be emitted per second
	float emitCounter;

	//!  particle idx
	int particleIdx;

	// Number of allocated particles
	int allocatedParticles;

	/** Is the emitter active */
	bool isActive;

	/** Quantity of particles that are being simulated at the moment */
	int particleCount;
	/** How many seconds the emitter will run. -1 means 'forever' */
	float duration;
	/** sourcePosition of the emitter */
	struct point sourcePosition;
	/** Position variance of the emitter */
	struct point posVar;
	/** life, and life variation of each particle */
	float life;
	/** life variance of each particle */
	float lifeVar;
	/** angle and angle variation of each particle */
	float angle;
	/** angle variance of each particle */
	float angleVar;

	/** start size in pixels of each particle */
	float startSize;
	/** size variance in pixels of each particle */
	float startSizeVar;
	/** end size in pixels of each particle */
	float endSize;
	/** end size variance in pixels of each particle */
	float endSizeVar;
	/** start color of each particle */
	struct color4f startColor;
	/** start color variance of each particle */
	struct color4f startColorVar;
	/** end color and end color variation of each particle */
	struct color4f endColor;
	/** end color variance of each particle */
	struct color4f endColorVar;
	//* initial angle of each particle
	float startSpin;
	//* initial angle of each particle
	float startSpinVar;
	//* initial angle of each particle
	float endSpin;
	//* initial angle of each particle
	float endSpinVar;
	/** emission rate of the particles */
	float emissionRate;
	/** maximum particles of the system */
	int totalParticles;
	/** conforms to CocosNodeTexture protocol */
//	Texture2D* _texture;
	/** conforms to CocosNodeTexture protocol */
//	BlendFunc _blendFunc;
	/** does the alpha value modify color */
	bool opacityModifyRGB;
	/** does FlippedY variance of each particle */
//	int yCoordFlipped;

	/** particles movement type: Free or Grouped
	 @since v0.8
	 */
	int positionType;
};

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
	int v = lua_tointeger(L, -1);
	lua_pop(L,1);
	return v;
}

static int
_init_from_table(struct particle_system *ps, struct lua_State *L) {
	ps->angle = dict_float(L, "angle");
	ps->angleVar = dict_float(L, "angleVariance");
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

	ps->startSize = dict_float(L, "startParticleSize") / 32.0;
	ps->startSizeVar = dict_float(L, "startParticleSizeVariance") / 100.0;
	ps->endSize = dict_float(L, "finishParticleSize") / 32.0;
	ps->endSizeVar = dict_float(L, "finishParticleSizeVariance") / 100.0;

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
		ps->mode.A.gravity.x = dict_float(L, "gravityx") / 100;
		ps->mode.A.gravity.y = dict_float(L, "gravityy") / 100;

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
		ps->mode.B.endRadiusVar = 0.0f;
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

static void
_init_with_particles(struct particle_system *ps, int numberOfParticles) {
	ps->totalParticles = numberOfParticles;
	ps->particles = (struct particle *)(ps+1);
	ps->matrix = (struct matrix *)(ps->particles + numberOfParticles);
	ps->allocatedParticles = numberOfParticles;
	ps->isActive = true;
	ps->positionType = POSITION_TYPE_FREE;
	ps->emitterMode = PARTICLE_MODE_GRAVITY;
}

static struct particle_system *
_new(struct lua_State *L) {
	int maxParticles = dict_int(L,"maxParticles");
	int totalsize = maxParticles * (sizeof(struct particle) + sizeof(struct matrix));
	struct particle_system * ps = lua_newuserdata(L, totalsize);
	lua_insert(L, -2);
	memset(ps, 0, totalsize);
	_init_with_particles(ps, maxParticles);
	if (!_init_from_table(ps, L)) {
		lua_pop(L,2);
		return NULL;
	}
	lua_pop(L,1);
	return ps;
}

static inline float
clampf(float x) {
	if (x < 0)
		return 0;
	if (x > 1.0f)
		return 1.0f;
	return x;
}

inline static float
RANDOM_M11(unsigned int *seed) {
	*seed = (*seed * 134775813 + 1) % 0x7fff;
	union {
		uint32_t d;
		float f;
	} u;
	u.d = (((uint32_t)rand() & 0x7fff) << 8) | 0x40000000;
	return u.f - 3.0f;
}

static void
_initParticle(struct particle_system *ps, struct particle* particle) {
	uint32_t RANDSEED = rand();

	particle->timeToLive = ps->life + ps->lifeVar * RANDOM_M11(&RANDSEED);
	if (particle->timeToLive <= 0) {
		return;
	}

	particle->startPos = ps->sourcePosition;
	particle->pos.x = ps->posVar.x * RANDOM_M11(&RANDSEED);
	particle->pos.y = ps->posVar.y * RANDOM_M11(&RANDSEED);

	struct color4f *start = &particle->color;
	start->r = clampf(ps->startColor.r + ps->startColorVar.r * RANDOM_M11(&RANDSEED));
	start->g = clampf(ps->startColor.g + ps->startColorVar.g * RANDOM_M11(&RANDSEED));
	start->b = clampf(ps->startColor.b + ps->startColorVar.b * RANDOM_M11(&RANDSEED));
	start->a = clampf(ps->startColor.a + ps->startColorVar.a * RANDOM_M11(&RANDSEED));

	struct color4f end;
	end.r = clampf(ps->endColor.r + ps->endColorVar.r * RANDOM_M11(&RANDSEED));
	end.g = clampf(ps->endColor.g + ps->endColorVar.g * RANDOM_M11(&RANDSEED));
	end.b = clampf(ps->endColor.b + ps->endColorVar.b * RANDOM_M11(&RANDSEED));
	end.a = clampf(ps->endColor.a + ps->endColorVar.a * RANDOM_M11(&RANDSEED));

	particle->deltaColor.r = (end.r - start->r) / particle->timeToLive;
	particle->deltaColor.g = (end.g - start->g) / particle->timeToLive;
	particle->deltaColor.b = (end.b - start->b) / particle->timeToLive;
	particle->deltaColor.a = (end.a - start->a) / particle->timeToLive;

	float startS = ps->startSize + ps->startSizeVar * RANDOM_M11(&RANDSEED);
	if (startS < 0) {
		startS = 0;
	}

	particle->size = startS;

	if (ps->endSize == START_SIZE_EQUAL_TO_END_SIZE) {
		particle->deltaSize = 0;
	} else {
		float endS = ps->endSize + ps->endSizeVar * RANDOM_M11(&RANDSEED);
		if (endS < 0) {
			endS = 0;
		}
		particle->deltaSize = (endS - startS) / particle->timeToLive;
	}

	// rotation
	float startA = ps->startSpin + ps->startSpinVar * RANDOM_M11(&RANDSEED);
	float endA = ps->endSpin + ps->endSpinVar * RANDOM_M11(&RANDSEED);
	particle->rotation = startA;
	particle->deltaRotation = (endA - startA) / particle->timeToLive;

	// direction
	float a = CC_DEGREES_TO_RADIANS( ps->angle + ps->angleVar * RANDOM_M11(&RANDSEED) );

	// Mode Gravity: A
	if (ps->emitterMode == PARTICLE_MODE_GRAVITY) {
		struct point v;
		v.x = cosf(a);
		v.y = sinf(a);
		float s = ps->mode.A.speed + ps->mode.A.speedVar * RANDOM_M11(&RANDSEED);

		// direction
		particle->mode.A.dir.x = v.x * s ;
		particle->mode.A.dir.y = v.y * s ;

		// radial accel
		particle->mode.A.radialAccel = ps->mode.A.radialAccel + ps->mode.A.radialAccelVar * RANDOM_M11(&RANDSEED);


		// tangential accel
		particle->mode.A.tangentialAccel = ps->mode.A.tangentialAccel + ps->mode.A.tangentialAccelVar * RANDOM_M11(&RANDSEED);

		// rotation is dir
		if(ps->mode.A.rotationIsDir) {
			struct point *p = &(particle->mode.A.dir);
			particle->rotation = -CC_RADIANS_TO_DEGREES(atan2f(p->y,p->x));
		}
	}
	// Mode Radius: B
	else {
		// Set the default diameter of the particle from the source position
		float startRadius = ps->mode.B.startRadius + ps->mode.B.startRadiusVar * RANDOM_M11(&RANDSEED);
		float endRadius = ps->mode.B.endRadius + ps->mode.B.endRadiusVar * RANDOM_M11(&RANDSEED);

		particle->mode.B.radius = startRadius;

		if (ps->mode.B.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS) {
			particle->mode.B.deltaRadius = 0;
		} else {
			particle->mode.B.deltaRadius = (endRadius - startRadius) / particle->timeToLive;
		}

		particle->mode.B.angle = a;
		particle->mode.B.degreesPerSecond = CC_DEGREES_TO_RADIANS(ps->mode.B.rotatePerSecond + ps->mode.B.rotatePerSecondVar * RANDOM_M11(&RANDSEED));
	}
}

static void
_addParticle(struct particle_system *ps) {
	if (ps->particleCount == ps->totalParticles) {
		return;
	}

	struct particle * particle = &ps->particles[ ps->particleCount ];
	_initParticle(ps, particle);
	++ps->particleCount;
}

static void
_stopSystem(struct particle_system *ps) {
	ps->isActive = false;
	ps->elapsed = ps->duration;
	ps->emitCounter = 0;
}

static void
_normalize_point(struct point *p, struct point *out) {
	float l2 = p->x * p->x + p->y *p->y;
	if (l2 == 0) {
		out->x = 1.0f;
		out->y = 0;
	} else {
		float len = sqrtf(l2);
		out->x = p->x/len;
		out->y = p->y/len;
	}
}

static void
_update_particle(struct particle_system *ps, struct particle *p, float dt) {
	if (ps->positionType == POSITION_TYPE_RELATIVE) {
		p->startPos = ps->sourcePosition;
	}
	// Mode A: gravity, direction, tangential accel & radial accel
	if (ps->emitterMode == PARTICLE_MODE_GRAVITY) {
		struct point tmp, radial, tangential;

		radial.x = 0;
		radial.y = 0;
		// radial acceleration
		if (p->pos.x || p->pos.y) {
			_normalize_point(&p->pos, &radial);
		}
		tangential = radial;
		radial.x *= p->mode.A.radialAccel;
		radial.y *= p->mode.A.radialAccel;

		// tangential acceleration
		float newy = tangential.x;
		tangential.x = -tangential.y * p->mode.A.tangentialAccel;
		tangential.y = newy * p->mode.A.tangentialAccel;

		// (gravity + radial + tangential) * dt
		tmp.x = radial.x + tangential.x + ps->mode.A.gravity.x;
		tmp.y = radial.y + tangential.y + ps->mode.A.gravity.y;
		tmp.x *= dt;
		tmp.y *= dt;
		p->mode.A.dir.x += tmp.x;
		p->mode.A.dir.y += tmp.y;
		tmp.x = p->mode.A.dir.x * dt;
		tmp.y = p->mode.A.dir.y * dt;
		p->pos.x += tmp.x;
		p->pos.y += tmp.y;
	}
	// Mode B: radius movement
	else {
		// Update the angle and radius of the particle.
		p->mode.B.angle += p->mode.B.degreesPerSecond * dt;
		p->mode.B.radius += p->mode.B.deltaRadius * dt;

		p->pos.x = - cosf(p->mode.B.angle) * p->mode.B.radius;
		p->pos.y = - sinf(p->mode.B.angle) * p->mode.B.radius;
	}

	p->color.r += (p->deltaColor.r * dt);
	p->color.g += (p->deltaColor.g * dt);
	p->color.b += (p->deltaColor.b * dt);
	p->color.a += (p->deltaColor.a * dt);

	p->rotation += (p->deltaRotation * dt);
}

static void
_remove_particle(struct particle_system *ps, int idx) {
	if ( idx != ps->particleCount-1) {
		ps->particles[idx] = ps->particles[ps->particleCount-1];
	}
	--ps->particleCount;
}

static void
_update(struct particle_system *ps, float dt) {
	if (ps->isActive) {
		float rate = ps->emissionRate;

		// emitCounter should not increase where ps->particleCount == ps->totalParticles
		if (ps->particleCount < ps->totalParticles)	{
			ps->emitCounter += dt;
		}

		while (ps->particleCount < ps->totalParticles && ps->emitCounter > rate) {
			_addParticle(ps);
			ps->emitCounter -= rate;
		}

		ps->elapsed += dt;
		if (ps->duration != DURATION_INFINITY && ps->duration < ps->elapsed) {
			_stopSystem(ps);
		}
	}

	int i = 0;

	while (i < ps->particleCount) {
		struct particle *p = &ps->particles[i];
		p->timeToLive -= dt;
		if (p->timeToLive > 0) {
			_update_particle(ps,p,dt);
			if (p->size <= 0) {
				_remove_particle(ps, i);
			} else {
				++i;
			}
		} else {
			_remove_particle(ps, i);
		}
	}
}

static uint32_t
color4f(struct color4f *c4f) {
	uint8_t rr = (int)(c4f->r*255);
	uint8_t gg = (int)(c4f->g*255);
	uint8_t bb = (int)(c4f->b*255);
	uint8_t aa = (int)(c4f->a*255);
	return (uint32_t)rr << 24 | (uint32_t)gg << 16 | (uint32_t)bb << 8 | aa;
}

static void
calc_mat(struct particle * p, struct matrix *m) {
	matrix_identity(m);
	struct srt srt;
	srt.rot = p->rotation * 1024 / 360;
	srt.scalex = p->size * 1024;
	srt.scaley = srt.scalex;
	srt.offx = (p->pos.x + p->startPos.x) * 1024;
	srt.offy = (p->pos.x + p->startPos.x) * 1024;
	matrix_srt(m, &srt);
}

// ----------------- lua interface

static int
lnew(lua_State *L) {
	luaL_checktype(L,1,LUA_TTABLE);
	struct particle_system * ps = _new(L);
	if (ps == NULL)
		return 0;
	return 1;
}

static int
lupdate(lua_State *L) {
	luaL_checktype(L,1,LUA_TUSERDATA);
	struct particle_system *ps = lua_touserdata(L,1);
	float dt = luaL_checknumber(L,2);
	// float x = luaL_checknumber(L,3);
	// float y = luaL_checknumber(L,4);
	// ps->sourcePosition.x = x;
	// ps->sourcePosition.y = y;
	_update(ps, dt);

	lua_pushboolean(L, ps->isActive);
	return 1;
}

static int
ldata(lua_State *L) {
	luaL_checktype(L,1,LUA_TUSERDATA);
	luaL_checktype(L,2,LUA_TTABLE);
	luaL_checktype(L,3,LUA_TTABLE);
	struct particle_system *ps = lua_touserdata(L,1);
	int n = ps->particleCount;
	int i;
	for (i=0;i<n;i++) {
		struct particle *p = &ps->particles[i];
		calc_mat(p,&ps->matrix[i]);
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
		{ "update", lupdate },
		{ "data", ldata },
		{ NULL, NULL },
	};

	luaL_newlib(L,l);

	return 1;
}


