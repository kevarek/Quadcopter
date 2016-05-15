/*
*Filename: stabilisation.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef STABILISATION_H
#define STABILISATION_H

#include "psd.h"
#include "engines.h"

//#define STABILISATION_MINTHRUST	0
//#define STABILISATION_MAXTHRUST	100000
//#define STABILISATION_THRUSTRANGE	( STABILISATION_MAXTHRUST - STABILISATION_MINTHRUST )


extern PSDStructTypedef                    PitchPSDStruct, RollPSDStruct, YawPSDStruct;


extern void stabilisation_Init(void);
extern void stabilisation_EmergencyStop(void);
extern void stabilisation_CancelEmergencyStop(void);



extern void stabilisation_RequestUpdate(void);
extern void stabilisation_HandleStabilisation(void);

extern void stabilisation_SetDesiredThrust(int newVal);
extern void stabilisation_SetDesiredPitchAngle(float newVal);
extern void stabilisation_SetDesiredRollAngle(float newVal);
extern void stabilisation_SetDesiredYawAngle(float newVal);
extern void stabilisation_IncrementDesiredYawAngle(float valToInc);


void stabilisation_Reset(void);

#endif  //end of STABILISATION_H
