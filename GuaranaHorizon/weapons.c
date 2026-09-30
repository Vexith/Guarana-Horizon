#include "weapons.h"
#include "camera.h"
#include "renderer.h"
#include <math.h>
#include <SDL3/SDL.h>

extern SDL_Renderer* g_sdl_renderer;

void weapons_init(BulletPool* pool) {
	if (!pool) return;
	for (int i = 0; i < MAX_BULLETS; i++) {
		pool->bullets[i].active = 0;
	}
}

void fire(BulletPool* pool, const Aircraft* shooter, int party) {
	if (!pool || !shooter || !shooter->alive) return;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!pool->bullets[i].active) {
			Vec3 bfwd, brgt, bup;
			get_basis(shooter, &bfwd, &brgt, &bup);

			pool->bullets[i].position = add(shooter->position, scale(bfwd, 2.0f));
			pool->bullets[i].velocity = scale(bfwd, BULLET_SPEED);

			pool->bullets[i].ttl = BULLET_LIFETIME;
			pool->bullets[i].active = 1;
			pool->bullets[i].owner_party = party;
			return;
		}
	}
}

void weapons_update(BulletPool* pool, float dt) {
	if (!pool) return;
	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!pool->bullets[i].active) continue;

		pool->bullets[i].position = add(pool->bullets[i].position, scale(pool->bullets[i].velocity, dt));
		pool->bullets[i].ttl -= dt;

		if (pool->bullets[i].ttl <= 0.0f) {
			pool->bullets[i].active = 0;
		}

		if (pool->bullets[i].position.y < 0.5f) {
			pool->bullets[i].active = 0;
		}
	}
}
void check_hits(BulletPool* pool, Aircraft** targets, int count, int* target_parties) {
	if (!pool || !targets) return;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!pool->bullets[i].active) continue;

		for (int j = 0; j < count; j++) {
			if (!targets[j] || !targets[j]->alive) continue;

			if (pool->bullets[i].owner_party == target_parties[j]) continue;

			Vec3 d = sub(pool->bullets[i].position, targets[j]->position);
			float dist2 = dot(d, d);
			float hit_radius = 4.0f;
			if (targets[j]->model && targets[j]->model->bound_radius > 0.0f) {
				hit_radius = targets[j]->model->bound_radius * 0.35f;
			}

			if (dist2 < hit_radius * hit_radius) {
				aircraft_apply_damage(targets[j], BULLET_DAMAGE);
				pool->bullets[i].active = 0;
				break;
			}
		}
	}
}

void weapons_draw(const BulletPool* pool, const Camera* cam) {
	if (!pool || !cam || !g_sdl_renderer) return;

	for (int i = 0; i < MAX_BULLETS; i++) {
		if (!pool->bullets[i].active) continue;

		Vec3 tip = pool->bullets[i].position;
		Vec3 tail = sub(tip, scale(pool->bullets[i].velocity, 0.05f));

		float sx1, sy1, sx2, sy2;
		if (!project_point(cam, tip, &sx1, &sy1)) continue;
		if (!project_point(cam, tail, &sx2, &sy2)) continue;

		if (pool->bullets[i].owner_party == 0) {
			SDL_SetRenderDrawColor(g_sdl_renderer, 255, 240, 100, 255);
		}
		else {
			SDL_SetRenderDrawColor(g_sdl_renderer, 255, 100, 60, 255);
		}
		SDL_RenderLine(g_sdl_renderer, sx1, sy1, sx2, sy2);
	}
}
