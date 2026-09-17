#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"
#include "aircraft.h"

typedef struct {
	Vec3 position;
	Vec3 rotation;
	Vec3 target;
	Vec3 up;
	Vec3 smoothed_forward;
	Vec3 smoothed_up;



	float fov;
	float fov_base;
	float near_plane;
	float far_plane;
	
	bool initialized;
} Camera;

void camera_init(Camera* camera);
void follow_plane(Camera* camera, const Aircraft* aircraft, float dt);

#endif