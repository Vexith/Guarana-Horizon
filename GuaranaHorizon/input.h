#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>

typedef struct {
	float elevator;
	float ailerion;
	float rudder;
	float thrust;
	float FlapsLevel;
	int UndercarriageLevel;
	int SpeedBrakeLevel;
	float timefac;
} AircraftInput;

void input_init(AircraftInput* input);
void input_update(AircraftInput* input, const SDL_Event* event);

#endif