/*
*Filename: 	leds.c
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

/* DESCRIPTION

*/

#include "leds.h"
#include "main.h"

#define LEDS_ENPERCLK	__GPIOB_CLK_ENABLE();


leds_HandleTd GreenLed = {GPIOB, GPIO_PIN_10};


leds_HandleTd* leds_List[] = {
	&GreenLed,
};


util_ErrTd leds_Init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	int32_t i;

	LEDS_ENPERCLK;
	for( i=0; i<UTIL_SIZEOFARRAY(leds_List); i++ ){
		if( leds_List[i] == 0 || leds_List[i]->Port == 0 ){
			continue;
		}
		else{
			GPIO_InitStructure.Pin = leds_List[i]->Pin;
			GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
			HAL_GPIO_Init(leds_List[i]->Port, &GPIO_InitStructure);
		}
	}
	return util_ErrTd_Ok;
}


util_ErrTd leds_Set(leds_HandleTd* h, util_StateTd newState){
	if( h == 0 || h->Port == 0 ){
		return util_ErrTd_Null;
	}
	else{
		HAL_GPIO_WritePin(h->Port, h->Pin, newState);
		return util_ErrTd_Ok;
	}
}
