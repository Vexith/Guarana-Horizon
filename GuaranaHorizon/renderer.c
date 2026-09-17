#include "renderer.h"
#include <math.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include "aircraft.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

#define WIDTH 1280
#define HEIGHT 720

#define PI 3.14159826f
#define GRID_EXTENT 5000.0f
#define GRID_STEP 100.0f


int renderer_init(void) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		return 0;
	}
	window = SDL_CreateWindow("Game", WIDTH, HEIGHT, 0);
	if (window == NULL) {
		SDL_Quit();
		return 0;
	}

	renderer = SDL_CreateRenderer(window, NULL);
	if (renderer == NULL) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 0;
	}
	return 1;
}

void shutdown(void) {
	if (renderer != NULL)
		SDL_DestroyRenderer(renderer);
	if (window != NULL)
		SDL_DestroyWindow(window);

	SDL_Quit();
}

void clear_renderer(void) {
	bool debug = true;
	if (debug == true) {
		if (!SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255)) {
			printf("Clear color error: %s\n", SDL_GetError());
		}
		if (!SDL_RenderClear(renderer)) {
			printf("Render clear error: %s\n", SDL_GetError());
		}
	}
	else {
	SDL_SetRenderDrawColor(renderer, 10, 10, 20, 255);
	SDL_RenderClear(renderer);
	}
	
	
}

void renderer_present(void) {
	SDL_RenderPresent(renderer);
}
static Vec3 rotate_point(Vec3 point, Vec3 rotation) {
	Vec3 rotated;

	float cos_x = cosf(rotation.x);
	float sin_x = sinf(rotation.x);

	float cos_y = cosf(rotation.y);
	float sin_y = sinf(rotation.y);

	float cos_z = cosf(rotation.z);
	float sin_z = sinf(rotation.z);

	// Pitch
	rotated.x = point.x;
	rotated.y = point.y * cos_x - point.z * sin_x;
	rotated.z = point.y * sin_x + point.z * cos_x;

	point = rotated;

	// Yaw
	rotated.x = point.x * cos_y + point.z * sin_y;
	rotated.y = point.y;
	rotated.z = -point.x * sin_y + point.z * cos_y;

	point = rotated;

	// Roll
	rotated.x = point.x * cos_z - point.y * sin_z;
	rotated.y = point.x * sin_z + point.y * cos_z;
	rotated.z = point.z;
	return rotated;
}
void draw_ground_grid(const Camera* cam) {
	if (cam == NULL) return;

	SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);

	for (float x = -GRID_EXTENT; x <= GRID_EXTENT; x += GRID_STEP) {
		Vec3 a = { x, 0.0f, -GRID_EXTENT };
		Vec3 b = { x, 0.0f, GRID_EXTENT };

		float x1, y1, x2, y2;
		if (!project_point(cam, a, &x1, &y1)) continue;
		if (!project_point(cam, b, &x2, &y2)) continue;
		
		SDL_RenderLine(renderer, x1, y1, x2, y2);
	}

	for (float z = -GRID_EXTENT; z <= GRID_EXTENT; z += GRID_STEP) {
		Vec3 a = { -GRID_EXTENT, 0.0f, z };
		Vec3 b = { GRID_EXTENT, 0.0f, z };

		float x1, y1, x2, y2;

		if (!project_point(cam, a, &x1, &y1)) continue;
		if (!project_point(cam, b, &x2, &y2)) continue;

		SDL_RenderLine(renderer, x1, y1, x2, y2);
	}
}
void draw_airplane(const Aircraft* aircraft, const Camera* camera) {
	if (aircraft == 0 || camera == 0) return;
	
	AircraftBasis basis = get_basis(aircraft);

	Vec3 vertices[6];

	vertices[0] = (Vec3){ 0.0f, 0.0f, 25.0f }; // nose
	vertices[1] = (Vec3){ 0.0f, 0.0f, -15.0f }; // tail
	vertices[2] = (Vec3){ -25.0f, 0.0f, -5.0f }; // left wing
	vertices[3] = (Vec3){ 25.0f, 0.0f, -5.0f }; // right wing
	vertices[4] = (Vec3){ 0.0f, 8.0f, -10.0f }; // upper tail
	vertices[5] = (Vec3){ 0.0f, -5.0f, -10.0f }; // upper tail


	int edges[8][2] = {
		{0, 2}, {0, 3}, {2, 1}, {3, 1},
		{1, 4}, {1, 5}, {2, 3}, {4, 5}  // six seveennn
	};

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	for (int i = 0; i < 6; i++) {
		Vec3 local = vertices[i];

		vertices[i] = aircraft->position;

		vertices[i] = add(vertices[i], scale(basis.forward, local.z));
		vertices[i] = add(vertices[i], scale(basis.right, local.x));
		vertices[i] = add(vertices[i], scale(basis.up, local.y));
	}
	for (int i = 0; i < 8; i++) {
		Vec3 a, b;

		float x1, y1;
		float x2, y2;
		
		a = vertices[edges[i][0]];
		b = vertices[edges[i][1]];

		if (!project_point(camera, a, &x1, &y1))
			continue;

		if (!project_point(camera, b, &x2, &y2))
			continue;

		SDL_RenderLine(renderer, x1, y1, x2, y2);
	}
}
int project_point(const Camera* camera, Vec3 point, float* screen_x, float* screen_y) {
	if (camera == NULL || screen_x == NULL || screen_y == NULL) return 0;

	float fov_radians = camera->fov * PI / 180.0f;
	float focal_length = 1.0f / tanf(fov_radians * 0.5f);
	Vec3 forward = normalize(sub(camera->target, camera->position));
	Vec3 right = normalize(cross(camera->up, forward));
	Vec3 up = cross(forward, right);

	Vec3 relative = sub(point, camera->position);
	Vec3 cam_space;

	cam_space.x = dot(relative, right);
	cam_space.y = dot(relative, up);
	cam_space.z = dot(relative, forward);

	if (cam_space.z <= camera->near_plane) return 0;

	

	*screen_x = WIDTH * 0.5f + (cam_space.x * focal_length / cam_space.z) * (HEIGHT * 0.5f);
	*screen_y = HEIGHT * 0.5f - (cam_space.y * focal_length / cam_space.z) * (HEIGHT * 0.5f);
	
	return 1;
}
