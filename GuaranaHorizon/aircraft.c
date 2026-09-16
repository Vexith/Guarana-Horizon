#include "aircraft.h"
#include <math.h>
#include <stdio.h>
#include "quaternion.h"
#include "input.h"

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
#define PITCH_STAB 2.5f
#define YAW_STAB 1.5f
#define ROLL_STAB 1.2f

#define PITCH_TORQ 5000.0f
#define YAW_TORQ 2500.0f
#define ROLL_TORQ 8000.0f

#define WING_SPAN 11.0f
#define MEAN_CHORD 1.65f

#define CM_ALPHA 4.0f
#define CM_Q -15.0f
#define CN_BETA 0.3f
#define CN_R (-0.5f)
#define CL_BETA (-0.10f)
#define CL_P (-0.8f)

#define CM_ELEVATOR 0.30f
#define CN_RUDDER 0.15f
#define CL_AILERON 0.06f
#define INPUT_RAMP_TIME 1.2f


static float clamp_float(float value, float minimum, float maximum) {
	if (value < minimum) { return minimum; }
	if (value > maximum) { return maximum; }
	return value;
}



void aircraft_init(Aircraft* aircraft) {
	if (aircraft == 0) return; 

	aircraft->inertia_x = 3000.0f;
	aircraft->inertia_y = 3500.0f;
	aircraft->inertia_z = 5000.0f;

	aircraft->pitch_torq = 800.0f;
	aircraft->yaw_torq = 400.0f;
	aircraft->roll_torq = 1200.0f;

	aircraft->orientation = q_identity();
	aircraft->position = (Vec3){ 0.0f, 100.0f, 0.0f };
	aircraft->velocity = (Vec3){ 0.0f, 0.0f, 70.0f };
	(aircraft)->rotation = (Vec3){ 0.0f, 0.0f, 0.0f };

	aircraft->input_pitch = 0.0f;
	aircraft->input_yaw = 0.0f;
	aircraft->input_roll = 0.0f;

	(aircraft)->angular_velocity = zero();
	(aircraft)->center_of_mass = zero();
	
	(aircraft)->mass = 1200.0f; // 1200kg aircraft
	(aircraft)->wing_area = 18.0f; // 18 m^2 wing area

	(aircraft)->lift_coefficient = 0.35f;
	(aircraft)->drag_coefficient = 0.035f;

	(aircraft)->engine_thrust = 15000.0f; // maximum engine thrust in newtons

	(aircraft)->throttle = 0.65f;
}

