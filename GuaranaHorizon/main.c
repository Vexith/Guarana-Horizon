#include <stdio.h>
#include "aircraft.h"
#include "input.h"

int main(void) {
	Aircraft aircraft;
	AircraftInput input;

	const float dt = 1.0f / 60.0f;

	aircraft_init(&aircraft);
	input_init(&input);
	
	input.throttle = 1.0f;

	for (int frame = 0; frame < 600; frame++) {
		update_aircraft(&aircraft, dt);
		if (frame % 60 == 0) {
			printf("Time: %.1f s\n", frame *dt);
			printf("Position: X %.2f | Y %.2f | Z %.2f\n",aircraft.position.x, aircraft.position.y, aircraft.position.z);
			printf("Velocity: X %.2f |  Y %.2f | Z: %.2f\n", aircraft.velocity.x, aircraft.velocity.y, aircraft.velocity.z);
			printf("Rotation: X %.2f |  Y %.2f | Z: %.2f\n", aircraft.rotation.x, aircraft.rotation.y, aircraft.rotation.z);
		}
	}
}