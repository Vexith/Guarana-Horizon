#include "vector.h"
#include <math.h>

// basic math
Vec3 zero(void) { 
Vec3 result = { 0.0f, 0.0f, 0.0f };
return result;
}

Vec3 add(Vec3 a, Vec3 b) {
	Vec3 result;

	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;

	return result;
}

Vec3 sub(Vec3 a, Vec3 b) {
	Vec3 result;

	result.x = a.x - b.x;
	result.y = a.y - b.y;
	result.z = a.z - b.z;
	return result;
}

Vec3 negate(Vec3 v) { return scale(v, -1.0f); }

float dot(Vec3 a, Vec3 b) {
	return a.x * b.x +
		a.y * b.y +
		a.z * b.z;
}
Vec3 scale(Vec3 v, float scalar) {
	Vec3 result;

	result.x = v.x * scalar;
	result.y = v.y * scalar;
	result.z = v.z * scalar;

	return result;
}
float length_squared(Vec3 v) { return dot(v, v); }
float length(Vec3 v) {return sqrtf(length_squared(v));}

Vec3 normalize(Vec3 v) {
	float yourmom = length(v);

	if (yourmom <= 0.000001f)
		return zero();

	return scale(v, 1.0f / yourmom);
}

Vec3 cross(Vec3 a, Vec3 b) {
	Vec3 result;

	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	return result;
}