AircraftBasis get_basis(const Aircraft* aircraft) {
	AircraftBasis basis;

	if (aircraft == 0) {
		basis.forward = (Vec3){ 0.0f, 0.0f, 1.0f };
		basis.right = (Vec3){ 1.0f, 0.0f, 0.0f };
		basis.up = (Vec3){ 0.0f, 1.0f, 0.0f };
		return basis;
	}
	basis.forward = q_rotatevec(aircraft->orientation, (Vec3) { 0.0f, 0.0f, 1.0f });
	basis.right = q_rotatevec(aircraft->orientation, (Vec3) { 1.0f, 0.0f, 0.0f });
	basis.up = q_rotatevec(aircraft->orientation, (Vec3) { 0.0f, 1.0f, 0.0f });

	return basis;
}
void apply_input(Aircraft* aircraft, const AircraftInput* input, float dt) {
	if (aircraft == 0 || input == 0) return;

	(aircraft)->throttle += input->throttle * THROTTLE_RATE * dt;
	(aircraft)->throttle = clamp_float((aircraft)->throttle, 0.0f, 1.0f);
}
void update_aircraft(Aircraft* aircraft,const AircraftInput* input, float dt){
	if (aircraft == 0) return; // sanity checking
	if (dt <= 0.0f) return;
	AircraftBasis basis;
	

	Vec3 side_forcevec, velocity_direction, total_force, thrust_force, lift_force, drag_force, gravity_force;
	Vec3 angular_momentum;
	Vec3 gyroscopic, lift_direction;

	
	float speed = 0.0f;
	float forward_speed = 0.0f;
	float vertical_speed = 0.0f;
	float lateral_speed = 0.0f;
	
	float angle_of_attack = 0.0f;
	float sideslip_angle = 0.0f;

	float dynamic_pressure = 0.0f;
	float lift = 0.0f;
	float drag = 0.0f;
	float side_force = 0.0f;

	float effective_lift_co = 0.0f;
	float induced_drag = 0.0f;
	float stall_factor = 0.0f;
	float acceleration;

	float pitch_mom = 0.0f;
	float yaw_mom = 0.0f;
	float roll_mom = 0.0f;



	basis = get_basis(aircraft);
	speed = length(aircraft->velocity);

	if (!isfinite(speed)) {
		printf("FATAL ERROR: speed became invalid\n");
		return;
	}
	total_force = zero();

	// thrust calc
	thrust_force = scale(basis.forward, (aircraft)->engine_thrust * clamp_float((aircraft)->throttle, 0.0f, 1.0f));
	total_force = add(total_force, thrust_force);

	// gravity calc
	gravity_force.x = 0.0f;
	gravity_force.y = -(aircraft)->mass * GRAVITY;
	gravity_force.z = 0.0f;

	total_force = add(total_force, gravity_force);

	// aerodynamics calcs (q = 1/2 * rho * v^2)
	forward_speed = dot((aircraft)->velocity, basis.forward);
	vertical_speed = dot((aircraft)->velocity, basis.up);
	lateral_speed = dot((aircraft)->velocity, basis.right);

	if (forward_speed > EPSILON) {
		angle_of_attack = atan2f(-vertical_speed, forward_speed);
		sideslip_angle = atan2f(lateral_speed, forward_speed);
	}

	angle_of_attack = clamp_float(angle_of_attack, -MAX_ADA, MAX_ADA);
	// aerodynamic
	if (forward_speed > EPSILON) {
		dynamic_pressure = 0.5f * AIR_DENSITY * speed * speed;

		effective_lift_co = (aircraft)->lift_coefficient + angle_of_attack * LIFT_SLOPE;
		// stall
		if (fabsf(angle_of_attack) > STALL_ADA) {
			stall_factor = 1.0f - (
				(fabsf(angle_of_attack) - STALL_ADA) /
				(MAX_ADA - STALL_ADA)
				);
			stall_factor = clamp_float(stall_factor, 0.0f, 1.0f);

			effective_lift_co *= STALL_LIFT + (1.0f - STALL_LIFT) * stall_factor;
		}
		lift = dynamic_pressure * (aircraft)->wing_area * effective_lift_co;

		if (!isfinite(lift)) {
			printf("FATAL ERROR: lift became invalid\n");
			return;
		}
		
		side_force = -0.8f * sideslip_angle * dynamic_pressure * (aircraft)->wing_area;
		side_forcevec = scale(basis.right, side_force);

		total_force = add(total_force, side_forcevec);
	}
	if (speed > EPSILON) {
		velocity_direction = normalize((aircraft)->velocity);
		lift_direction = sub(basis.up, scale(velocity_direction, dot(basis.up, velocity_direction)));

		if (length_squared(lift_direction) > EPSILON) {
			lift_direction = normalize(lift_direction);

			lift_force = scale(lift_direction, lift);
			total_force = add(total_force, lift_force);
		}

		dynamic_pressure = 0.5f * AIR_DENSITY * speed * speed;
		if (forward_speed > EPSILON) {
			drag = dynamic_pressure * (aircraft)->wing_area * (aircraft)->drag_coefficient;
			induced_drag = INDUCED_DRAG * lift * lift;
			drag += induced_drag;
		}
		else {
			drag = 0.0f;
		}
		if (!isfinite(drag)) {
			printf("FATAL ERROR: drag became invalid\n");
			return;
		}
		drag_force = scale(velocity_direction, -drag);
		total_force = add(total_force, drag_force);

	}
	acceleration = 1.0f / (aircraft)->mass;

	(aircraft)->velocity = add((aircraft)->velocity, scale(total_force, acceleration * dt));

	aircraft->position = add(aircraft->position, scale((aircraft)->velocity, dt));
	

	float ramp_step = dt / INPUT_RAMP_TIME;

	aircraft->input_pitch += ((float)input->pitch - aircraft->input_pitch) * ramp_step;
	aircraft->input_yaw += ((float)input->yaw - aircraft->input_yaw) * ramp_step;
	aircraft->input_roll += ((float)input->roll - aircraft->input_roll) * ramp_step;

	if (input != 0 && speed > EPSILON) {
		float q_dyn = 0.5f * AIR_DENSITY * speed * speed;

		pitch_mom += q_dyn * (aircraft)->wing_area * MEAN_CHORD * CM_ELEVATOR * aircraft->input_pitch;
		yaw_mom += q_dyn * (aircraft)->wing_area * WING_SPAN * CN_RUDDER * aircraft->input_yaw;
		roll_mom += q_dyn * (aircraft)->wing_area * WING_SPAN * CL_AILERON * aircraft->input_roll;
	}
	if (speed > EPSILON) {
		float q;
		float pitch_rate;
		float yaw_rate;
		float roll_rate;
		

		q = 0.5f * AIR_DENSITY * speed * speed;

		pitch_rate = (aircraft)->angular_velocity.x * MEAN_CHORD / (2.0f * speed);
		yaw_rate = (aircraft)->angular_velocity.y * WING_SPAN / (2.0f * speed);
		roll_rate = (aircraft)->angular_velocity.z * WING_SPAN / (2.0f * speed);

		pitch_mom += q * (aircraft)->wing_area * MEAN_CHORD * (CM_ALPHA * angle_of_attack + CM_Q * pitch_rate);
		yaw_mom += q * (aircraft)->wing_area * WING_SPAN * (CN_BETA * sideslip_angle + CN_R * yaw_rate);
		roll_mom += q * (aircraft)->wing_area * WING_SPAN * (CL_BETA * sideslip_angle + CL_P * roll_rate);
	}
	angular_momentum = (Vec3){
		(aircraft)->angular_velocity.x * aircraft->inertia_x,
		(aircraft)->angular_velocity.y * aircraft->inertia_y,
		(aircraft)->angular_velocity.z * aircraft->inertia_z,
	};
	
	gyroscopic = cross((aircraft)->angular_velocity, angular_momentum);

	(aircraft)->angular_velocity.x += (pitch_mom - gyroscopic.x) / aircraft->inertia_x * dt;
	(aircraft)->angular_velocity.y += (yaw_mom - gyroscopic.y) / aircraft->inertia_y * dt;
	(aircraft)->angular_velocity.z += (roll_mom - gyroscopic.z) / aircraft->inertia_z * dt;


	// ANGULAR Z

	{
		Vec3 omega = (aircraft)->angular_velocity;
		float wmag = length(omega);
		Quaternion dq;

		if (wmag > EPSILON) {
			Vec3 axis = scale(omega, 1.0f / wmag);
			dq = q_from_axangle(axis, wmag * dt);
		}
		else {
			dq = q_identity();
		}

		aircraft->orientation = q_multiply(aircraft->orientation, dq);
		aircraft->orientation = q_normalize(aircraft->orientation);
	}
	/*pitch_rot = q_from_axangle(rot_basis.right, ANGULAR.x * dt);
	yaw_rot = q_from_axangle(rot_basis.up, ANGULAR.y * dt);
	roll_rot = q_from_axangle(rot_basis.forward, ANGULAR.z * dt);

	rot_delta = q_multiply(roll_rot, q_multiply(yaw_rot, pitch_rot));
	aircraft->orientation = q_multiply(aircraft->orientation, rot_delta);
	aircraft->orientation = q_normalize(aircraft->orientation);
	*/

	printf(
		"YAW %.2f | ANG %.3f %.3f %.3f | "
		"AOA %.3f BETA %.3f | MOM %.1f %.1f %.1f\n",
		input != 0 ? input->yaw : 0.0f,
		(aircraft)->angular_velocity.x,
		(aircraft)->angular_velocity.y,
		(aircraft)->angular_velocity.z,
		angle_of_attack,
		sideslip_angle,
		pitch_mom,
		yaw_mom,
		roll_mom
	);

}

