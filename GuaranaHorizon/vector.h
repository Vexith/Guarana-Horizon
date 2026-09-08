#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
	float x;
	float y;
	float z;
} Vec3;

Vec3 zero(void);
Vec3 add(Vec3 a, Vec3 b);
Vec3 sub(Vec3 a, Vec3 b);
Vec3 scale(Vec3 a, float scalar);
Vec3 negate(Vec3 v);


float dot(Vec3 a, Vec3 b);
float length(Vec3 v);
float length_squared(Vec3 v);

Vec3 normalize(Vec3 v);
Vec3 cross(Vec3 a, Vec3 b);
#endif