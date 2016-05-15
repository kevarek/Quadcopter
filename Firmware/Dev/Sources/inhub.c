/*
*Filename: 	inhub.c
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

/*
This module takes care
*/


#include "inhub.h"


#define TIM_INSTANCE				TIM7
#define TIM_ENPERCLK				__TIM7_CLK_ENABLE
#define TIM_IRQH					TIM7_IRQHandler
#define TIM_IRQN					TIM7_IRQn
#define TIM_PRESC					36					//2MHz counting
#define TIM_PERIOD					400					//Period=400 @ 2MHz ~ 2500Hz update rate


static TIM_HandleTypeDef Tmr;

//Sensor definitions	Name	GetDataFn	Scale	Offset	Alpha
ih_HandleTd Pitch = { "pitch", 	0,			1,		0,		0,		};
ih_HandleTd Roll = 	{ "roll", 	0,			1,		0,		0,		};
ih_HandleTd Yaw = 	{ "yaw", 	0,			1,		0,		0,		};
ih_HandleTd Bat = 	{ "bat", 	0,			1,		0,		0,		};



ih_HandleTd* ih_SensorList[IH_MAX_SENSORS] = 	{
		&Pitch,
		&Roll,
		&Yaw,
		&Bat,
	};


void ih_Init(void){
	//Enable timer and setup periodic interrupt.
	TIM_ENPERCLK();
	Tmr.Instance = TIM_INSTANCE;
	Tmr.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	Tmr.Init.CounterMode = TIM_COUNTERMODE_UP;
	Tmr.Init.Period = TIM_PERIOD - 1;
	Tmr.Init.Prescaler = TIM_PRESC - 1;
	HAL_TIM_Base_Init(&Tmr);

	HAL_NVIC_SetPriority(TIM_IRQN, 2, 3);
	HAL_NVIC_EnableIRQ(TIM_IRQN);
	__HAL_TIM_ENABLE_IT(&Tmr, TIM_IT_UPDATE);
	HAL_TIM_Base_Start(&Tmr);

}


float ih_GetVal(ih_HandleTd *h){
	float Val;

	if( ( h == 0 ) || ( h->GetDataFn == 0 ) ) {
		return NAN;
	}
	Val = h->GetDataFn() * h->Scale - h->Offset;
	return Val;
}


void ih_SetScale(ih_HandleTd *h, float val){
	if( h == 0 || UTIL_ISNAN(val) ){
		return;
	}
	h->Scale = val;
}

float ih_GetScale(ih_HandleTd *h){
	if( h == 0 ){
		return 0;
	}
	return h->Scale;
}

void ih_SetOffset(ih_HandleTd *h, float val){
	if( h == 0 || UTIL_ISNAN(val) ){
		return;
	}
	h->Offset = val;
}

float ih_GetOffset(ih_HandleTd *h){
	if( h == 0 ){
		return 0;
	}
	return h->Offset;
}


char* ih_GetSensName(ih_HandleTd *h){
	if( h == 0 ){
		return "null";
	}
	return h->Name;
}


//////////////////////////////////////////////////////////////////////
////////// FILTERING /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

float ih_GetFilteredVal(ih_HandleTd *h){
	if( h == 0 ){
		return NAN;
	}
	return h->FilteredVal;
}


float ih_GetFilteredValMax(ih_HandleTd *h){
	if( h == 0 ){
		return NAN;
	}
	return h->FilteredValMax;
}


float ih_GetFilteredValMin(ih_HandleTd *h){
	if( h == 0 ){
		return NAN;
	}
	return h->FilteredValMin;
}


void ih_ResetFilteredMinMax(ih_HandleTd *h){
	if( h == 0 ){
		return;
	}
	h->FilteredValMin = +INFINITY;
	h->FilteredValMax = -INFINITY;
}


void ih_SetAplha(ih_HandleTd *h, float val){
	if( h == 0 ){												//Check if pointer to value instance is valid
		return;
	}
	else if( ( val < 0 ) || ( val > 1 ) || UTIL_ISNAN(val) ){	//Check if new alpha value for exp mov avg filter is valid <0..1>
		return;
	}
	h->Alpha = val;
}


void ih_SetTimeConstant(ih_HandleTd *h, float valSec){
	float SamplingFrequency;

	if( h == 0 || ( valSec < 0 ) || ( valSec > 1 ) || UTIL_ISNAN(valSec) ){
		return;
	}
	SamplingFrequency = ( (float)SystemCoreClock / ( TIM_PRESC * TIM_PERIOD ) );
	h->Alpha = exp(  ( -1.0 / SamplingFrequency ) / valSec );							//alpha = exp( -samplperiod/timeconstant )
}


float ih_GetTimeConstant(ih_HandleTd *h){
	if( h == 0 ){
		return 0;
	}
	return (-1 * TIM_PRESC * TIM_PERIOD / (float)SystemCoreClock ) / log(h->Alpha);
}


static void ih_UpdateAll(void){
	int i;
	float FilteredVal;
	ih_HandleTd *pSens;


	for( i=0; i<UTIL_SIZEOFARRAY(ih_SensorList); i++ ){
		pSens = ih_SensorList[i];

		if( pSens == 0 ){
			continue;
		}
		//Update value if possible
		if( pSens->GetDataFn != 0 ){
			pSens->Val = pSens->GetDataFn() * pSens->Scale + pSens->Offset;		//Calculate new value from data
		}
		else{
			pSens->Val = 0;
		}

		//Update filtered value
		if( pSens->Alpha != 0 ){
			FilteredVal = util_ExpMovAvg( pSens->Alpha, pSens->FilteredVal, pSens->Val );
		}
		else{
			FilteredVal = pSens->Val;
		}
		pSens->FilteredVal = FilteredVal;

		//Store extremes
		if( FilteredVal > pSens->FilteredValMax ){
			pSens->FilteredValMax = FilteredVal;
		}
		if( FilteredVal < pSens->FilteredValMin ){
			pSens->FilteredValMin = FilteredVal;
		}
	}
}


ih_HandleTd* ih_GetSensByName(char* n){
	uint32_t i;

	for( i=0; i<UTIL_SIZEOFARRAY(ih_SensorList); i++ ){
		if( strcmp((char*)ih_SensorList[i]->Name, (char*)n) == 0 ){
			return ih_SensorList[i];
		}
	}
	return 0;
}



//////////////////////////////////////////////////////////////////////
////////// TIMER INTERRUPT ROUTINE ///////////////////////////////////
//////////////////////////////////////////////////////////////////////

void TIM_IRQH(void){

	//If update interrupt is pending and update interrupt is enabled, clear update int flag, increment tick variable and if needed calculate cpu usage
	if( ( __HAL_TIM_GET_FLAG(&Tmr, TIM_FLAG_UPDATE) != RESET ) && ( __HAL_TIM_GET_ITSTATUS(&Tmr, TIM_IT_UPDATE) !=RESET ) ){
		__HAL_TIM_CLEAR_IT(&Tmr, TIM_IT_UPDATE);
		ih_UpdateAll();
	}
}
