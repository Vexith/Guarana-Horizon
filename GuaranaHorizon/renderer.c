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
#define GRID_EXTENT 1000.0f
#define GRID_STEP 50.0f
#define GRID_CELLS 20

#define MESH_NUM_VERTS ((int)(sizeof(MESH_VERTS) / sizeof(MESH_VERTS[0])))
#define MESH_NUM_TRIS ((int)(sizeof(MESH_TRIS) / sizeof(MESH_TRIS[0])))

static const Vec3 MESH_VERTS[] = {
	{ 0.0f, 0.0f, 25.0f },
	{ 0.0f, 5.0f, -15.0f },
	{ 0.0f, -5.0f, -15.0f },
	{ -22.0f, 0.0f, -3.0f },
	{ 22.0f, 0.0f, -3.0f },
};
static const Tri MESH_TRIS[] = {
	{0, 1, 3},
	{0, 4, 1},
	{0, 2, 4},
	{0, 3, 2},
	{1, 2, 3},
	{1, 4, 2},
};

static const Vec3 WORLD_LIGHT = { 0.408f, 0.816f, 0.408f };

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
static int project_segment(const Camera* cam, Vec3 a, Vec3 b, float* x1, float* x2, float* y1, float* y2) {
	float fov_radians = cam->fov * PI / 180.0f;
	float focal_length = 1.0f / tanf(fov_radians * 0.5f);

	Vec3 forward = normalize(sub(cam->target, cam->position));
	Vec3 right = normalize(cross(cam->up, forward));
	Vec3 up = cross(forward, right);

	Vec3 ra = sub(a, cam->position);
	Vec3 rb = sub(b, cam->position);

	float za = dot(ra, forward);
	float zb = dot(rb, forward);
	float n = cam->near_plane;

	if (za <= n) {
		float t = (n - za) / (zb - za);
		ra = add(ra, scale(sub(rb, ra), t));
		za = n;
	}
	else if (zb <= n) {
		float t = (n - zb) / (za - zb);
		rb = add(rb, scale(sub(ra, rb), t));
		zb = n;
	}

	float xa = dot(ra, right);
	float ya = dot(ra, up);
	float xb = dot(rb, right);
	float yb = dot(rb, up);


	*x1 = WIDTH * 0.5f + (xa * focal_length / za) * (HEIGHT * 0.5f);
	*y1 = HEIGHT * 0.5f - (ya * focal_length / za) * (HEIGHT * 0.5f);
	*x2 = WIDTH * 0.5f + (xb * focal_length / zb) * (HEIGHT * 0.5f);
	*y2 = HEIGHT * 0.5f - (yb * focal_length / zb) * (HEIGHT * 0.5f);

	float L = 1.0e5f;
	if (*x1 > L) *x1 = L;  if (*x1 < -L) *x1 = -L;
	if (*y1 > L) *y1 = L;  if (*y1 < -L) *y1 = -L;
	if (*x2 > L) *x2 = L;  if (*x2 < -L) *x2 = -L;
	if (*y2 > L) *y2 = L;  if (*y2 < -L) *y2 = -L;
	return 1;
}
void draw_ground(const Camera* cam, Vec3 focus) {
	if (cam == NULL) return;

	float cx = floorf(focus.x / GRID_STEP) * GRID_STEP;
	float cz = floorf(focus.z / GRID_STEP) * GRID_STEP;

	float half = GRID_CELLS * 0.5f * GRID_STEP;
	float x0 = cx - half;
	float z0 = cz - half;

	const float near_r = 0.22f, near_g = 0.38f, near_b = 0.22f;
	const float far_r = 0.10f, far_g = 0.13f, far_b = 0.20f;

	static SDL_Vertex verts[GRID_CELLS * GRID_CELLS * 6];
	int n = 0;

	for (int ix = 0; ix < GRID_CELLS; ix++) {
		for (int iz = 0; iz < GRID_CELLS; iz++) {
			float xa = x0 + ix * GRID_STEP;
			float xb = xa + GRID_STEP;
			float za = z0 + iz * GRID_STEP;
			float zb = za + GRID_STEP;

			Vec3 p00 = { xa, 0.0f, za };
			Vec3 p10 = { xb, 0.0f, za };
			Vec3 p11 = { xb, 0.0f, zb };
			Vec3 p01 = { xa, 0.0f, zb };

			float sx00, sy00, sx10, sy10, sx11, sy11, sx01, sy01;
			if (!project_point(cam, p00, &sx00, &sy00)) continue;
			if (!project_point(cam, p10, &sx10, &sy10)) continue;
			if (!project_point(cam, p11, &sx11, &sy11)) continue;
			if (!project_point(cam, p01, &sx01, &sy01)) continue;

			float mcx = (xa + xb) * 0.5f;
			float mcz = (za + zb) * 0.5f;
			float dx = mcx - focus.x;
			float dz = mcz - focus.z;
			float d = sqrtf(dx * dx + dz * dz);

			float t = d / half;
			if (t > 1.0f) t = 1.0f;

			SDL_FColor col = {
				near_r + (far_r - near_r) * t,
				near_g + (far_g - near_g) * t,
				near_b + (far_g - near_b) * t, 1.0f
			};

			SDL_Vertex q[6];
			q[0].position.x = sx00; q[0].position.y = sy00;
			q[1].position.x = sx10; q[1].position.y = sy10;
			q[2].position.x = sx11; q[2].position.y = sy11;
			q[3].position.x = sx00; q[3].position.y = sy00;
			q[4].position.x = sx11; q[4].position.y = sy11;
			q[5].position.x = sx01; q[5].position.y = sy01;

			for (int k = 0; k < 6; k++) {
				q[k].color = col;
				q[k].tex_coord.x = 0.0f;
				q[k].tex_coord.y = 0.0f;
				verts[n++] = q[k];
			}
		}
	}

	if (n > 0) {
		SDL_RenderGeometry(renderer, NULL, verts, n, NULL, 0);
	}
}
void draw_ground_grid(const Camera* cam) {
	if (cam == NULL) return;

	SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);

	for (float x = -GRID_EXTENT; x <= GRID_EXTENT; x += GRID_STEP) {
		Vec3 a = { x, 0.0f, -GRID_EXTENT };
		Vec3 b = { x, 0.0f, GRID_EXTENT };

		float x1, y1, x2, y2;
		if (!project_segment(cam, a, b, &x1, &y1, &x2, &y2)) continue;
		SDL_RenderLine(renderer, x1, y1, x2, y2);
	}

	for (float z = -GRID_EXTENT; z <= GRID_EXTENT; z += GRID_STEP) {
		Vec3 a = { -GRID_EXTENT, 0.0f, z };
		Vec3 b = { GRID_EXTENT, 0.0f, z };

		float x1, y1, x2, y2;
		if (!project_segment(cam, a, b, &x1, &y1, &x2, &y2)) continue;

		if (x1 > 5000.0f || x1 < -5000.0f || y1 > 5000.0f || y1 < -5000.0f) continue;
		if (x2 > 5000.0f || x2 < -5000.0f || y2 > 5000.0f || y2 < -5000.0f) continue;


		SDL_RenderLine(renderer, x1, y1, x2, y2);
	}
}
void draw_airplane(const Aircraft* aircraft, const Camera* camera) {
	if (aircraft == 0 || camera == 0) return;

	Vec3 basis_forward, basis_right, basis_up;
	get_basis(aircraft, &basis_forward, &basis_right, &basis_up);

	const float base_r = 0.85f;
	const float base_g = 0.85f;
	const float base_b = 0.90f;

	Vec3 world[MESH_NUM_VERTS];
	for (int i = 0; i < MESH_NUM_VERTS; i++) {
		Vec3 L = MESH_VERTS[i];
		world[i] = add(aircraft->position,
			add(scale(basis_right, L.x),
				add(scale(basis_up, L.y),
					scale(basis_forward, L.z))));
	}

	SDL_Vertex verts[MESH_NUM_TRIS * 3];
	int n = 0;

	for (int t = 0; t < MESH_NUM_TRIS; t++) {
		Vec3 A = world[MESH_TRIS[t].a];
		Vec3 B = world[MESH_TRIS[t].b];
		Vec3 C = world[MESH_TRIS[t].c];

		Vec3 nrm = cross(sub(B, A), sub(C, A));
		Vec3 to_cam = sub(camera->position, A);
		if (dot(nrm, to_cam) <= 0.0f) continue;

		nrm = normalize(nrm);

		float shade = fabsf(dot(nrm, WORLD_LIGHT));
		if (shade < 0.15f) shade = 0.15f;

		Vec3 pts[3] = { A, B, C };
		float px[3], py[3];
		bool ok = true;
		for (int k = 0; k < 3; k++) {
			if (!project_point(camera, pts[k], &px[k], &py[k])) { ok = false; break; }
		}
		if (!ok) continue;

		SDL_FColor col = { base_r * shade, base_g * shade, base_b * shade, 1.0f };

		for (int k = 0; k < 3; k++) {
			verts[n].position.x = px[k];
			verts[n].position.y = py[k];
			verts[n].color = col;
			verts[n].tex_coord.x = 0.0f;
			verts[n].tex_coord.y = 0.0f;
			n++;
		}
	}

	if (n > 0) {
		SDL_RenderGeometry(renderer, NULL, verts, n, NULL, 0);
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
