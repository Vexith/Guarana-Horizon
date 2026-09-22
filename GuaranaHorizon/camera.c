#include "camera.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define MOUSE_SENS 0.0025f
#define CAM_DIST 55.0f
#define CAM_DISTFAST 85.0f
#define CAM_PITCHDEF 0.20f
#define CAM_PITCH_MIN -0.35f
#define CAM_PITCH_MAX 1.15f
#define CAM_LAG 0.10f
#define RECENTER_DELAY 0.80f
#define RECENTER_LAG 0.60f
#define FOV_BASE 70.0f
#define FOV_FAST 85.0f
#define FOV_SPEED_REF 250.0f
#define FOV_LAG 0.25f

static float wrap_pi(float a) {
	while (a > (float)M_PI) a -= 2.0f * (float)M_PI;
	while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
	return a;
}

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

void follow_plane(Camera* cam, const Aircraft* aircraft, float dt, float mouse_dx, float mouse_dy) {
	if (!cam || !aircraft || dt <= 0.0f) return;
	

	const Vec3 world_up = { 0.0f, 1.0f, 0.0f };
	Vec3 basis_forward, basis_right, basis_up;
	get_basis(aircraft, &basis_forward, &basis_right, &basis_up);

	Vec3 forward = normalize(basis_forward);

	bool mouse_active = (fabsf(mouse_dx) > 0.001f) || (fabsf(mouse_dy) > 0.001f);
	if (mouse_active) {
		cam->yaw = wrap_pi(cam->yaw + mouse_dx * MOUSE_SENS);
		cam->pitch += mouse_dy * MOUSE_SENS;
		cam->recenter_timer = 0.0f;
	}
	else {
		cam->recenter_timer += dt;
	}
	if (cam->pitch < CAM_PITCH_MIN) cam->pitch = CAM_PITCH_MIN;
	if (cam->pitch > CAM_PITCH_MAX) cam->pitch = CAM_PITCH_MAX;
	
	Vec3 fwd_h = { forward.x , 0.0f, forward.z };
	if (length(fwd_h) > 1e-4f && cam->recenter_timer > RECENTER_DELAY) {
		fwd_h = normalize(fwd_h);
		float ac_yaw = atan2f(fwd_h.x, fwd_h.z);
		float d = wrap_pi(ac_yaw - cam->yaw);
		float t = 1.0f - expf(-dt / RECENTER_LAG);
		cam->yaw = wrap_pi(cam->yaw + d * t);
	}

	float speed = length(aircraft->velocity);
	float speed_norm = speed / FOV_SPEED_REF;
	if (speed_norm < 0.0f) speed_norm = 0.0f;
	if (speed_norm > 1.0f) speed_norm = 1.0f;

	float distance = CAM_DIST + (CAM_DISTFAST - CAM_DIST) * speed_norm;

	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	Vec3 look_dir = {
		cp * sinf(cam->yaw),
		-sp,
		cp * cosf(cam->yaw)
	};

	Vec3 target = aircraft->position;
	Vec3 desired_pos = sub(target, scale(look_dir, distance));

	if (!cam->initialized) {
		cam->position = desired_pos;
		cam->target = target;
		cam->up = world_up;
		cam->fov = FOV_BASE + (FOV_FAST - FOV_BASE) * speed_norm;
		cam->initialized = true;
		return;
	}

	float alpha = 1.0f - expf(-dt / CAM_LAG);
	cam->position = add(cam->position, scale(sub(desired_pos, cam->position), alpha));
	cam->target = add(cam->target, scale(sub(target, cam->target), alpha));
	cam->up = world_up;

	float target_fov = FOV_BASE + (FOV_FAST - FOV_BASE) * speed_norm;
	float fov_alpha = 1.0f - expf(-dt / FOV_LAG);
	cam->fov += (target_fov - cam->fov) * fov_alpha;
}

void get_viewmatrix(const Camera* cam, float out[16]) {
	Vec3 f = normalize(sub(cam->target, cam->position));
	Vec3 up = cam->up;

	if (fabsf(dot(f, up)) > 0.999f) {
		up = (Vec3){ 0.0f, 0.0f, 1.0f };
		if (fabsf(dot(f, up)) > 0.999f) up = (Vec3){ 1.0f, 0.0f, 0.0f };
	}

	Vec3 s = normalize(cross(f, up));
	Vec3 u = cross(s, f);

	out[0] = s.x;  out[1] = u.x;  out[2] = -f.x;  out[3] = 0.0f;
	out[4] = s.y;  out[5] = u.y;  out[6] = -f.y;  out[7] = 0.0f;
	out[8] = s.z;  out[9] = u.z;  out[10] = -f.z;  out[11] = 0.0f;
	out[12] = -dot(s, cam->position);
	out[13] = -dot(u, cam->position);
	out[14] = dot(f, cam->position);
	out[15] = 1.0f;
}
void get_projmatrix(const Camera* cam, float aspect, float out[16]) {
	float fov_rad = cam->fov * (float)M_PI / 180.0f;
	float f = 1.0f / tanf(fov_rad * 0.5f);
	float n = cam->near_plane;
	float far = cam->far_plane;

	memset(out, 0, sizeof(float) * 16);
	out[0] = f / aspect;
	out[5] = f;
	out[10] = (far + n) / (n - far);
	out[11] = -1.0f;
	out[14] = (2.0f * far * n) / (n - far);
}
