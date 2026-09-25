#include <stdio.h>
#include <SDL3/SDL.h>
#include "model.h"
#include "aircraft.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"

int main(void) {
	Aircraft aircraft;
	AircraftInput input;
	SDL_Event event;
	Camera camera;

	bool running = true;
	const float dt = 1.0f / 60.0f;

	aircraft_init(&aircraft);
	
	Model* m = load_obj("C:\\Users\\Tomazini\\source\\repos\\GuaranaHorizon\\x64\\Release\\assets\\plane.obj", 50.0f);
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

	while (running) {
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			input_update(&input, &event);
		}
		
		aircraft_update(&aircraft, &input);

		follow_plane(&camera, &aircraft, dt);

		clear_renderer();
		draw_ground(&camera, aircraft.position);
		Vec3 my_color = color_hex("0x8000");
		draw_airplane(&aircraft, &camera, my_color);
		
		renderer_present();
		SDL_Delay(16);
	}
	shutdown();

	return 0;
}