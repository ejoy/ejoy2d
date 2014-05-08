
/*
	This particle system is come from cocos2d-x ( https://github.com/cocos2d/cocos2d-x )
	The origin source is :
		https://github.com/cocos2d/cocos2d-x/blob/develop/cocos/2d/CCParticleSystem.cpp
	Need rewrite later :)
 */



#include "particle.h"
#include "spritepack.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

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
	*seed = *seed * 134775813 + 1;
	union {
		uint32_t d;
		float f;
	} u;
	u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
	return u.f - 3.0f;
}

static void
_initParticle(struct particle_system *ps, struct particle* particle) {
	uint32_t RANDSEED = rand();

	particle->timeToLive = ps->config->life + ps->config->lifeVar * RANDOM_M11(&RANDSEED);
	if (particle->timeToLive <= 0) {
		return;
	}

	int *mat = particle->emitMatrix.m;
	if (ps->config->emitterMatrix) {
		memcpy(mat, ps->config->emitterMatrix->m, 6 * sizeof(int));
	} else {
		mat[0] = 1024;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = 1024;
		mat[4] = 0;
		mat[5] = 0;
	}
	particle->startPos = ps->config->sourcePosition;
	particle->pos.x = ps->config->posVar.x * RANDOM_M11(&RANDSEED);
	particle->pos.y = ps->config->posVar.y * RANDOM_M11(&RANDSEED);

	struct color4f *start = &particle->color;
	start->r = clampf(ps->config->startColor.r + ps->config->startColorVar.r * RANDOM_M11(&RANDSEED));
	start->g = clampf(ps->config->startColor.g + ps->config->startColorVar.g * RANDOM_M11(&RANDSEED));
	start->b = clampf(ps->config->startColor.b + ps->config->startColorVar.b * RANDOM_M11(&RANDSEED));
	start->a = clampf(ps->config->startColor.a + ps->config->startColorVar.a * RANDOM_M11(&RANDSEED));

	struct color4f end;
	end.r = clampf(ps->config->endColor.r + ps->config->endColorVar.r * RANDOM_M11(&RANDSEED));
	end.g = clampf(ps->config->endColor.g + ps->config->endColorVar.g * RANDOM_M11(&RANDSEED));
	end.b = clampf(ps->config->endColor.b + ps->config->endColorVar.b * RANDOM_M11(&RANDSEED));
	end.a = clampf(ps->config->endColor.a + ps->config->endColorVar.a * RANDOM_M11(&RANDSEED));

	particle->deltaColor.r = (end.r - start->r) / particle->timeToLive;
	particle->deltaColor.g = (end.g - start->g) / particle->timeToLive;
	particle->deltaColor.b = (end.b - start->b) / particle->timeToLive;
	particle->deltaColor.a = (end.a - start->a) / particle->timeToLive;

	float startS = ps->config->startSize + ps->config->startSizeVar * RANDOM_M11(&RANDSEED);
	if (startS < 0) {
		startS = 0;
	}

	particle->size = startS;

	if (ps->config->endSize == START_SIZE_EQUAL_TO_END_SIZE) {
		particle->deltaSize = 0;
	} else {
		float endS = ps->config->endSize + ps->config->endSizeVar * RANDOM_M11(&RANDSEED);
		if (endS < 0) {
			endS = 0;
		}
		particle->deltaSize = (endS - startS) / particle->timeToLive;
	}


	// rotation
	float startA = ps->config->startSpin + ps->config->startSpinVar * RANDOM_M11(&RANDSEED);
	float endA = ps->config->endSpin + ps->config->endSpinVar * RANDOM_M11(&RANDSEED);
	particle->rotation = startA;
	particle->deltaRotation = (endA - startA) / particle->timeToLive;

	// direction
	float a = CC_DEGREES_TO_RADIANS( ps->config->angle + ps->config->angleVar * RANDOM_M11(&RANDSEED) );

	// Mode Gravity: A
	if (ps->config->emitterMode == PARTICLE_MODE_GRAVITY) {
		struct point v;
		v.x = cosf(a);
		v.y = sinf(a);
		float s = ps->config->mode.A.speed + ps->config->mode.A.speedVar * RANDOM_M11(&RANDSEED);

		// direction
		particle->mode.A.dir.x = v.x * s ;
		particle->mode.A.dir.y = v.y * s ;

		// radial accel
		particle->mode.A.radialAccel = ps->config->mode.A.radialAccel + ps->config->mode.A.radialAccelVar * RANDOM_M11(&RANDSEED);


		// tangential accel
		particle->mode.A.tangentialAccel = ps->config->mode.A.tangentialAccel + ps->config->mode.A.tangentialAccelVar * RANDOM_M11(&RANDSEED);

		// rotation is dir
		if(ps->config->mode.A.rotationIsDir) {
			struct point *p = &(particle->mode.A.dir);
			particle->rotation = -CC_RADIANS_TO_DEGREES(atan2f(p->y,p->x));
		}
	}
	// Mode Radius: B
	else {
		// Set the default diameter of the particle from the source position
		float startRadius = ps->config->mode.B.startRadius + ps->config->mode.B.startRadiusVar * RANDOM_M11(&RANDSEED);
		float endRadius = ps->config->mode.B.endRadius + ps->config->mode.B.endRadiusVar * RANDOM_M11(&RANDSEED);

		particle->mode.B.radius = startRadius;

		if (ps->config->mode.B.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS) {
			particle->mode.B.deltaRadius = 0;
		} else {
			particle->mode.B.deltaRadius = (endRadius - startRadius) / particle->timeToLive;
		}

		particle->mode.B.angle = a;
		particle->mode.B.degreesPerSecond = CC_DEGREES_TO_RADIANS(ps->config->mode.B.rotatePerSecond + ps->config->mode.B.rotatePerSecondVar * RANDOM_M11(&RANDSEED));
	}
}

