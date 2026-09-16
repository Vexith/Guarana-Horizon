#include "input.h"


void input_init(AircraftInput* input) {
	if (input == 0) return;

	input->pitch = 0.0f;
	input->yaw = 0.0f;
	input->roll = 0.0f;
	input->throttle = 0.0f;
}

void input_update(AircraftInput* input, const SDL_Event* event) {
	if (input == NULL || event == NULL) return;

	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.key) {
		case SDLK_W:
			input->pitch = -1.0f;
			break;
		case SDLK_S:
			input->pitch = 1.0f;
			break;
		case SDLK_A:
			input->yaw = -1.0f;
			break;
		case SDLK_D:
			input->yaw = 1.0f;
			break;
		case SDLK_E:
			input->roll = -1.0f;
			break;
		case SDLK_Q:
			input->roll = 1.0f;
			break;
		case SDLK_R:
			input->throttle = 1.0f;
			break;
		case SDLK_F:
			input->throttle = -1.0f;
			break;
		}
	}

	if (event->type == SDL_EVENT_KEY_UP) {
		switch (event->key.key) {
		case SDLK_W:
		case SDLK_S:
			input->pitch = 0.0f;
			break;
		case SDLK_A:
		case SDLK_D:
			input->yaw = 0.0f;
			break;
		case SDLK_Q:
		case SDLK_E:
			input->roll = 0.0f;
			break;
		case SDLK_R:
		case SDLK_F:
			input->throttle = 0.0f;
			break;
		}
	}
}

