#ifndef PHYSICS_H
#define PHYSICS_H

#include "vector.h"

typedef struct {
	float gravity, air_density;
} PhysicsWorld;

void physics_init(PhysicsWorld* world);

#endif