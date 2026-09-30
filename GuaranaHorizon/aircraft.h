#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <math.h>
#include "quaternion.h"
#include "vector.h"
#include "input.h"
#include "model.h"

typedef struct {
	Vec3 forward, right, up;
} AircraftBasis;

typedef struct {
	float respawn_timer;
	Vec3 spawn_position;
	float spawn_yaw;
	Model* model;
	Vec3 position;
	Quaternion orientation;
	Vec3 velocity;
	Vec3 angular_velocity;
	float elevator_smooth;
	float ailerion_smooth;
	float rudder_smooth;

	float gamma;
	float theta;
	float phi;

	float thrust;
	float elevatoreffect;
	float rolleffect;
	float ruddereffect;
	float FlapsLevel;
	int UndercarriageLevel;
	int SpeedBrakeLevel;
	float recthrust;

	float accx, accy, accz;
	float forcex, forcey, forcez;
	float realspeed;
	float DragEffect;

	float maxthrust;
	float manoeverability;
	float RollRate;
	float maxgamma;
	float maxtheta;
	float inertia;
	float deadweight;

	float StallSpeed;
	float DiveSpeedLimit1;
	float DiveSpeedStructuralLimit;
	float SeaLevelSpeedLimitThreshold;
	float SpeedBrakePower;

	float SpeedHistoryArray[10];
	int SpeedHistoryIdx;
	float InertiallyDampenedPlayerSpeed;

	float AirDensityDrag;
	float GammaDrag;
	float LoopedBeyondVerticalDrag;
	float SpeedBeyondStructuralLimitsDrag;
	float FlapDrag;
	float UndercarriageDrag;
	float SpeedBrakeDrag;
	float RegulatedForceX;
	float RegulatedForceY;
	float RegulatedForceZ;

	float durability;
	float max_durability;
	int alive;
	float damage_flash;

} Aircraft;

void aircraft_init(Aircraft* a);
void aircraft_update(Aircraft* a, const AircraftInput* in);
void get_basis(const Aircraft* a, Vec3* fwd, Vec3* rgt, Vec3* up);
void get_euler(const Aircraft* a, float* gamma, float* theta, float* phi);

void set_model(Aircraft* a,
	float maxthrust, float manoeverability, float RollRate,
	float maxgamma, float maxtheta, float inertia,
	float deadweight, float StaticDrag,
	float StallSpeed, float DiveSpeedLimit1, float DiveSpeedStructuralLimit,
	float SeaLevelSpeedLimitThreshold,
	float CompressibilitySpeed, float CompressibilitySpeedWithSpeedBrakes,
	float MaxFullPowerAltRatio, float ServiceCeilingAltitude,
	float FlapSpeed, float SpeedBrakePower);
void aircraft_apply_damage(Aircraft* air, float dmg);
void aircraft_respawn(Aircraft* air);

#endif 