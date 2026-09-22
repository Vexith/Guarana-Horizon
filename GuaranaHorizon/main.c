#include <stdio.h>
#include <SDL3/SDL.h>

#include "aircraft.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"

int main(void) {
	Aircraft aircraft;
	//AircraftInput input;
	SDL_Event event;
	Camera camera;

	bool running = true;
	const float dt = 1.0f / 60.0f;

	aircraft_init(&aircraft);
	AircraftInput input;
	input_init(&input);
	input.thrust = aircraft.maxthrust * 0.7f;

	if (!renderer_init()) {
		printf("Failed to intiliaze SDL3.\n");
		return 1;
	}
	camera_init(&camera);
	while (running) {
		float mouse_dx = 0.0f, mouse_dy = 0.0f;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION) {
				mouse_dx += event.motion.xrel;
				mouse_dy += event.motion.yrel;
			}
			input_update(&input, &event);
		}
		
		aircraft_update(&aircraft, &input);

		follow_plane(&camera, &aircraft, dt, mouse_dx, mouse_dy);

		clear_renderer();
		draw_ground(&camera, aircraft.position);
		//draw_ground_grid(&camera);
		draw_airplane(&aircraft, &camera);
		
		renderer_present();
		SDL_Delay(16);
	}
	shutdown();

	return 0;
}