#include "ai_bot.h"
#include "vector.h"
#include <math.h>
#include <string.h>

#define BOT_MAX_AILERION 0.7f
#define BOT_MAX_ELEVATOR 0.8f
#define BOT_MIN_DIST 15.0f
#define BOT_FIRE_DIST 60.0f
#define BOT_FIRE_ANGLE 0.25f

void bot_init(Bot* bot, Vec3 spawn_pos, float yaw_deg, Vec3 color) {
	if (!bot) return;
	memset(bot, 0, sizeof(*bot));

	aircraft_init(&bot->aircraft);
	bot->aircraft.position = spawn_pos;

	bot->aircraft.orientation = q_from_axangle((Vec3){0.0f, 1.0f, 0.0f}, yaw_deg * D2R + PI);
	Vec3 basis_fwd, basis_rgt, basis_up;
	get_basis(&bot->aircraft, &basis_fwd, &basis_rgt, &basis_up);
	bot->aircraft.velocity = scale(basis_fwd, 0.5f);
	bot->aircraft.realspeed = 0.5f;

	input_init(&bot->input);
	bot->input.thrust = bot->aircraft.maxthrust * 0.85f;
	bot->input.timefac = 1.0f;

	bot->aggression = 0.75f;
	bot->skill = 0.65f;
	bot->reaction = 0.12f;
	bot->state = 1;
	bot->my_color = color;

	bot->prev_yaw_error = 0.0f;
	bot->prev_pitch_error = 0.0f;
	bot->decision_timer = 0.0f;
	bot->aircraft.spawn_position = spawn_pos;
	bot->aircraft.spawn_yaw = yaw_deg;
	bot->aircraft.respawn_timer = 0.0f;

	bot->fire_cooldown = 0.0f;
	bot->wants_to_fire = 0;
}
void bot_spawnpoint(const Aircraft* aircraft, Bot* bots) {
	Vec3 p_fwd, p_rgt, p_up;
	
	Vec3 bot_colors[3] = {
	{0.85f, 0.15f, 0.15f},
	{0.15f, 0.75f, 0.15f},
	{0.85f, 0.65f, 0.15f},
	};
	get_basis(aircraft, &p_fwd, &p_rgt, &p_up);

	float lateral[3] = { -35.0f, 0.0f, 35.0f };
	float dist_ahead = 70.0f;
	float offset_height = 10.0f;

	for (int i = 0; i < 3; i++) {
		Vec3 spawn = aircraft->position;
		spawn = add(spawn, scale(p_fwd, dist_ahead));
		spawn = add(spawn, scale(p_rgt, lateral[i]));
		spawn.y += offset_height;

		Vec3 to_player = sub(aircraft->position, spawn);
		float yaw_to_player = atan2f(-to_player.x, -to_player.z) * (180.0f / PI);

		yaw_to_player -= 180.0f;
		bot_init(&bots[i], spawn, yaw_to_player, bot_colors[i]);
	}

	
}
void bot_update(Bot* bot, const Aircraft* target, float dt) {
	if (!bot || !target) return;

	Aircraft* a = &bot->aircraft;
	if (!a->alive) return;

	bot->input.timefac = dt * 60.0f;
	bot->wants_to_fire = 0;
	if (bot->fire_cooldown > 0.0f) bot->fire_cooldown -= dt;


	Vec3 basis_fwd, basis_rgt, basis_up;
	get_basis(&bot->aircraft, &basis_fwd, &basis_rgt, &basis_up);

	Vec3 to_target = sub(target->position, a->position);
	float dist = length(to_target);
	float dist_xz = sqrtf(to_target.x * to_target.x + to_target.z * to_target.z);

	float desired_ailerion = 0.0f;
	float desired_elevator = 0.0f;

	if (dist > 0.5f) {
		Vec3 dir_n = normalize(to_target);

		Vec3 fwd_h = { basis_fwd.x, 0.0f, basis_fwd.z };
		Vec3 tgt_h = { dir_n.x, 0.0f, dir_n.z };
		float yaw_error = 0.0f;

		if (length(fwd_h) > 0.01f && length(tgt_h) > 0.01f) {
			fwd_h = normalize(fwd_h);
			tgt_h = normalize(tgt_h);

			float dot_h = clampf(fwd_h.x * tgt_h.x + fwd_h.z * tgt_h.z, -1.0f, 1.0f);
			yaw_error = acosf(dot_h);

			float cross_y = fwd_h.z * tgt_h.x - fwd_h.x * tgt_h.z;
			if (cross_y < 0.0f) yaw_error = -yaw_error;
		}

		float tgt_pitch = asinf(clampf(dir_n.y, -1.0f, 1.0f));
		float bot_pitch = asinf(clampf(basis_fwd.y, -1.0f, 1.0f));
		float pitch_error = tgt_pitch - bot_pitch;

		float d_yaw = (yaw_error - bot->prev_yaw_error) / (dt > 0.001f ? dt : 0.016f);
		float d_pitch = (pitch_error - bot->prev_pitch_error) / (dt > 0.001f ? dt : 0.016f);

		float kp_yaw = 1.6f;
		float kd_yaw = 0.35f;
		float kd_pitch = 0.5f;
		float kp_pitch = 2.2f;

		desired_ailerion = kp_yaw * yaw_error + kd_yaw * d_yaw;
		desired_elevator = -(kp_pitch * pitch_error + kd_pitch * d_pitch);

		desired_ailerion = clampf(desired_ailerion, -BOT_MAX_AILERION, BOT_MAX_AILERION);
		desired_elevator = clampf(desired_elevator, -BOT_MAX_ELEVATOR, BOT_MAX_ELEVATOR);

		if (dist < BOT_MIN_DIST) {
			desired_elevator = 0.5f;
			desired_ailerion = 0.0f;
		}

		if (a->realspeed < a->StallSpeed * 1.3f) {
			desired_elevator = clampf(desired_elevator + 0.3f, -1.0f, BOT_MAX_ELEVATOR);
		}

		bot->prev_yaw_error = yaw_error;
		bot->prev_pitch_error = pitch_error;
	}

	bot->input.elevator = desired_elevator * bot->skill;
	bot->input.ailerion = desired_ailerion * bot->skill;
	bot->input.rudder = 0.0f;

	if (dist > 80.0f) {
		bot->input.thrust = a->maxthrust;
	}
	else if (dist < 20.0f) {
		bot->input.thrust = a->maxthrust * 0.6f;
	}
	else {
		bot->input.thrust = a->maxthrust * 0.9f;
	}

	if (dist < 80.0f && dist > 15.0f && bot->fire_cooldown <= 0.0f) {
		Vec3 dir_to_target = normalize(to_target);
		float dot_fwd = dot(basis_fwd, dir_to_target);
		if (dot_fwd > 0.98f) {
			bot->wants_to_fire = 1;
			bot->fire_cooldown = 0.15f;
		}
	}

	aircraft_update(a, &bot->input);
}