#ifndef AI_BOT_H
#define AI_BOT_H

#include "aircraft.h"
#include "input.h"

typedef struct {
	Aircraft aircraft;
	AircraftInput input;

	float aggression;
	float skill;
	float reaction;

	float prev_yaw_error;
	float prev_pitch_error;
	float decision_timer;

	int state;
	Vec3 my_color;

	float fire_cooldown;
	int wants_to_fire;
} Bot;

void bot_init(Bot* bot, Vec3 spawn_pos, float yaw_deg, Vec3 color);
void bot_update(Bot* bot, const Aircraft* target, float dt);
void bot_spawnpoint(const Aircraft* aircraft, Bot* bots);

#endif