#include "physics.h"
#include <math.h>

int collcheck_ground(Aircraft* air) {
	if (!air || !air->alive) return 0;

	float radius = (air->model && air->model->bound_radius > 0.0f) ? air->model->bound_radius * 0.3f : 3.0f;
	float floor_y = 2.0f + radius;

	if (air->position.y < floor_y) {
		float impact = -air->velocity.y;
		if (impact < 0.0f) impact = 0.0f;

		float dmg = impact * 15.0f;
		if (impact < 0.05f) dmg = 1.0f;

		aircraft_apply_damage(air, dmg);

		air->position.y = floor_y;
		if (air->velocity.y < 0.0f) air->velocity.y = 0.0f;
		if (air->accy < 0.0f) air->accy = 0.0f;
		if (air->forcey < 0.0f) air->forcey = 0.0f;
		return 1;
	}
}

int collcheck_aircraft(Aircraft* air, Aircraft* air2) {
	if (!air || !air2 || !air->alive || !air2->alive) return 0;

	float ra = (air->model && air->model->bound_radius > 0.0f) ? air->model->bound_radius * 0.3f : 3.0f;
	float rb = (air2->model && air2->model->bound_radius > 0.0f) ? air2->model->bound_radius * 0.3f : 3.0f;

	Vec3 d = sub(air->position, air2->position);
	float dist2 = dot(d, d);
	float rsum = ra + rb;

	if (dist2 < rsum * rsum) {
		Vec3 rel_v = sub(air->velocity, air2->velocity);
		float rel_speed = length(rel_v);

		float dmg = rel_speed * 8.0f;
		if (dmg < 5.0f) dmg = 5.0f;

		aircraft_apply_damage(air, dmg);
		aircraft_apply_damage(air2, dmg);
		return 1;
	}
	return 0;
}