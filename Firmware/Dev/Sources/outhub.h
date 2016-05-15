/*
*Filename: 	outhub.h
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

/* DESCRIPTION

*/


#ifndef	OUTHUB_H
#define OUTHUB_H

#include "outhub.h"
#include "main.h"
#include "utility.h"



typedef struct{
	char* Name;
	util_ErrTd (*SetStateFn)(util_StateTd state);
	util_ErrTd (*SetModeFn)(util_OutModeTd mode, float param1, float param2);
	util_ErrTd (*SetValFn)(float val);
	float OutputRange[2];

	util_StateTd State;
	util_OutModeTd Mode;
	float Value;
}oh_HandleTd;


extern util_ErrTd oh_Init(void);
extern util_ErrTd oh_SetMode(oh_HandleTd* h, util_OutModeTd newMode, float param1, float param2);
extern util_ErrTd oh_GetMode(oh_HandleTd* h, util_OutModeTd* m);
extern util_ErrTd oh_SetState(oh_HandleTd* h, util_StateTd newState);
extern util_ErrTd oh_SetVal(oh_HandleTd* h, float newVal);
extern util_ErrTd oh_GetVal(oh_HandleTd* h, float* val);
extern util_ErrTd oh_SetRange(oh_HandleTd* h, float newMin, float newMax);
extern util_ErrTd oh_GetRange(oh_HandleTd* h, float* retMin, float* retMax);

#endif	//OUTHUB_H
