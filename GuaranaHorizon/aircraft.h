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
	float StaticDrag;
	float StallSpeed;
	float DiveSpeedLimit1;
	float DiveSpeedStructuralLimit;
	float SeaLevelSpeedLimitThreshold;
	float CompressibilitySpeed;
	float CompressibilitySpeedWithSpeedBrakes;
	float MaxFullPowerAltRatio;
	float ServiceCeilingAltitude;
	float SpeedBrakePower;
	float ClipDistance;
	float BlackoutSensitivity;
	float RedoutSensitivity;

	float FlapSpeed;
	float FlapsLevelElevatorEffect0;
	float FlapsLevelElevatorEffect1;
	float FlapsLevelElevatorEffect2;
	float FlapsLevelElevatorEffect3;
	float FlapsLevelElevatorEffect4;

	int OnTheGround;
	int WepCapable;

	float SpeedHistoryArray[10];
	int SpeedHistoryIdx;
	float InertiaTimer;
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
#endif 