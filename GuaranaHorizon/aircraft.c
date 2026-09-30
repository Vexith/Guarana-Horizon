#include "aircraft.h"
#include "quaternion.h"
#include "vector.h"
#include "input.h"
#include <stdio.h>

#include <string.h>

#define EPS 1e-8f
#define INPUT_SMOOTH_TIME 0.20f


void aircraft_init(Aircraft* a) {
	if (!a) return;
	memset(a, 0, sizeof(*a));

	a->position = (Vec3){ 0.0f, 30.0f, 0.0f };
	a->velocity = zero();
	a->angular_velocity = zero();
	
	a->orientation = q_from_axangle((Vec3) { 0.0f, 1.0f, 0.0f }, (float)PI);
	a->elevator_smooth = 0.0f;
	a->ailerion_smooth = 0.0f;
	a->rudder_smooth = 0.0f;

	a->gamma = 180.0f;
	a->theta = 0.0f;
	a->phi = 0.0f;

	a->thrust = 0.0f;
	a->recthrust = 0.0f;

	a->realspeed = 0.3f;
	a->DragEffect = 1.0f;

	a->maxthrust = 1.5f;
	a->manoeverability = 0.15f;
	a->RollRate = 0.55f;
	a->maxgamma = 65.0f;
	a->maxtheta = 90.0f;
	a->inertia = 2.5f;
	a->deadweight = 0.13f;
	a->StallSpeed = 0.13f;
	a->DiveSpeedLimit1 = 0.35f;
	a->DiveSpeedStructuralLimit = 0.45f;
	a->SeaLevelSpeedLimitThreshold = 0.36f;
	a->SpeedBrakePower = 1.0f;

	a->AirDensityDrag = 1.0f;
	a->GammaDrag = 1.0f;
	a->LoopedBeyondVerticalDrag = 1.0f;
	a->SpeedBeyondStructuralLimitsDrag = 1.0f;
	a->FlapDrag = 1.0f;
	a->UndercarriageDrag = 1.0f;
	a->SpeedBrakeDrag = 1.0f;
	
	a->SpeedHistoryIdx = 0;
	a->InertiallyDampenedPlayerSpeed = 0.30f;

	a->max_durability = 100.0f;
	a->durability = 100.0f;
	a->alive = 1;
	a->damage_flash = 0.0f;
}
void set_model(Aircraft* a, float maxthrust, float manoeverability, float RollRate,
	float maxgamma, float maxtheta, float inertia,
	float deadweight, float StaticDrag,
	float StallSpeed, float DiveSpeedLimit1, float DiveSpeedStructuralLimit,
	float SeaLevelSpeedLimitThreshold,
	float CompressibilitySpeed, float CompressibilitySpeedWithSpeedBrakes,
	float MaxFullPowerAltRatio, float ServiceCeilingAltitude,
	float FlapSpeed, float SpeedBrakePower) {
	if (!a) return;
	a->max_durability = 100.0f;
	a->durability = 100.0f;
	a->alive = 1;
	a->damage_flash = 0.0f;

	a->maxthrust = maxthrust;
	a->manoeverability = manoeverability;
	a->RollRate = RollRate;
	a->maxgamma = maxgamma;
	a->maxtheta = maxtheta;
	a->inertia = inertia;
	a->deadweight = deadweight;
	a->StallSpeed = StallSpeed;
	a->DiveSpeedLimit1 = DiveSpeedLimit1;
	a->DiveSpeedStructuralLimit = DiveSpeedStructuralLimit;
	a->SeaLevelSpeedLimitThreshold = SeaLevelSpeedLimitThreshold;
	a->SpeedBrakePower;
}
void get_basis(const Aircraft* a, Vec3* fwd, Vec3* right, Vec3* up) {
	if (!a) {
		if (fwd) *fwd = (Vec3){ 0.0f, 0.0f, 1.0f };
		if (right) *right = (Vec3){ 1.0f, 0.0f, 0.0f };
		if (up) *up = (Vec3){ 0.0f, 1.0f, 0.0f };
		return;
	}
	if (fwd) *fwd = q_rotatevec(a->orientation, (Vec3) { 0.0f, 0.0f, 1.0f });
	if (right) *right = q_rotatevec(a->orientation, (Vec3) { 1.0f, 0.0f, 0.0f });
	if (up) *up = q_rotatevec(a->orientation, (Vec3) { 0.0f, 1.0f, 0.0f });
}

