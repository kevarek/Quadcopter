/*
*Filename: engines.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef ENGINES_H
#define ENGINES_H

#include "main.h"

typedef struct {
	TIM_HandleTypeDef* TimHandle;
	TIM_TypeDef* TimInst;
	uint32_t TimOutCh;
	GPIO_TypeDef* Port;
	uint16_t Pin;
	uint16_t PinAF;
	float ThrottleRange[2];

	util_StateTd State;
    float Throttle;
} eng_HandleTd;


extern eng_HandleTd eng_Front;
extern eng_HandleTd eng_Right;
extern eng_HandleTd eng_Left;
extern eng_HandleTd eng_Rear;


extern util_ErrTd eng_Init(void);

extern util_ErrTd eng_SetState(eng_HandleTd *h, util_StateTd s);
extern util_ErrTd eng_SetThrottle(eng_HandleTd *h, float throttle);

extern util_ErrTd eng_EnableAll(void);
extern util_ErrTd eng_StopAll(void);

#endif  //end of ENGINES_H
