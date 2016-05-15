/*
*Filename: 	sensorhub.c
*Author: 	Stanislav Subrt
*Year:		2014
*/

#include "sensorhub.h"
#include "main.h"


#define TIM_INSTANCE				TIM7
#define TIM_ENPERCLK				__TIM7_CLK_ENABLE
#define TIM_IRQH					TIM7_IRQHandler
#define TIM_IRQN					TIM7_IRQn
#define TIM_PRESC					36
#define TIM_PERIOD					400


static TIM_HandleTypeDef Tmr;


//							Get data function						Scale	Offset	Alpha		Int.period
//sh_HandleTd AnalogIn1 = {	(uint8_t*)"a1", ain_GetAnalogIn1Data, 	1, 		0, 		0, 			100};


sh_HandleTd* SensorList[] = 	{

								};


void sh_Init(void){

	//Enable timer and setup periodic interrupt.
	TIM_ENPERCLK();
	Tmr.Instance = TIM_INSTANCE;
	Tmr.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	Tmr.Init.CounterMode = TIM_COUNTERMODE_UP;
	Tmr.Init.Period = TIM_PERIOD - 1;
	Tmr.Init.Prescaler = TIM_PRESC - 1;
	HAL_TIM_Base_Init(&Tmr);

	HAL_NVIC_SetPriority(TIM_IRQN, 1, 3);
	HAL_NVIC_EnableIRQ(TIM_IRQN);
	__HAL_TIM_ENABLE_IT(&Tmr, TIM_IT_UPDATE);
	HAL_TIM_Base_Start(&Tmr);
}


float sh_GetVal(sh_HandleTd *h){
	float Val;

	if( ( h == 0 ) || ( h->GetDataFn == 0 ) ) {
		return NAN;
	}
	Val = h->GetDataFn() * h->Scale - h->Offset;
	return Val;
}


void sh_SetScale(sh_HandleTd *h, float val){

	if( h == 0 || UTIL_ISNAN(val) ){
		return;
	}

	h->Scale = val;
}


void sh_SetOffset(sh_HandleTd *h, float val){

	if( h == 0 || UTIL_ISNAN(val) ){
		return;
	}

	h->Offset = val;
}


void sh_SetZero(sh_HandleTd *h){

	if( h == 0 ){
		return;
	}

	h->Offset = h->Val;
}


//////////////////////////////////////////////////////////////////////
////////// FILTERING /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

float sh_GetFilteredVal(sh_HandleTd *h){

	if( h == 0 ){
		return NAN;
	}

	return h->FilteredVal;
}


void sh_SetAplha(sh_HandleTd *h, float val){

	if( h == 0 ){												//Check if pointer to value instance is valid
		return;
	}
	else if( ( val < 0 ) || ( val > 1 ) || UTIL_ISNAN(val) ){	//Check if new alpha value for exp mov avg filter is valid <0..1>
		return;
	}

	h->Alpha = val;
}


void sh_SetTimeConstant(sh_HandleTd *h, float val){
	float SamplingFrequency;

	if( h == 0 || ( val < 0 ) || ( val > 1 ) || UTIL_ISNAN(val) ){
		return;
	}

	SamplingFrequency = ( (float)SystemCoreClock / ( TIM_PRESC * TIM_PERIOD ) );
	h->Alpha = exp(  ( -1.0 / SamplingFrequency ) / val );							//alpha = exp( -samplperiod/timeconstant )
}

#define SENS_OPTIMISATIONx
#ifdef SENS_OPTIMISATION
float tes;
#endif

static void sh_UpdateAll(void){
	int i;
	float FilteredVal, Time, TimeDif, ValDif;
	sh_HandleTd *pSens;

#ifdef SENS_OPTIMISATION
	tes = bl_GetPreciseTick();
#endif

	Time = bl_GetPreciseTick();						//Save actual time
	for( i=0; i<UTIL_SIZEOFARRAY(SensorList); i++ ){
		pSens = SensorList[i];

		//Update value if possible
		if( pSens->GetDataFn != 0 ){
			pSens->Val = pSens->GetDataFn() * pSens->Scale - pSens->Offset;		//Calculate new value from data																//Save new value
		}
		//Update filtered value
		FilteredVal = util_ExpMovAvg( pSens->Alpha, pSens->FilteredVal, pSens->Val );
		pSens->FilteredVal = FilteredVal;
		//Store extremes
		if( FilteredVal > pSens->FilteredValMax ){
			pSens->FilteredValMax = FilteredVal;
		}

		if( FilteredVal < pSens->FilteredValMin ){
			pSens->FilteredValMin = FilteredVal;
		}

		//Update speed from filtered values
		TimeDif = Time - pSens->IntPrevTime;			//Get time difference from last cycle
		if( TimeDif >= pSens->IntPeriodMs ){			//Check if time difference from latest measuremen is greater than set integration time

			ValDif = pSens->FilteredVal - pSens->IntPrevFilteredVal;	//Get value difference
			pSens->FilteredValSpeed = 1000 * ValDif / TimeDif;					//Calculate speed [unit/s]


			pSens->IntPrevFilteredVal = pSens->FilteredVal;				//Save current filtered value for next calculation
			pSens->IntPrevTime = Time;											//Save current timestamp for next calculation
		}
	}
#ifdef SENS_OPTIMISATION
	tes = bl_GetPreciseTick() - tes;
#endif
}


//////////////////////////////////////////////////////////////////////
////////// TIMER INTERRUPT ROUTINE ///////////////////////////////////
//////////////////////////////////////////////////////////////////////

void TIM_IRQH(void){

	//If update interrupt is pending and update interrupt is enabled, clear update int flag, increment tick variable and if needed calculate cpu usage
	if( ( __HAL_TIM_GET_FLAG(&Tmr, TIM_FLAG_UPDATE) != RESET ) && ( __HAL_TIM_GET_ITSTATUS(&Tmr, TIM_IT_UPDATE) !=RESET ) ){
		__HAL_TIM_CLEAR_IT(&Tmr, TIM_IT_UPDATE);
		sh_UpdateAll();
	}
}
