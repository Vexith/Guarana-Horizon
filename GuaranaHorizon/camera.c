#include "camera.h"
#include <stddef.h>
#include "aircraft.h"
#include <math.h>

#define CAM_BACK 70.0f
#define CAM_UP 20.0f
#define CAM_LOOK 10.0f

#define CAM_LAG 0.12f
#define CAM_LAG_TURN 0.22f
#define CAM_TARGET_UP 6.0f
#define ROLL_BLEND 0.3f
#define FOV_LAG 0.25f
#define ANGLE_LAG 0.30f


#define CAM_BACK_FAST 100.0f
#define CAM_UP_FAST 30.0f

#define FOV_BASE 70.0f
#define FOV_FAST 82.0f
#define FOV_SPEED_REF 250.0f

void camera_init(Camera* camera) {
	if (camera == NULL) return;

	camera->position = (Vec3){ 0.0f,0.0f,0.0f };
	camera->rotation = (Vec3){ 0.0f,0.0f,0.0f };
	camera->target = (Vec3){ 0.0f, 0.0f, 1.0f };
	camera->up = (Vec3){ 0.0f, 1.0f, 0.0f };
	camera->fov = FOV_BASE;
	camera->smoothed_forward = (Vec3){ 0.0f, 0.0f, 1.0f };
	camera->smoothed_up = (Vec3){ 0.0f, 1.0f, 0.0f };
	camera->fov_base = FOV_BASE;
	camera->near_plane = 0.1f;
	camera->far_plane = 10000.0f;
	camera->initialized = false;
}

void follow_plane(Camera* cam, const Aircraft* aircraft, const AircraftInput* input, float dt) {
	if (cam == NULL || aircraft == NULL) return;
	
	AircraftBasis basis = get_basis(aircraft);

	float speed = length(aircraft->velocity);
	float speed_norm = speed / FOV_SPEED_REF;
	if (speed_norm > 1.0f) speed_norm = 1.0f;
	if (speed_norm < 0.0f) speed_norm = 0.0f;

	float back = CAM_BACK + (CAM_BACK_FAST - CAM_BACK) * speed_norm;
	float up = CAM_UP + (CAM_UP_FAST - CAM_UP) * speed_norm;

	float turn_amount = 0.0f;
	if (input != NULL) {
		turn_amount = fabsf((float)input->roll) + fabsf((float)input->pitch);
		if (turn_amount > 1.0f) turn_amount = 1.0f;
	}
	float lag = CAM_LAG + (CAM_LAG_TURN - CAM_LAG) * turn_amount;

	float angle_alpha = 1.0f - expf(-dt / ANGLE_LAG);

	cam->smoothed_forward = normalize(add(cam->smoothed_forward, scale(sub(basis.forward, cam->smoothed_forward), angle_alpha)));
	cam->smoothed_up = normalize(add(cam->smoothed_up, scale(sub(basis.up, cam->smoothed_up), angle_alpha)));



	Vec3 target_pos = add(aircraft->position,
		add(scale(cam->smoothed_forward, -back), scale(cam->smoothed_up, up)));

	Vec3 target_look = add(aircraft->position, scale(cam->smoothed_forward, CAM_TARGET_UP));

	Vec3 world_up = (Vec3){ 0.0f, 1.0f, 0.0f };
	Vec3 mixed_up = normalize(add(
		scale(basis.up, ROLL_BLEND),
		scale(world_up, 1.0f - ROLL_BLEND)
	));

	float target_fov = FOV_BASE + (FOV_FAST - FOV_BASE) * speed_norm;

	if (!cam->initialized) {
		cam->position = target_pos;
		cam->target = target_look;
		cam->up = mixed_up;
		cam->fov = target_fov;
		cam->initialized = true;
		return;
	}

	float alpha = 1.0f - expf(-dt / lag);
	cam->position = add(cam->position, scale(sub(target_pos, cam->position), alpha));
	cam->target = add(cam->target, scale(sub(target_look, cam->target), alpha));
	cam->up = add(cam->up, scale(sub(mixed_up, cam->up), alpha));

	float fov_alpha = 1.0f - expf(-dt / FOV_LAG);
	cam->fov += (target_fov - cam->fov) * fov_alpha;
}