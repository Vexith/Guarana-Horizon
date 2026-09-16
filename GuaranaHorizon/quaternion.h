#ifndef QUATERNION_H
#define QUATERNION_H

#include "vector.h"

typedef struct {
	float w;
	float x;
	float y;
	float z;
} Quaternion;

Quaternion q_identity(void);
Quaternion q_from_axangle(Vec3 axis, float angle);
Quaternion q_multiply(Quaternion a, Quaternion b);
Quaternion q_normalize(Quaternion q);
Vec3 q_rotatevec(Quaternion q, Vec3 v);

#endif