#include "input.h"

void input_init(AircraftInput* input) {
	input->pitch = 0.0f;
	input->yaw = 0.0f;
	input->roll = 0.0f;
	input->throttle = 0.0f;
}