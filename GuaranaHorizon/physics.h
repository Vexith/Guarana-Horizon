#ifndef PHYSICS_H
#define PHYSICS_H

#include "aircraft.h"

int collcheck_ground(Aircraft* air);
int collcheck_aircraft(Aircraft* air, Aircraft* air2);

#endif