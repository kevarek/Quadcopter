
/*
*Filename: engines.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "engines.h"


#define TIM2_PERIOD     480000
#define TIM2_1MS_CNT    48000


#define ENG1PIN GPIO_Pin_4
#define ENG2PIN GPIO_Pin_5
#define ENG3PIN GPIO_Pin_6
#define ENG4PIN GPIO_Pin_7
#define ENGPINS ( ENG1PIN | ENG2PIN | ENG3PIN | ENG4PIN )
#define ENGPORT GPIOA
#define ENGPORT_PERIPHCLK	RCC_AHBPeriph_GPIOA

EngineStructTypedef Eng1, Eng2, Eng3, Eng4;


//////////////////////////////////////////////
/////////SIMPLE ENGINE CONTROL////////////////
//////////////////////////////////////////////


void eng_Init(void){
    NVIC_InitTypeDef NVIC_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    //enable the TIM2 gloabal Interrupt
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    //GPIO init
    RCC_AHBPeriphClockCmd(ENGPORT_PERIPHCLK, ENABLE);
    //setup GPIO pin for Eng1
    GPIO_InitStruct.GPIO_Pin  = ENG1PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(ENGPORT, &GPIO_InitStruct);
    //setup GPIO pin for Eng2
    GPIO_InitStruct.GPIO_Pin  = ENG2PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(ENGPORT, &GPIO_InitStruct);
    //setup GPIO pin for Eng3
    GPIO_InitStruct.GPIO_Pin  = ENG3PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(ENGPORT, &GPIO_InitStruct);
    //setup GPIO pin for Eng4
    GPIO_InitStruct.GPIO_Pin  = ENG4PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(ENGPORT, &GPIO_InitStruct);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    //timer init - 100hz, 1ms takes 48000
    TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_Period = TIM2_PERIOD;     //10ms autoreload = 100hz
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);

    //configure 1ms pulses for all four compares - 1ms pulses with 100Hz will start ESC
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_Pulse = TIM2_1MS_CNT;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);

    //enable specified interrupts for TIM2 and enable timer ---- engines are running now :)
    TIM_ITConfig(TIM2, TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    eng_InitEngineStructs();
    eng_Stop(); //stop command - starts ESC
}


//stops engines - not limited by min or max thrust value saturation
void eng_Stop(void){
    eng_SetEng1Thrust(0);
    eng_SetEng2Thrust(0);
    eng_SetEng3Thrust(0);
    eng_SetEng4Thrust(0);
}


void eng_SetThrust(EngineStructTypedef *eng, int newVal){
    //check saturation
    if ( newVal > ENGINES_MAXTHRUST ) newVal = ENGINES_MAXTHRUST;
    else if ( newVal < ENGINES_MINTHRUST ) newVal = ENGINES_MINTHRUST;

    eng->SetThrustFn(newVal);
}

int eng_GetThrust(EngineStructTypedef *eng){
    return eng->ActualThrust;
}


////////////////////////////////////////////////////
/////////Private functions//////////////////////////
////////////////////////////////////////////////////

void eng_InitEngineStructs(void){
    Eng1.ActualThrust = 0;
    Eng1.SaturationMin = ENGINES_MINTHRUST;
    Eng1.SaturationMax = ENGINES_MAXTHRUST;
    Eng1.SetThrustFn = eng_SetEng1Thrust;

    Eng2.ActualThrust = 0;
    Eng2.SaturationMin = ENGINES_MINTHRUST;
    Eng2.SaturationMax = ENGINES_MAXTHRUST;
    Eng2.SetThrustFn = eng_SetEng2Thrust;

    Eng3.ActualThrust = 0;
    Eng3.SaturationMin = ENGINES_MINTHRUST;
    Eng3.SaturationMax = ENGINES_MAXTHRUST;
    Eng3.SetThrustFn = eng_SetEng3Thrust;

    Eng4.ActualThrust = 0;
    Eng4.SaturationMin = ENGINES_MINTHRUST;
    Eng4.SaturationMax = ENGINES_MAXTHRUST;
    Eng4.SetThrustFn = eng_SetEng4Thrust;
}


void eng_SetEng1Thrust(int newVal){
    unsigned NewCCRValue = TIM2_1MS_CNT + (int)( ((float)TIM2_1MS_CNT / (float)ENGINES_THRUSTRANGE) * (float)newVal );
    TIM_SetCompare1(TIM2, NewCCRValue);
    Eng1.ActualThrust = newVal;
}


void eng_SetEng2Thrust(int newVal){
    unsigned NewCCRValue = TIM2_1MS_CNT + (int)( ((float)TIM2_1MS_CNT / (float)ENGINES_THRUSTRANGE) * (float)newVal );
    TIM_SetCompare2(TIM2, NewCCRValue);
    Eng2.ActualThrust = newVal;
}

void eng_SetEng3Thrust(int newVal){
    unsigned NewCCRValue = TIM2_1MS_CNT + (int)( ((float)TIM2_1MS_CNT / (float)ENGINES_THRUSTRANGE) * (float)newVal );
    TIM_SetCompare3(TIM2, NewCCRValue);
    Eng3.ActualThrust = newVal;
}


void eng_SetEng4Thrust(int newVal){
    unsigned NewCCRValue = TIM2_1MS_CNT + (int)( ((float)TIM2_1MS_CNT / (float)ENGINES_THRUSTRANGE) * (float)newVal );
    TIM_SetCompare4(TIM2, NewCCRValue);
    Eng4.ActualThrust = newVal;
}



void TIM2_IRQHandler(void){

	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) == SET){
		GPIO_ResetBits(ENGPORT, ENG1PIN);
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);				//clear status register
	}

	if (TIM_GetITStatus(TIM2, TIM_IT_CC2) == SET){
		GPIO_ResetBits(ENGPORT, ENG2PIN);
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);				//clear status register
	}

	if (TIM_GetITStatus(TIM2, TIM_IT_CC3) == SET){
		GPIO_ResetBits(ENGPORT, ENG3PIN);
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC3);				//clear status register
	}

	if (TIM_GetITStatus(TIM2, TIM_IT_CC4) == SET){
		GPIO_ResetBits(ENGPORT, ENG4PIN);
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);				//clear status register
	}

	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET){
		GPIO_SetBits(ENGPORT, ENGPINS);
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);				//clear status register
	}
}
