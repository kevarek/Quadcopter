/*
*Filename: utility.c
*Author: Stanislav Subrt
*Year: 2014
*/


#include "utility.h"
#include "main.h"


//Converts value in degrees to radians
inline float util_DegToRad(float deg){
	return deg * 0.0174532925;		//2*PI*1RADIAN = 360deg
}


//Converts value in radians to degrees
inline float util_RadToDeg(float rad){
	return rad * 57.2957795;		//2*PI*1RADIAN = 360deg
}


//////////////////////////////////////////////////////////////////////
////////// EXPONENTIAL MOVING AVERAGE ALGORITHM //////////////////////
//////////////////////////////////////////////////////////////////////

//Calculates exponential moving average
//Output = (1-Alpha) * value + Alpha * PrevValue;
//Constant Alpha = exp( -T / Tau );
//Time constant Tau = -T / ln( Alpha );
//Sampling period T = -Tau * ln( Alpha );
inline float util_ExpMovAvg(float alpha, float prevVal, float newVal){
	return (1-alpha) * newVal + alpha * prevVal;
}


//////////////////////////////////////////////////////////////////////
////////// PID REGULATOR /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//Calculates action value coming from PID algorithm with simple proportional feed forwarding.
//u =
inline float util_UpdatePid(util_PidTd *h, float desired, float actual){
	float Err, Dif, Integral, Output;

	if( h == 0 ){
		return 0;
	}

	Err = desired - actual;
	Dif = Err - h->PrevErr;
	Integral = h->Integral + Err;

	switch( h->AntiWindupMode ){
		case util_PidAntiWindupModeTd_Sat:

			Output = h->FF*desired + h->P*Err + h->I*Integral + h->D*Dif;

			if( Output < h->ModeLim[0] ){
				Output = h->ModeLim[0];
			}
			else if( Output > h->ModeLim[1] ){
				Output = h->ModeLim[1];
			}
			h->PrevErr = Err;
			//h->Integral = Integral; //Integral value is not updated in output saturation antiwindup mode
		break;

		case util_PidAntiWindupModeTd_IntLim:

			if( Integral < h->ModeLim[0] ){
				Integral = h->ModeLim[0];
			}
			else if( Integral > h->ModeLim[1] ){
				Integral = h->ModeLim[1];
			}

			Output = h->FF*desired + h->P*Err + h->I*Integral + h->D*Dif;

			h->PrevErr = Err;
			h->Integral = Integral;
		break;

		default:
			Output = h->FF*desired + h->P*Err + h->I*Integral + h->D*Dif;
			h->PrevErr = Err;
			h->Integral = Integral;
		break;
	}

	return Output;
}


inline void util_InitPid(util_PidTd *h, float ff, float pp, float ii, float dd, util_PidAntiWindupModeTd m, float ModeLimMin, float ModeLimMax){
	if( h == 0 ){
		return;
	}

	h->FF = ff;
	h->P = pp;
	h->I = ii;
	h->D = dd;
	h->AntiWindupMode = m;
	h->ModeLim[0] = ModeLimMin;
	h->ModeLim[1] = ModeLimMax;
	h->Integral = 0;
	h->PrevErr = 0;
}


inline void util_ResetPid(util_PidTd *h){
	if( h == 0 ){
		return;
	}

	h->Integral = 0;
	h->PrevErr = 0;
}


//////////////////////////////////////////////////////////////////////
////////// DYNAMIC MEMORY ALLOCATION /////////////////////////////////
//////////////////////////////////////////////////////////////////////

//This is sbrk implementation which stops hardfault each time when sprintf with float param is called
caddr_t _sbrk(int incr){
	extern char end asm("end");
	static char *heap_end;
	char *prev_heap_end;

	if (heap_end == 0){
		heap_end = &end;
	}
	prev_heap_end = heap_end;
	heap_end += incr;
	return (caddr_t)prev_heap_end;
}

