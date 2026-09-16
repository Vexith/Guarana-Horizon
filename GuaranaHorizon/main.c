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

	if (!renderer_init()) {
		printf("Failed to intiliaze SDL3.\n");
		return 1;
	}

	while (running) {
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			input_update(&input, &event);
		}
		camera_init(&camera);

		apply_input(&aircraft, &input, dt);
		update_aircraft(&aircraft, &input,dt);

		follow_plane(&camera, &aircraft, dt);

		clear_renderer();
		draw_ground_grid(&camera);
		draw_cube(&aircraft, &camera);
		//ddraw(&aircraft);
		renderer_present();
		SDL_Delay(16);
	}
	shutdown();

	return 0;
}