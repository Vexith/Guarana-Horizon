#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "vector.h"
#include "input.h"
#include "quaternion.h"

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
	
	Quaternion orientation;
	float mass;
	float wing_area;
	float lift_coefficient;
	float drag_coefficient;
	float engine_thrust;

	float pitch_authority;
	float yaw_authority;
	float roll_authority;

	float input_pitch;
	float input_yaw;
	float input_roll;

	float throttle;
	float inertia_x;
	float inertia_y;
	float inertia_z;

	float pitch_torq;
	float yaw_torq;
	float roll_torq;
} Aircraft;

void aircraft_init(Aircraft* aircraft);
void update_aircraft(Aircraft* aircraft, const AircraftInput* input, float dt);
void apply_input(Aircraft* aircraft, const AircraftInput* input, float dt);

AircraftBasis get_basis(const Aircraft* aircraft);

float speed(const Aircraft* aircraft);
float altitude(const Aircraft* aircraft);
#endif