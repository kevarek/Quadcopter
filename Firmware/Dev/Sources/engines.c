/*
*Filename: 	engines.c
*Author:	Stanislav Subrt
*Year: 		2014
*/

#include "engines.h"


#define ENG_ENPERCLK					__GPIOA_CLK_ENABLE();\
										__GPIOB_CLK_ENABLE();\
										__TIM3_CLK_ENABLE();


//With SYSCLK 72MHz, prescaler 20 and period 36000, PWM freq is 100Hz and in 1ms counter counts to 3600
#define ENG_TIM_INSTANCE				TIM3
#define ENG_TIM_PRESCALER				20
#define ENG_TIM_PERIOD					36000
#define ENG_TIM_CNTPERMS				3600


TIM_HandleTypeDef Timer;


eng_HandleTd eng_Front = 	{ &Timer,	TIM3,	TIM_CHANNEL_1,	GPIOA,	GPIO_PIN_6,	 GPIO_AF2_TIM3,	{0, 1} };
eng_HandleTd eng_Right = 	{ &Timer,	TIM3,	TIM_CHANNEL_2,	GPIOA,	GPIO_PIN_4,	 GPIO_AF2_TIM3,	{0, 1} };
eng_HandleTd eng_Left = 	{ &Timer,	TIM3,	TIM_CHANNEL_3,	GPIOB,	GPIO_PIN_0,	 GPIO_AF2_TIM3,	{0, 1} };
eng_HandleTd eng_Rear = 	{ &Timer,	TIM3,	TIM_CHANNEL_4,	GPIOB,	GPIO_PIN_1,	 GPIO_AF2_TIM3,	{0, 1} };


eng_HandleTd* eng_List[] = {
	&eng_Front,
	&eng_Right,
	&eng_Left,
	&eng_Rear,
};


util_ErrTd eng_Init(void){
	GPIO_InitTypeDef GPOInit;
	TIM_OC_InitTypeDef TimerCfg;
	int32_t i;

	ENG_ENPERCLK;

	//Initialize all GPIOs and their respective timers for engine control
	for( i=0; i<UTIL_SIZEOFARRAY(eng_List); i++ ){
		if( eng_List[i] != 0 && eng_List[i]->Port != 0 && eng_List[i]->TimHandle != 0 && eng_List[i]->TimInst != 0 ){
			GPOInit.Mode = GPIO_MODE_AF_PP;
			GPOInit.Pull = GPIO_NOPULL;
			GPOInit.Speed = GPIO_SPEED_LOW;
			GPOInit.Alternate = eng_List[i]->PinAF;
			GPOInit.Pin = eng_List[i]->Pin;
			HAL_GPIO_Init(eng_List[i]->Port, &GPOInit);

			//Timer freq is 72MHz / 20 = 3.6MHz, with period of 36000 reload happens with 100Hz frequency
			eng_List[i]->TimHandle->Instance = eng_List[i]->TimInst;
			eng_List[i]->TimHandle->Init.Prescaler = ENG_TIM_PRESCALER - 1;
			eng_List[i]->TimHandle->Init.Period = ENG_TIM_PERIOD - 1;
			eng_List[i]->TimHandle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
			eng_List[i]->TimHandle->Init.CounterMode = TIM_COUNTERMODE_UP;
			HAL_TIM_PWM_Init(eng_List[i]->TimHandle);

			//Start PWM generation with 1ms pulses for ESC (1ms pulse means 0% thrust, 2ms means full thrust. PWM freq is 100Hz.)
			TimerCfg.OCMode = TIM_OCMODE_PWM1;
			TimerCfg.OCPolarity = TIM_OCPOLARITY_HIGH;
			TimerCfg.OCFastMode = TIM_OCFAST_DISABLE;
			TimerCfg.Pulse = ENG_TIM_CNTPERMS;
			HAL_TIM_PWM_ConfigChannel(eng_List[i]->TimHandle, &TimerCfg, eng_List[i]->TimOutCh);

			HAL_TIM_PWM_Start(eng_List[i]->TimHandle, eng_List[i]->TimOutCh);

		}
	}

	eng_StopAll();
	return util_ErrTd_Ok;
}


//Set engine state (engine cant move if its state is different than "on")
util_ErrTd eng_SetState(eng_HandleTd *h, util_StateTd s){
	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( s == util_StateTd_On ){
		h->State = util_StateTd_On;
	}
	else{
		h->State = util_StateTd_Off;
	}

	return eng_SetThrottle(h, 0);
}


//Set engine throttle (engine cant move if its state is different than "on")
util_ErrTd eng_SetThrottle(eng_HandleTd *h, float throttle){

	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( throttle < h->ThrottleRange[0] ){
		throttle = h->ThrottleRange[0];
	}
	else if( throttle > h->ThrottleRange[1] ){
		throttle = h->ThrottleRange[1];
	}

	h->Throttle = throttle;

	if( h->State == util_StateTd_On ){
		__HAL_TIM_SetCompare(h->TimHandle, h->TimOutCh, (int16_t)(ENG_TIM_CNTPERMS + ( throttle * ENG_TIM_CNTPERMS )));
	}
	else{
		__HAL_TIM_SetCompare(h->TimHandle, h->TimOutCh, (int16_t)(ENG_TIM_CNTPERMS));
	}

	return util_ErrTd_Ok;
}


util_ErrTd eng_EnableAll(void){
	int32_t i;

	for( i=0; i<UTIL_SIZEOFARRAY(eng_List); i++ ){
		eng_SetState(eng_List[i], util_StateTd_On);
	}
	return util_ErrTd_Ok;
}


util_ErrTd eng_StopAll(void){
	int32_t i;

	for( i=0; i<UTIL_SIZEOFARRAY(eng_List); i++ ){
		eng_SetState(eng_List[i], util_StateTd_Off);
	}
	return util_ErrTd_Ok;
}
