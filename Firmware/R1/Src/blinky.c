/*
*Filename: blinky.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "blinky.h"
#include "stm32f0xx.h"
#include "main.h"
#include "sensors.h"
#include "comm.h"


void blinky_Init(void){
    NVIC_InitTypeDef NVIC_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;


    /* Enable the TIM3 gloabal Interrupt */
    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);


    GPIO_InitStruct.GPIO_Pin  = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    GPIO_Init(GPIOA, &GPIO_InitStruct);

	//1ms update period
    TIM_TimeBaseInitStruct.TIM_Prescaler = 999;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period = 48;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}


//////////////////////////////////////////////////////////////////////
////////// LED NOTIFICATION //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


static int NotificationPeriodMs = 0;
void blinky_SetNotificationPeriod(int newPeriodMs){
	static int PreviousPeriod = 2000;
	if( newPeriodMs <0 ){
		newPeriodMs = PreviousPeriod;
	}
	else{
		PreviousPeriod = NotificationPeriodMs;
	}
	NotificationPeriodMs = newPeriodMs;
}

void NotificationEventCallback(void){
	static volatile int j = 0;
	if(j) GPIO_SetBits(GPIOA, GPIO_Pin_0);
	else GPIO_ResetBits(GPIOA, GPIO_Pin_0);
	j^=1;
}

////////////////////////////////////////////
////////////INTERNAL FUNCTIONS//////////////
////////////////////////////////////////////

void TIM3_IRQHandler(void){
	static volatile unsigned NotificationCounter = 0, SensorUpdateCounter = 0;

    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET){	// TIM3 update interrupt
		NotificationCounter++;
		SensorUpdateCounter++;

		if( SensorUpdateCounter >= ( SENSORS_SAMPLINGPERIOD_S * 1000 ) ){	//request update of sensors
			SensorUpdateCounter = 0;
			sensors_RequestUpdate();
		}


		if( (NotificationCounter >= NotificationPeriodMs) && (NotificationPeriodMs > 0) ){
			NotificationCounter = 0;
			NotificationEventCallback();
		}
	}

    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);				//clear status register

}

