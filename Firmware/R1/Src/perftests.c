/*
*Filename: perftests.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "stm32f0xx.h"
#include "perftests.h"

#include "comm.h"

void perftests_Init(void){
    NVIC_InitTypeDef NVIC_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;


    /* Enable the TIM14 gloabal Interrupt */
    NVIC_InitStruct.NVIC_IRQChannel = TIM14_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);


    TIM_TimeBaseInitStruct.TIM_Prescaler = 9;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period = 0xFFFF;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
    TIM_TimeBaseInit(TIM14, &TIM_TimeBaseInitStruct);
    TIM_ITConfig(TIM14, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM14, DISABLE);
}

int TestRunning = 0;
void perftests_StartTimer(int id){
	if (TestRunning){
		return;
	}
	TestRunning = id;
	TIM_SetCounter(TIM14, 0);
	TIM_Cmd(TIM14, ENABLE);
}

int perftests_ReadTimer(void){
	volatile int v;

	v =  TIM_GetCounter(TIM14);
	TIM_Cmd(TIM14, DISABLE);
	v++;
	v--;
	TestRunning = 0;
	return v;
}


//////////////////////////////////////
//////INTERNAL FUNCTIONS//////////////
//////////////////////////////////////
//
void TIM14_IRQHandler(void){
	volatile int i = 0;
    if (TIM_GetITStatus(TIM14, TIM_IT_Update) == SET){	// TIM14 update interrupt
		i++;
	}
    TIM_ClearITPendingBit(TIM14, TIM_IT_Update);				//clear status register
}