void get_euler(const Aircraft* a, float* gamma, float* theta, float* phi) {
	Vec3 fwd, right, up;
	get_basis(a, &fwd, &right, &up);

	if (phi) {
		float p = atan2f(-fwd.x, -fwd.z) * R2D;
		if (p < 0.0f) p += 360.0f;
		*phi = p;
	}
	if (gamma) {
		float fy = clampf(fwd.y, -1.0f, 1.0f);
		*gamma = 180.0f - asinf(fy) * R2D;
	}
	if (theta) {
		*theta = atan2f(-right.y, up.y) * R2D;
	}
}

void aircraft_update(Aircraft* a, const AircraftInput* in) {
	if (!a || !in) { printf(">>> a ou in NULL\n"); return; }

	Vec3 vaxis, raxis, uaxis;
	Quaternion dq_pitch, dq_yaw, dq_roll, dq;
	float timefac, elevator, aileron, rudder;
	float P_rad, Y_rad, R_rad;
	float new_gamma, new_theta, theta_change;
	float throttlechange;
	float braking, brakepower;
	float stepfac, scalef;
	float gravityforce, pitch_angle;
	float avg;
	int i;

	if (!a || !in) return;
	timefac = in->timefac;
	if (timefac <= 0.0f) return;
	if (a->realspeed <= 0.0f) a->realspeed = 1.0f;

	elevator = clampf(in->elevator, -1.0f, 1.0f);
	aileron = clampf(in->ailerion, -1.0f, 1.0f);
	rudder = clampf(in->rudder, -1.0f, 1.0f);
	{
		float dt_real = timefac / 60.0f;
		float alpha = 1.0f - expf(-dt_real / INPUT_SMOOTH_TIME);

		a->elevator_smooth += (elevator - a->elevator_smooth) * alpha;
		a->ailerion_smooth += (aileron - a->ailerion_smooth) * alpha;
		a->rudder_smooth += (rudder - a->rudder_smooth) * alpha;

		elevator = a->elevator_smooth;
		aileron = a->ailerion_smooth;
		rudder = a->rudder_smooth;
	}
	a->elevatoreffect = elevator;
	a->rolleffect = aileron;
	a->ruddereffect = rudder;
	a->FlapsLevel = in->FlapsLevel;
	a->UndercarriageLevel = in->UndercarriageLevel;
	a->SpeedBrakeLevel = in->SpeedBrakeLevel;

	a->recthrust = clampf(in->thrust, 0.0f, a->maxthrust);
	throttlechange = a->maxthrust / 100.0f * timefac;
	if (a->recthrust > a->thrust + throttlechange) {
		a->thrust += throttlechange * 2.0f;
	}
	else if (a->recthrust < a->thrust - throttlechange) {
		a->thrust -= throttlechange * 0.5f;
	}
	if (a->thrust > a->maxthrust) a->thrust = a->maxthrust;
	if (a->thrust < 0.0f) a->thrust = 0.0f;

	P_rad = elevator * a->manoeverability * (3.33f + 8.0f * a->realspeed) * timefac * D2R;
	Y_rad = -rudder * a->manoeverability * (0.66f + 2.0f * a->realspeed) * timefac * D2R;
	R_rad = -aileron * a->RollRate * (1.0f + a->realspeed) * timefac * 3.5f * D2R;
	{
		float pitch_offset = a->gamma - 180.0f;
		float stability_k = 0.0010f;

		P_rad -= pitch_offset * stability_k;
	}



	{
		const float inertia_factor = 0.88f;
		const float inertia_yaw = 0.90f;
		const float inertia_roll = 0.78f;

		a->angular_velocity.x = a->angular_velocity.x * inertia_factor + P_rad * (1.0f - inertia_factor);
		a->angular_velocity.y = a->angular_velocity.y * inertia_yaw + Y_rad * (1.0f - inertia_factor);
		a->angular_velocity.z = a->angular_velocity.z * inertia_roll + R_rad * (1.0f - inertia_factor);

		P_rad = a->angular_velocity.x;
		Y_rad = a->angular_velocity.y;
		R_rad = a->angular_velocity.z;
	}

	


	float gamma_hi = 180.0f + a->maxgamma;
	float gamma_lo = 180.0f - a->maxgamma;
	float soft_zone = 12.0f;
	float spring_k = 0.002f;
	
	if (a->gamma < gamma_lo + soft_zone) {
		float x = ((gamma_lo + soft_zone) - a->gamma) / soft_zone;
		if (x > 1.0f) x = 1.0f;
		P_rad += x * x * spring_k;
	}
	else if (a->gamma > gamma_hi - soft_zone) {
		float x = (a->gamma - (gamma_hi - soft_zone)) / soft_zone;
		if (x > 1.0f) x = 1.0f;
		P_rad -= x * x * spring_k;
	}

	new_gamma = a->gamma + P_rad * R2D;
	if (new_gamma < gamma_lo) {
		new_gamma = gamma_lo;
		a->angular_velocity.x *= 0.5f;
	}
	if (new_gamma > gamma_hi) {
		new_gamma = gamma_hi;
		a->angular_velocity.x *= 0.5f;
	}
	a->angular_velocity = (Vec3){ P_rad, Y_rad, R_rad };
	dq_pitch = q_from_axangle((Vec3) { 1.0f, 0.0f, 0.0f }, P_rad);
	dq_yaw = q_from_axangle((Vec3) { 0.0f, 1.0f, 0.0f }, Y_rad);
	dq_roll = q_from_axangle((Vec3) { 0.0f, 0.0f, 1.0f }, R_rad);
	dq = q_multiply(dq_pitch, q_multiply(dq_yaw, dq_roll));
	a->orientation = q_normalize(q_multiply(a->orientation, dq));

	
	get_euler(a, &a->gamma, &a->theta, &a->phi);
	get_basis(a, &vaxis, &raxis, &uaxis);

	braking = (fabsf(rudder * 6.0f) +
		fabsf(elevator * 10.0f) +
		fabsf(aileron * 5.0f) +
		a->DragEffect) / 500.0f;
	brakepower = powf(0.93f - braking, timefac);
	a->accx *= brakepower;
	a->accy *= brakepower;
	a->accz *= brakepower;

	a->accx += a->thrust * vaxis.x * 1.3f * timefac;
	a->accy += a->thrust * vaxis.y * 0.5f * timefac;
	a->accz += a->thrust * vaxis.z * 1.5f * timefac;

	a->accx += a->thrust * uaxis.x * 0.067f * timefac;
	a->accz += a->thrust * uaxis.z * 0.067f * timefac;

	a->accy -= a->thrust * uaxis.y * 0.067f * timefac * cosf(a->theta * D2R);

	float vfwd = dot(a->velocity, vaxis);
	float vup = dot(a->velocity, uaxis);
	float vlat = dot(a->velocity, raxis);

	float speed = length(a->velocity);
	if (speed < 0.01f) speed = 0.01f;

	float aoa = 0.0f;
	if (vfwd > 0.01f) {
		aoa = atan2f(-vup, vfwd);
	}
	if (aoa > 0.45f) aoa = 0.45f;
	if (aoa < -0.45f) aoa = -0.45f;

	float CL = aoa * 4.5f;
	if (CL > 1.3f) CL = 1.3f;
	if (CL < -1.3f) CL = -1.3f;

	float CD = 0.025f + CL * CL * 0.06f;

	float q = 0.5f * a->InertiallyDampenedPlayerSpeed * a->InertiallyDampenedPlayerSpeed;

	float lift = q * CL * 1.8f;
	a->accx += -vaxis.x * vup / speed * 0 + lift * uaxis.x * 0;

	a->accx += lift * uaxis.x;
	a->accy += lift * uaxis.y;
	a->accz += lift * uaxis.z;

	float drag = q * CD * 1.8f;
	Vec3 vdir = scale(a->velocity, 1.0f / speed);
	a->accx -= vdir.x * drag;
	a->accy -= vdir.y * drag;
	a->accz -= vdir.z * drag;

	a->accy -= 0.15f * timefac;



	stepfac = 0.24f;

	a->velocity.x += a->accx * timefac;
	a->velocity.y += a->accy * timefac;
	a->velocity.z += a->accz * timefac;


	a->position.x += a->velocity.x * timefac * stepfac;
	a->position.z += a->velocity.z * timefac * stepfac;
	a->position.y += a->velocity.y * timefac * stepfac;

	scalef = 1.1f;
	a->forcex = a->accx * stepfac * scalef;
	a->forcey = a->accy * stepfac * scalef;
	a->forcez = a->accz * stepfac * scalef;


	{
		float alt = a->position.y;
		float d = 1.0f - alt * 0.000007f;
		if (d < 0.4f) d = 0.4f;
		a->AirDensityDrag = d;
	}
	a->forcex /= a->AirDensityDrag;
	a->forcey /= a->AirDensityDrag;
	a->forcez /= a->AirDensityDrag;

	{
		float excess = 0.0f;
		float hi = 180.0f + a->maxgamma;
		float lo = 180.0f - a->maxgamma;
		if (a->gamma > hi) excess = a->gamma - hi;
		else if (a->gamma < lo) excess = lo - a->gamma;
		a->GammaDrag = 1.0f + excess * 0.05f;
	}
	a->forcex /= a->GammaDrag;
	a->forcey /= a->GammaDrag;
	a->forcez /= a->GammaDrag;

	{
		float fy = vaxis.y;
		a->LoopedBeyondVerticalDrag = (fy < -0.3f) ? (1.0f - fy) : 1.0f;
	}
	a->forcex /= a->LoopedBeyondVerticalDrag;
	a->forcey /= a->LoopedBeyondVerticalDrag;
	a->forcez /= a->LoopedBeyondVerticalDrag;

	{
		float s = a->realspeed;
		if (s > a->DiveSpeedStructuralLimit) {
			float over = s - a->DiveSpeedStructuralLimit;
			a->SpeedBeyondStructuralLimitsDrag = 1.0f + over * 5.0f;
		}
		else {
			a->SpeedBeyondStructuralLimitsDrag = 1.0f;
		}
	}
	a->forcex /= a->SpeedBeyondStructuralLimitsDrag;
	a->forcey /= a->SpeedBeyondStructuralLimitsDrag;
	a->forcez /= a->SpeedBeyondStructuralLimitsDrag;
	{
		float d = 1.0f;
		if (a->FlapsLevel > 0.0f) {
			float f = clampf(a->FlapsLevel / 4.0f, 0.0f, 1.0f);
			d = 1.0f + f * 0.35f;
		}
		a->FlapDrag = d;
	}
	a->forcex /= a->FlapDrag;
	a->forcey /= a->FlapDrag;
	a->forcez /= a->FlapDrag;

	a->UndercarriageDrag = a->UndercarriageLevel ? 1.25f : 1.0f;
	a->forcex /= a->UndercarriageDrag;
	a->forcey /= a->UndercarriageDrag;
	a->forcez /= a->UndercarriageDrag;

	a->UndercarriageDrag = a->UndercarriageLevel ? 1.25f : 1.0f;
	a->forcex /= a->UndercarriageDrag;
	a->forcey /= a->UndercarriageDrag;
	a->forcez /= a->UndercarriageDrag;

	
	a->SpeedBrakeDrag = a->SpeedBrakeLevel ? a->SpeedBrakePower : 1.0f;
	a->forcex /= a->SpeedBrakeDrag;
	a->forcey /= a->SpeedBrakeDrag;
	a->forcez /= a->SpeedBrakeDrag;


	{
		float fmag = sqrtf(a->forcex * a->forcex
			+ a->forcey * a->forcey
			+ a->forcez * a->forcez);
		float limit = a->SeaLevelSpeedLimitThreshold * 3.0f;
		if (fmag > limit && fmag > EPS) {
			float k = limit / fmag;
			a->RegulatedForceX = a->forcex * k;
			a->RegulatedForceY = a->forcey * k;
			a->RegulatedForceZ = a->forcez * k;
		}
		else {
			a->RegulatedForceX = a->forcex;
			a->RegulatedForceY = a->forcey;
			a->RegulatedForceZ = a->forcez;
		}
	}
	a->forcex = a->RegulatedForceX;
	a->forcey = a->RegulatedForceY;
	a->forcez = a->RegulatedForceZ;

	{
		float r2 = a->forcex * a->forcex
			+ a->forcey * a->forcey
			+ a->forcez * a->forcez;
		if (r2 <= 0.0f) r2 = 0.001f;
		a->realspeed = sqrtf(r2);
	}

	a->SpeedHistoryArray[a->SpeedHistoryIdx] = a->realspeed;
	a->SpeedHistoryIdx = (a->SpeedHistoryIdx + 1) % 10;
	avg = 0.0f;
	for (i = 0; i < 10; i++) avg += a->SpeedHistoryArray[i];
	a->InertiallyDampenedPlayerSpeed = avg / 10.0f;

	a->velocity.x = a->forcex;
	a->velocity.y = a->forcey;
	a->velocity.z = a->forcez;


	(void)pitch_angle; 
}
void aircraft_respawn(Aircraft* air) {
	if (!air) return;

	air->position = air->spawn_position;
	air->orientation = q_from_axangle((Vec3) { 0, 1, 0 }, air->spawn_yaw + PI);
	air->velocity = zero();
	air->angular_velocity = zero();
	air->accx = air->accy = air->accz = 0.0f;
	air->forcex = air->forcey = air->forcez = 0.0f;
	air->thrust = 0.0f;
	air->recthrust = 0.0f;
	air->realspeed = 0.3f;

	Vec3 basis_fwd, basis_rgt, basis_up;
	get_basis(air, &basis_fwd, &basis_rgt, &basis_up);
	air->velocity = scale(basis_fwd, 0.5f);

	air->durability = air->max_durability;
	air->alive = 1;
	air->damage_flash = 0.0f;
	air->respawn_timer = 0.0f;
}
void aircraft_apply_damage(Aircraft* a, float dmg) {
	if (!a || !a->alive) return;
	a->durability -= dmg;
	a->damage_flash = 1.0f;
	if (a->durability <= 0.0f) {
		a->durability = 0.0f;
		a->alive = 0;
	}
}
