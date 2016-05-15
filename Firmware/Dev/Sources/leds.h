/*
*Filename: 	leds.h
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

/* DESCRIPTION

*/

#ifndef LEDS_H
#define LEDS_H

#include "main.h"


typedef struct{
	GPIO_TypeDef* Port;
	uint16_t Pin;
}leds_HandleTd;


extern leds_HandleTd GreenLed;

extern util_ErrTd leds_Init(void);
extern util_ErrTd leds_Set(leds_HandleTd* h, util_StateTd newState);

#endif  //end of LEDS_H
