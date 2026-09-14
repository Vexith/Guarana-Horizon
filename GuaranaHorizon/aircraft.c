#include "aircraft.h"
#include <math.h>
#include <stdio.h>

#define POSITION (aircraft)->position
#define VELOCITY (aircraft)->velocity
#define ROTATION (aircraft)->rotation
#define ANGULAR (aircraft)->angular_velocity
#define CENMASS (aircraft)->center_of_mass
#define MASS (aircraft)->mass
#define WING_AREA (aircraft)->wing_area
#define LIFT_CO (aircraft)->lift_coefficient
#define DRAG_CO (aircraft)->drag_coefficient
#define ENGINETHRUST (aircraft)->engine_thrust

#define PITCH_AUTH (aircraft)->pitch_authority
#define YAW_AUTH (aircraft)->yaw_authority
#define ROLL_AUTH (aircraft)->roll_authority
#define THROTTLE (aircraft)->throttle

#define AIR_DENSITY 1.225f
#define GRAVITY 9.80665f

#define ANGULAR_DAMPING 2.0f
#define EPSILON 0.00000001f

#define LIFT_SLOPE 4.5f
#define MAX_ADA 0.35f
#define STALL_ADA 0.26f
#define STALL_LIFT 0.35f

#define INDUCED_DRAG 0.0000000001f
#define THROTTLE_RATE 0.25f
#define CONTROL_RESPONSE 8.0f

static float clamp_float(float value, float minimum, float maximum) {
	if (value < minimum) { return minimum; }
	if (value > maximum) { return maximum; }
	return value;
}



void aircraft_init(Aircraft* aircraft) {
	if (aircraft == 0) return; 

	POSITION = (Vec3){ 0.0f, 100.0f, 0.0f };
	VELOCITY = (Vec3){ 0.0f, 0.0f, 70.0f };
	ROTATION = (Vec3){ 0.0f, 0.0f, 0.0f };

	ANGULAR = zero();
	CENMASS = zero();
	
	MASS = 1200.0f; // 1200kg aircraft
	WING_AREA = 18.0f; // 18 m^2 wing area

	LIFT_CO = 0.35f;
	DRAG_CO = 0.035f;

	ENGINETHRUST = 15000.0f; // maximum engine thrust in newtons

	PITCH_AUTH = 1.8f;
	YAW_AUTH = 0.8f;
	ROLL_AUTH = 3.0f;
	THROTTLE = 0.65f;
}

