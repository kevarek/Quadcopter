/*
*Filename: stabilisation.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef STABILISATION_H
#define STABILISATION_H

#include "main.h"
#include "quaternions.h"

#define ST_TASKPERIOD_MS				2
#define ST_MAHONYPERIOD_MS				10
#define ST_MAHONYFREQ_HZ				( 1000 / ST_MAHONYPERIOD_MS )


extern int32_t st_ManageSettings(int (*OpHandler)(void*, void*, unsigned), void *dest);

extern void st_SetDesiredThrust(float val);
extern void st_SetDesiredRoll(float val);
extern void st_SetDesiredPitch(float val);
extern void st_SetDesiredYaw(float val);

extern void st_IncrementMinThrust(float val);
extern float st_GetMinThrust(void);

extern void st_ResetRegulators(void);
extern void st_IncrementKp(float val);
extern void st_IncrementKi(float val);
extern void st_IncrementKd(float val);

extern void st_IncrementYawKp(float val);

//RTOS task which handles quadrocopters stabilisation
#include "cmsis_os.h"
extern osThreadId st_StabilityTaskId;
extern void st_StabilityTask(void const *argument);

#endif  //end of STABILISATION_H
