#ifndef WEAPONS_H
#define WEAPONS_H

#include "aircraft.h"

#define MAX_BULLETS 256
#define BULLET_SPEED 4.0f
#define BULLET_LIFETIME 1.5f
#define BULLET_DAMAGE 15.0f

typedef struct {
	int active;
	Vec3 position;
	Vec3 velocity;
	float ttl;
	int owner_party;
} Bullet;

typedef struct {
	Bullet bullets[MAX_BULLETS];
} BulletPool;

void weapons_init(BulletPool* pool);
void fire(BulletPool* pool, const Aircraft* shooter, int party);
void weapons_update(BulletPool* pool, float dt);
void check_hits(BulletPool* pool, Aircraft** targets, int count, int* target_parties);
void weapons_draw(const BulletPool* pool, const struct Camera* camera);

#endif 