AircraftBasis get_basis(const Aircraft* aircraft) {
	AircraftBasis basis;

	float pitch, yaw, roll, cp, sp, cy, sy, cr, sr;

	pitch = ROTATION.x;
	yaw = ROTATION.y;
	roll = ROTATION.z;

	cp = cosf(pitch);
	sp = sinf(pitch);

	cy = cosf(yaw);
	sy = sinf(yaw);

	cr = cosf(roll);
	sr = sinf(roll);

	// forward calc
	basis.forward.x = sy * cp;
	basis.forward.y = -sp;
	basis.forward.z = cy * cp;
	// right calc
	basis.right.x = cy * cr + sy * sp * sr;
	basis.right.y = cp * sr;
	basis.right.z = -sy * cr + cy * sp * sr;
	// up calc
	basis.up.x = -cy * sr + sy * sp * cr;
	basis.up.y = cp * cr;
	basis.up.z = sy * sr + cy * sp * cr;

	basis.forward = normalize(basis.forward);
	basis.right = normalize(basis.right);
	basis.up = normalize(basis.up);

	return basis;
}
void apply_input(Aircraft* aircraft, const AircraftInput* input, float dt) {
	if (aircraft == 0 || input == 0) return;

	ANGULAR.x += (input->pitch * PITCH_AUTH - ANGULAR.x) * CONTROL_RESPONSE * dt;
	ANGULAR.y += (input->yaw * YAW_AUTH - ANGULAR.y) * CONTROL_RESPONSE * dt;
	ANGULAR.z += (input->roll * ROLL_AUTH - ANGULAR.z) * CONTROL_RESPONSE * dt;

	THROTTLE += input->throttle * THROTTLE_RATE * dt;
	THROTTLE = clamp_float(THROTTLE, 0.0f, 1.0f);
}
void update_aircraft(Aircraft* aircraft, float dt){
	AircraftBasis basis;

	Vec3 velocity_direction, total_force, thrust_force, lift_force, drag_force, gravity_force;

	float vertical_speed = 0.0f;
	float angle_of_attack = 0.0f;
	float speed = 0.0f;
	float dynamic_pressure = 0.0f;
	float acceleration = 0.0f;

	float stall_factor = 0.0f;
	float forward_speed = 0.0f;
	float lift = 0.0f;
	float drag = 0.0f;
	float effective_lift_co = 0.0f;
	float induced_drag = 0.0f;
	if (aircraft == 0) return; // sanity checking
	if (dt <= 0.0f) return;

	basis = get_basis(aircraft);
	speed = length(VELOCITY);
	if (!isfinite(speed)) {
		printf("FATAL ERROR: speed became invalid\n");
		return;
	}
	total_force = zero();

	// thrust calc
	thrust_force = scale(basis.forward, ENGINETHRUST * clamp_float(THROTTLE, 0.0f, 1.0f));
	total_force = add(total_force, thrust_force);

	// gravity calc
	gravity_force.x = 0.0f;
	gravity_force.y = -MASS * GRAVITY;
	gravity_force.z = 0.0f;

	total_force = add(total_force, gravity_force);

	// aerodynamics calcs (q = 1/2 * rho * v^2)
	forward_speed = dot(VELOCITY, basis.forward);
	vertical_speed = dot(VELOCITY, basis.up);

	angle_of_attack = 0.0f;

	if (forward_speed > EPSILON) {
		angle_of_attack = atan2f(-vertical_speed, forward_speed);
	}
	angle_of_attack = clamp_float(angle_of_attack, -MAX_ADA, MAX_ADA);

	if (forward_speed > EPSILON) {
		dynamic_pressure = 0.5f * AIR_DENSITY * forward_speed * forward_speed;

		effective_lift_co = LIFT_CO + angle_of_attack * LIFT_SLOPE;

		if (fabsf(angle_of_attack) > STALL_ADA) {
			stall_factor = 1.0f - (
				(fabsf(angle_of_attack) - STALL_ADA) /
				(MAX_ADA - STALL_ADA)
				);
			stall_factor = clamp_float(stall_factor, 0.0f, 1.0f);

			effective_lift_co *= STALL_LIFT + (1.0f - STALL_LIFT) * stall_factor;
		}
		lift = dynamic_pressure * WING_AREA * effective_lift_co;

		if (!isfinite(lift)) {
			printf("FATAL ERROR: lift became invalid\n");
			return;
		}
		lift_force = scale(basis.up, lift);
		total_force = add(total_force, lift_force);
	}
	if (speed > EPSILON) {
		velocity_direction = normalize(VELOCITY);

		dynamic_pressure = 0.5f * AIR_DENSITY * speed * speed;
		drag = dynamic_pressure * WING_AREA * DRAG_CO;

		// producing lift also produces additional drag; simplified intentionally
		induced_drag = INDUCED_DRAG * lift * lift;

		drag += induced_drag;

		if (!isfinite(drag)) {
			printf("FATAL ERROR: drag became invalid\n");
			return;
		}
		drag_force = scale(velocity_direction, -drag);
		total_force = add(total_force, drag_force);

	}
	acceleration = 1.0f / MASS;

	VELOCITY = add(VELOCITY, scale(total_force, acceleration * dt));

	POSITION = add(POSITION, scale(VELOCITY, dt));

	float damping;

	damping = clamp_float(1.0f - ANGULAR_DAMPING * dt, 0.0f, 1.0f);

	ANGULAR.x *= damping;
	ANGULAR.y *= damping;
	ANGULAR.z *= damping;

	ROTATION.x += ANGULAR.x * dt;
	ROTATION.y += ANGULAR.y * dt;
	ROTATION.z += ANGULAR.z * dt;

}

