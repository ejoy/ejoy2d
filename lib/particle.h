#ifndef ejoy_2d_particle_h
#define ejoy_2d_particle_h

#include <lua.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "matrix.h"


#define CC_DEGREES_TO_RADIANS(__ANGLE__) ((__ANGLE__) * 0.01745329252f) // PI / 180
#define CC_RADIANS_TO_DEGREES(__ANGLE__) ((__ANGLE__) * 57.29577951f) // PI * 180

#define PARTICLE_MODE_GRAVITY 0
#define PARTICLE_MODE_RADIUS 1

#define POSITION_TYPE_FREE 0
#define POSITION_TYPE_RELATIVE 1
#define POSITION_TYPE_GROUPED 2

/** The Particle emitter lives forever */
#define DURATION_INFINITY (-1)

/** The starting size of the particle is equal to the ending size */
#define START_SIZE_EQUAL_TO_END_SIZE (-1)

/** The starting radius of the particle is equal to the ending radius */
#define START_RADIUS_EQUAL_TO_END_RADIUS (-1)

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
	struct matrix emitMatrix;

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

struct particle_config {
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

	//Emitter name
//    std::string _configName;

	// color modulate
	//    BOOL colorModulate;

	/** How many seconds the emitter will run. -1 means 'forever' */
	float duration;
	struct matrix* emitterMatrix;
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
//	bool opacityModifyRGB;
	/** does FlippedY variance of each particle */
//	int yCoordFlipped;

	/** particles movement type: Free or Grouped
	 @since v0.8
	 */
	int positionType;
};

struct particle_system {
	//! time elapsed since the start of the system (in seconds)
	float elapsed;
	//! Array of particles
	struct particle *particles;
	struct matrix *matrix;

	//! How many particles can be emitted per second
	float emitCounter;

	//!  particle idx
	//int particleIdx;

	// Number of allocated particles
	int allocatedParticles;

	/** Is the emitter active */
	bool isActive;

	/* Is the system has particle alive */
	bool isAlive;

	/** Quantity of particles that are being simulated at the moment */
	int particleCount;

	struct particle_config *config;
};


void init_with_particles(struct particle_system *ps, int numberOfParticles);
void particle_system_update(struct particle_system *ps, float dt);
void calc_particle_system_mat(struct particle * p, struct matrix *m, int edge);
void particle_system_reset(struct particle_system *ps);

int ejoy2d_particle(lua_State *L);

#endif
