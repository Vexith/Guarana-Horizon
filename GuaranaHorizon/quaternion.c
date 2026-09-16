#include "quaternion.h"
#include <math.h>

Quaternion q_identity(void) {
	Quaternion q;

	q.w = 1.0f;
	q.x = 0.0f;
	q.y = 0.0f;
	q.z = 0.0f;

	return q;
}

Quaternion q_from_axangle(Vec3 axis, float angle) {
	Quaternion q;

	float half_angle = angle * 0.5f;
	float s = sinf(half_angle);
	axis = normalize(axis);

	q.w = cosf(half_angle);
	q.x = axis.x * s;
	q.y = axis.y * s;
	q.z = axis.z * s;

	return q;
}

Quaternion q_multiply(Quaternion a, Quaternion b) {
	Quaternion q;

	q.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z; // what it is this shit???? /s
	q.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
	q.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
	q.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;

	return q;
}

Quaternion q_normalize(Quaternion q) {
	float magnitude;

	magnitude = sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);

	if (magnitude <= 1e-8f) {
		return q_identity();
	}

	q.w /= magnitude;
	q.x /= magnitude;
	q.y /= magnitude;
	q.z /= magnitude;

	return q;
}

Vec3 q_rotatevec(Quaternion q, Vec3 v) {
	Quaternion vec_q; // vector quaternion
	Quaternion inverse;
	Quaternion result;

	vec_q.w = 0.0f;
	vec_q.x = v.x;
	vec_q.y = v.y;
	vec_q.z = v.z;

	inverse.w = q.w;
	inverse.x = -q.x;
	inverse.y = -q.y;
	inverse.z = -q.z;

	result = q_multiply(q_multiply(q, vec_q), inverse);

	return (Vec3) {result.x, result.y, result.z};
	
}