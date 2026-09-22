#include "input.h"


void input_init(AircraftInput* input) {
	if (input == 0) return;

	input->elevator = 0.0f;
	input->rudder = 0.0f;
	input->ailerion = 0.0f;
	input->thrust = 0.0f;
	input->timefac = 1.0f;
}

void input_update(AircraftInput* input, const SDL_Event* event) {
	if (input == NULL || event == NULL) return;

	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.key) {
		case SDLK_W:
			input->elevator = -1.0f;
			break;
		case SDLK_S:
			input->elevator = 1.0f;
			break;
		case SDLK_A:
			input->rudder = -1.0f;
			break;
		case SDLK_D:
			input->rudder = 1.0f;
			break;
		case SDLK_E:
			input->ailerion = -1.0f;
			break;
		case SDLK_Q:
			input->ailerion = 1.0f;
			break;
		case SDLK_R:
			input->thrust = 1.0f;
			break;
		case SDLK_F:
			input->thrust = -1.0f;
			break;
		}
	}

	if (event->type == SDL_EVENT_KEY_UP) {
		switch (event->key.key) {
		case SDLK_W:
		case SDLK_S:
			input->elevator = 0.0f;
			break;
		case SDLK_A:
		case SDLK_D:
			input->rudder = 0.0f;
			break;
		case SDLK_Q:
		case SDLK_E:
			input->ailerion = 0.0f;
			break;
		case SDLK_R:
		case SDLK_F:
			input->thrust = 0.0f;
			break;
		}
	}
}

