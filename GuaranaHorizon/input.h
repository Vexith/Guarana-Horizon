#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

typedef struct {
	float pitch, yaw, roll, throttle;
} AircraftInput;

void input_init(AircraftInput* input);
void input_update(AircraftInput* input, const SDL_Event* event);

#endif