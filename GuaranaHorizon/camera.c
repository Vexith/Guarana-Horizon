#include "camera.h"
#include <math.h>
#include <string.h>

#define MOUSE_SENS 0.0025f
#define CAM_DIST 55.0f
#define CAM_DISTFAST 85.0f
#define CAM_PITCHDEF 0.20f
#define CAM_PITCH_MIN -0.35f
#define CAM_PITCH_MAX 1.15f
#define CAM_LAG 0.10f
#define RECENTER_DELAY 0.80f
#define RECENTER_LAG 0.60f
#define FOV_BASE 40.0f
#define FOV_FAST 70.0f
#define FOV_SPEED_REF 1.5f
#define FOV_LAG 0.25f

#define CAM_TURN_RATE 1.5f
#define CAM_TURN_SPEED_FAC 0.8f    
#define CAM_PITCH_FOLLOW 0.45f   
#define CAM_PITCH_LAG 0.30f   
#define CAM_BACK_DIST 35.0f
#define CAM_BACK_SPEED_FAC 30.0f
#define CAM_UP_DIST 12.0f
#define CAM_UP_SPEED_FAC 8.0f
#define CAM_LOOKAHEAD 30.0f
#define CAM_BANK_BLEND 0.30f
#define CAM_GROUND_MIN 5.0f



void camera_init(Camera* cam) {
	if (!cam) return;
	memset(cam, 0, sizeof(*cam));

	cam->up = (Vec3){ 0.0f, 1.0f, 0.0f };
	cam->yaw = 0.0f;
	cam->pitch = CAM_PITCHDEF;
	cam->fov = FOV_BASE;
	cam->near_plane = 0.1f;
	cam->far_plane = 10000.0f;
	cam->initialized = false;
}

static float wrap_pi(float a) {
	while (a > (float)PI) a -= 2.0f * (float)PI;
	while (a < -(float)PI) a += 2.0f * (float)PI;
	return a;
}

void follow_plane(Camera* cam, const Aircraft* air, float dt) {
	if (!cam || !air || dt <= 0.0f) return;

	Vec3 basis_fwd, basis_rgt, basis_up;
	get_basis(air, &basis_fwd, &basis_rgt, &basis_up);

	Vec3 vel_h = { air->velocity.x, 0.0f, air->velocity.z };
	float speed_h = length(vel_h);

	Vec3 move_dir;
	if (speed_h > 0.01f) {
		move_dir = normalize(vel_h);
	}
	else {
		Vec3 fwd_h = { basis_fwd.x, 0.0f, basis_fwd.z };
		move_dir = (length(fwd_h) > 0.01f) ? normalize(fwd_h) : (Vec3) { 0, 0, 1 };
	}

	float target_yaw = atan2f(move_dir.x, move_dir.z);
	if (!cam->initialized) {
		cam->yaw = target_yaw;
	}
	else {
		float dyaw = wrap_pi(target_yaw - cam->yaw);
		float max_step = (CAM_TURN_RATE + speed_h * CAM_TURN_SPEED_FAC) * dt;
		if (dyaw > max_step) dyaw = max_step;
		if (dyaw < -max_step) dyaw = -max_step;
		cam->yaw = wrap_pi(cam->yaw + dyaw);
	}

	float aircraft_pitch = asinf(clampf(basis_fwd.y, -1.0f, 1.0f));
	float target_pitch = aircraft_pitch * CAM_PITCH_FOLLOW;
	if (!cam->initialized) {
		cam->pitch = target_pitch;
	}
	else {
		float a = 1.0f - expf(-dt / CAM_PITCH_LAG);
		cam->pitch += (target_pitch - cam->pitch) * a;
	}

	float dist = CAM_BACK_DIST + speed_h * CAM_BACK_SPEED_FAC;
	float height = CAM_UP_DIST + speed_h * CAM_UP_SPEED_FAC;

	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	Vec3 cam_dir = {
		cp * sinf(cam->yaw),
		-sp,
		cp * cosf(cam->yaw)
	};

	Vec3 desired_pos = sub(air->position, scale(cam_dir, dist));
	desired_pos.y += height;

	if (!cam->initialized) {
		cam->position = desired_pos;
	}
	else {
		float a = 1.0f - expf(-dt / CAM_LAG);
		cam->position = add(cam->position, scale(sub(desired_pos, cam->position), a));
	} 

	Vec3 look_point = add(air->position, scale(move_dir, CAM_LOOKAHEAD));
	if (!cam->initialized) {
		cam->target = look_point;
	}
	else {
		float a = 1.0f - expf(-dt / CAM_LAG);
		cam->target = add(cam->target, scale(sub(look_point, cam->target), a));
	}

	Vec3 up_blend = { basis_up.x * CAM_BANK_BLEND, basis_up.y * CAM_BANK_BLEND + (1.0f - CAM_BANK_BLEND), basis_up.z * CAM_BANK_BLEND };
	cam->up = normalize(up_blend);

	float sn = speed_h / FOV_SPEED_REF;
	if (sn > 1.0f) sn = 1.0f;
	sn = sn * sn;
	float target_fov = FOV_BASE + (FOV_FAST - FOV_BASE) * sn;
	float a_fov = 1.0f - expf(-dt / FOV_LAG);
	cam->fov += (target_fov - cam->fov) * a_fov;

	if (cam->position.y < CAM_GROUND_MIN) cam->position.y = CAM_GROUND_MIN;
	cam->initialized = true;
}