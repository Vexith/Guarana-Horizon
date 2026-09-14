#include "renderer.h"
#include <math.h>
#include <SDL3/SDL.h>

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

#define WIDTH 1280
#define HEIGHT 720

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

void ddraw(const Aircraft* aircraft) {
	if (aircraft == NULL) return;

	float center_x = WIDTH * 0.5f;
	float center_y = HEIGHT * 0.5f;

	float size = 40.0f;

	float pitch = aircraft->rotation.x;
	float yaw = aircraft->rotation.y;
	float roll = aircraft->rotation.z;

	float nose_x = sinf(yaw) * size;
	float nose_y = -sinf(pitch) * size;

	float wing_x = cosf(roll) * size * 0.6f;
	float wing_y = sinf(roll) * size * 0.6f;

	SDL_FPoint nose;
	SDL_FPoint left;
	SDL_FPoint right;

	nose.x = center_x + nose_x;
	nose.y = center_y + nose_y;

	left.x = center_x - wing_x;
	left.y = center_y - wing_y;

	right.x = center_x + wing_x;
	right.y = center_y + wing_y;

	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderLine(renderer, nose.x, nose.y, left.x, left.y);
	SDL_RenderLine(renderer, left.x, left.y, right.x, right.y);
	SDL_RenderLine(renderer, right.x, right.y, nose.x, nose.y);

	/*SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderLine(renderer, 500, 300, 700, 300);
	SDL_RenderLine(renderer, 700, 300, 500, 400);
	SDL_RenderLine(renderer, 500, 400, 500, 300);*/
}