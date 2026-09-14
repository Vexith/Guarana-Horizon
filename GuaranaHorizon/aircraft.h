#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "vector.h"
#include "input.h"

typedef struct {
	Vec3 forward;
	Vec3 right;
	Vec3 up;
} AircraftBasis;

typedef struct {
	Vec3 position;
	Vec3 velocity;

	Vec3 rotation; // euler rotation in radians
	Vec3 angular_velocity;
	Vec3 center_of_mass; // center of mass relative to the aircraft origin
	
	
	float mass;
	float wing_area;
	float lift_coefficient;
	float drag_coefficient;
	float engine_thrust;

	float pitch_authority;
	float yaw_authority;
	float roll_authority;

	float throttle;
} Aircraft;

void aircraft_init(Aircraft* aircraft);
void update_aircraft(Aircraft* aircraft, float dt);
void apply_input(Aircraft* aircraft, const AircraftInput* input, float dt);

AircraftBasis get_basis(const Aircraft* aircraft);

float speed(const Aircraft* aircraft);
float altitude(const Aircraft* aircraft);
#endif