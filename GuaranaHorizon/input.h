#ifndef INPUT_H
#define INPUT_H

typedef struct {
	float pitch, yaw, roll, throttle;
} AircraftInput;

void input_init(AircraftInput* input);

#endif