static void
_addParticle(struct particle_system *ps) {
	if (ps->particleCount == ps->config->totalParticles) {
		return;
	}

	struct particle * particle = &ps->particles[ ps->particleCount ];
	_initParticle(ps, particle);
	++ps->particleCount;
}

static void
_stopSystem(struct particle_system *ps) {
	ps->isActive = false;
	ps->elapsed = ps->config->duration;
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
	if (ps->config->positionType != POSITION_TYPE_GROUPED) {
		p->startPos = ps->config->sourcePosition;
	}
	// Mode A: gravity, direction, tangential accel & radial accel
	if (ps->config->emitterMode == PARTICLE_MODE_GRAVITY) {
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
		tmp.x = radial.x + tangential.x + ps->config->mode.A.gravity.x;
		tmp.y = radial.y + tangential.y + ps->config->mode.A.gravity.y;
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

	p->size += (p->deltaSize * dt);
	if (p->size < 0)
		p->size = 0;

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

void
init_with_particles(struct particle_system *ps, int numberOfParticles) {
	ps->particles = (struct particle *)(ps+1);
	ps->matrix = (struct matrix *)(ps->particles + numberOfParticles);
	ps->config = (struct particle_config*)(ps->matrix + numberOfParticles);
	ps->allocatedParticles = numberOfParticles;
	ps->isActive = false;
	ps->config->totalParticles = numberOfParticles;
	ps->config->positionType = POSITION_TYPE_RELATIVE;
	ps->config->emitterMode = PARTICLE_MODE_GRAVITY;
}

void
particle_system_reset(struct particle_system *ps) {
	ps->isActive = true;
	ps->emitCounter = 0.0;
	ps->particleCount = 0;
	ps->elapsed = 0.0;
}

void
calc_particle_system_mat(struct particle * p, struct matrix *m, int edge) {
	matrix_identity(m);
	struct srt srt;
	srt.rot = p->rotation * 1024 / 360;
	srt.scalex = p->size * 1024 / edge;
	srt.scaley = srt.scalex;
	srt.offx = (p->pos.x + p->startPos.x) * SCREEN_SCALE;
	srt.offy = (p->pos.y + p->startPos.y) * SCREEN_SCALE;
	matrix_srt(m, &srt);

	struct matrix tmp;
	memcpy(tmp.m, m, sizeof(int) * 6);
	matrix_mul(m, &tmp, &p->emitMatrix);
}

void
particle_system_update(struct particle_system *ps, float dt) {
	if (ps->isActive) {
		float rate = ps->config->emissionRate;

		// emitCounter should not increase where ps->particleCount == ps->totalParticles
		if (ps->particleCount < ps->config->totalParticles)	{
			ps->emitCounter += dt;
		}

		while (ps->particleCount < ps->config->totalParticles && ps->emitCounter > rate) {
			_addParticle(ps);
			ps->emitCounter -= rate;
		}

		ps->elapsed += dt;
		if (ps->config->duration != DURATION_INFINITY && ps->config->duration < ps->elapsed) {
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

	ps->isAlive = ps->particleCount > 0;
}

