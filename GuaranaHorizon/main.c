#include <stdio.h>
#include <SDL3/SDL.h>

#include "aircraft.h"
#include "input.h"
#include "renderer.h"

int main(void) {
	Aircraft aircraft;
	AircraftInput input;
	SDL_Event event;

	bool running = true;
	const float dt = 1.0f / 60.0f;

	aircraft_init(&aircraft);
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
		
		apply_input(&aircraft, &input, dt);
		update_aircraft(&aircraft, dt);

		clear_renderer();
		ddraw(&aircraft);
		renderer_present();
		SDL_Delay(16);
	}
	shutdown();

	return 0;
}