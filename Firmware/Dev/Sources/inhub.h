/*
*Filename: inhub.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef INHUB_H
#define INHUB_H

#include "main.h"


typedef struct{
	char* Name;
	float (*GetDataFn)(void);	//Pointer to function which obtains data

	float Scale;				//Coefficient to convert data into value
	float Offset;				//Offset to substract from value
	float Alpha;				//Filter parameter (see exponential filter)
	float IntPeriodMs;			//Integration time (period)

	float Val;					//Latest calculated value
	float FilteredVal;			//Filtered value
	float FilteredValMin;		//Minimum filtered value
	float FilteredValMax;		//Maximum filtered value
}ih_HandleTd;



#define IH_MAX_SENSORS	16
extern ih_HandleTd* ih_SensorList[IH_MAX_SENSORS];


extern void ih_Init(void);


extern float ih_GetVal(ih_HandleTd *h);
extern void ih_SetScale(ih_HandleTd *h, float val);
extern float ih_GetScale(ih_HandleTd *h);
extern void ih_SetOffset(ih_HandleTd *h, float val);
extern float ih_GetOffset(ih_HandleTd *h);
extern void sens_SetZero(ih_HandleTd *h);

extern float ih_GetFilteredVal(ih_HandleTd *h);
extern float ih_GetFilteredValMax(ih_HandleTd *h);
extern float ih_GetFilteredValMin(ih_HandleTd *h);
extern void ih_ResetFilteredMinMax(ih_HandleTd *h);

extern void ih_SetAplha(ih_HandleTd *h, float val);
extern void ih_SetTimeConstant(ih_HandleTd *h, float val);
extern float ih_GetTimeConstant(ih_HandleTd *h);

extern char* ih_GetSensName(ih_HandleTd *h);
extern ih_HandleTd* ih_GetSensByName(char* n);


#endif  //end of INHUB_H
