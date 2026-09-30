#include <stdio.h>
#include <SDL3/SDL.h>
#include "model.h"
#include "aircraft.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"
#include "ai_bot.h"
#include "physics.h"
#include "weapons.h"

#define PLANE_FILE "assets/plane.obj"


int main(void) {
	Aircraft aircraft;
	AircraftInput input;
	SDL_Event event;
	Camera camera;
	BulletPool bullets;
	Bot bots[3];

	bool running = true;
	const float dt = 1.0f / 60.0f;
	
	aircraft_init(&aircraft);
	bot_spawnpoint(&aircraft, bots);
	
	Model* m = load_obj(PLANE_FILE, 50.0f);
	if (m) {
		model_free(aircraft.model);
		aircraft.model = m;
	}
	else {
		printf("FAIL TO LOAD 3D MODEL going to fallback");
		aircraft.model = create_default();
	}


	input_init(&input);
	input.thrust = aircraft.maxthrust * 0.7f;

	if (!renderer_init()) {
		printf("Failed to intiliaze SDL3.\n");
		return 1;
	}

	camera_init(&camera);
	weapons_init(&bullets);

	for (int i = 0; i < 3; i++) {
		bots[i].aircraft.model = aircraft.model;
		bots[i].aircraft.spawn_position = bots[i].aircraft.position;
	}

	while (running) {
		// input
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			input_update(&input, &event);
		}
		// is alive
		for (int i = 0; i < 3; i++) {
			if (bots[i].aircraft.alive) {
				bot_update(&bots[i], &aircraft, dt);
			}
			else {
				bots[i].aircraft.respawn_timer += dt;
				if (bots[i].aircraft.respawn_timer > 2.0f) {
					aircraft_respawn(&bots[i].aircraft);
				}
			}
		}
		if (aircraft.alive) {
			aircraft_update(&aircraft, &input);
		}
		else {
			aircraft.respawn_timer += dt;
			if (aircraft.respawn_timer > 2.0f) {
				aircraft_respawn(&aircraft);
			}
		}
		
		// collision with the ground
		if (aircraft.alive) collcheck_ground(&aircraft);
		for (int i = 0; i < 3; i++) {
			if (bots[i].aircraft.alive) collcheck_ground(&bots[i].aircraft);
		}
		// collision plane with plane
		for (int i = 0; i < 3; i++) {
			collcheck_aircraft(&aircraft, &bots[i].aircraft);
			for (int j = i + 1; j < 3; j++) {
				collcheck_aircraft(&bots[i].aircraft, &bots[j].aircraft);
			}
		}
		// guns (player)
		static float player_fire_cd = 0.0f;
		if (player_fire_cd > 0.0f) player_fire_cd -= dt;
		if (input.fire && aircraft.alive && player_fire_cd <= 0.0f) {
			fire(&bullets, &aircraft, 0);
			player_fire_cd = 0.10f;
		}
		// guns (bot)
		for (int i = 0; i < 3; i++) {
			if (bots[i].wants_to_fire && bots[i].aircraft.alive) {
				fire(&bullets, &bots[i].aircraft, 1);
				bots[i].wants_to_fire = 0;
			}
		}

		weapons_update(&bullets, dt);

		// collision bullet with plane
		{
			Aircraft* targets[4] = { &aircraft, &bots[0].aircraft,&bots[1].aircraft, &bots[2].aircraft };
			int parties[4] = { 0, 1, 1, 1 };
			check_hits(&bullets, targets, 4, parties);
		}
		// flash decay damage
		if (aircraft.damage_flash > 0.0f) {
			aircraft.damage_flash -= dt * 3.0f;
			if (aircraft.damage_flash < 0.0f) aircraft.damage_flash = 0.0f;
		}
		for (int i = 0; i < 3; i++) {
			if (bots[i].aircraft.damage_flash > 0.0f) {
				bots[i].aircraft.damage_flash -= dt * 3.0f;
				if (bots[i].aircraft.damage_flash < 0.0f) bots[i].aircraft.damage_flash = 0.0f;
			}
		}
		
		if (!aircraft.alive) {
			aircraft.respawn_timer += dt;
			if (aircraft.respawn_timer > 2.0f) {
				aircraft_respawn(&aircraft);
				printf("PLAYER RESPAWNED \n");
			}
		}
		for (int i = 0; i < 3; i++) {
			if (!bots[i].aircraft.alive) {
				bots[i].aircraft.respawn_timer += dt;
				if (bots[i].aircraft.respawn_timer > 2.0f) {
					aircraft_respawn(&bots[i].aircraft);
					printf("BOT  %d  RESPAWNED\n", i);
				}
			}
		}

		follow_plane(&camera, &aircraft, dt);

		clear_renderer();
		draw_ground(&camera, aircraft.position);

		static int frame = 0;
		if (++frame % 60 == 0) {
			for (int i = 0; i < 3; i++) {
				printf("BOT %d: alive=%d y=%.1f dur=%.1f pos=(%.0f,%.0f,%.0f)\n",
					i,
					bots[i].aircraft.alive,
					bots[i].aircraft.position.y,
					bots[i].aircraft.durability,
					bots[i].aircraft.position.x,
					bots[i].aircraft.position.y,
					bots[i].aircraft.position.z);
			}
		}
		printf("player: y=%.1f speed=%.2f gamma=%.1f vy=%.2f\n",
			aircraft.position.y,
			aircraft.realspeed,
			aircraft.gamma,
			aircraft.velocity.y);
		Vec3 my_color = color_hex(0x8080FF);
		typedef struct {
			Aircraft* ac;
			Vec3 color;
			float depth;
		} RenderItem;
		
		RenderItem items[4];
		items[0].ac = &aircraft;
		items[0].color = my_color;
		items[1].ac = &bots[0].aircraft; items[1].color = bots[0].my_color;
		items[2].ac = &bots[1].aircraft; items[2].color = bots[1].my_color;
		items[3].ac = &bots[2].aircraft; items[3].color = bots[2].my_color;

		for (int i = 0; i < 4; i++) {
			Vec3 d = sub(items[i].ac->position, camera.position);
			items[i].depth = dot(d, d);
		}

		for (int i = 0; i < 4; i++) {
			for (int j = i + 1; j < 4; j++) {
				if (items[j].depth > items[i].depth) {
					RenderItem tmp = items[i];
					items[i] = items[j];
					items[j] = tmp;
				}
			}
		}

		for (int i = 0; i < 4; i++) {
			draw_airplane(items[i].ac, &camera, items[i].color);
		}
		renderer_present();
		SDL_Delay(16);
	}
	shutdown();

	return 0;
}
