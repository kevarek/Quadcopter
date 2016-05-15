/*
*Filename: sensorhub.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef SENSORHUB_H
#define SENSORHUB_H

#include <stdint.h>

typedef struct{
	uint8_t* Name;
	float (*GetDataFn)(void);	//Pointer to function which obtains data
	float Scale;				//Coefficient to convert data into value
	float Offset;				//Offset to substract from value
	float Alpha;				//Filter parameter (see exponential filter)
	float IntPeriodMs;			//Integration time (period)

	float Val;					//Latest calculated value
	float FilteredVal;			//Filtered value
	float FilteredValSpeed;		//Speed of value change during integration period
	float FilteredValMin;		//Minimum filtered value
	float FilteredValMax;		//Maximum filtered value
	float IntPrevFilteredVal;	//Value of previous reading
	float IntPrevTime;			//Timestamp of previous reading
}sh_HandleTd;


extern void sh_Init(void);

extern float sh_GetVal(sh_HandleTd *h);
extern void sh_SetScale(sh_HandleTd *h, float val);
extern void sh_SetOffset(sh_HandleTd *h, float val);
extern void sh_SetZero(sh_HandleTd *h);

extern float sh_GetFilteredVal(sh_HandleTd *h);

extern void sh_SetAplha(sh_HandleTd *h, float val);
extern void sh_SetTimeConstant(sh_HandleTd *h, float val);

#endif  //end of SENSORHUB_H
