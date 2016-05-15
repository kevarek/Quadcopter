/*
*filename: psd.h
*author: Stanislav Subrt
*year: 2013
*/


#include "psd.h"

#define PSD_USE_SLIDING_AVERAGE

#ifndef PSD_USE_SLIDING_AVERAGE

void psd_Init(PSDStructTypedef *psd){
    psd->P = 0;
    psd->S = 0;
    psd->D = 0;

    psd->Period = 0;
    psd->DesiredValue = 0;
    psd->LastError = 0;
    psd->ErrorSum = 0;
    psd->SumSaturationMax = 0;
    psd->SumSaturationMin = 0;
    psd->OutputSaturationMax = 0;
    psd->OutputSaturationMin = 0;
}

void psd_Reset(PSDStructTypedef *psd){
    psd->DesiredValue = 0;
    psd->LastError = 0;
    psd->ErrorSum = 0;
}

void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d){
	if( p > 0) psd->P = p;
    if( s > 0) psd->S = s;
    if( d > 0) psd->D = d;
}

void psd_IncrementConstants(PSDStructTypedef *psd, float p, float s, float d){
	psd->P += p;
	psd->S += s;
	psd->D += d;

}
void psd_SetPeriod(PSDStructTypedef *psd, float newPeriodS){
	if( newPeriodS < 0 ) newPeriodS = 0;
	psd->Period = newPeriodS;
}

void psd_SetSumSaturation(PSDStructTypedef *psd, int satMin, int satMax){
    psd->SumSaturationMax = satMax;
    psd->SumSaturationMin = satMin;
}

void psd_SetOutputSaturation(PSDStructTypedef *psd, int satMin, int satMax){
    psd->OutputSaturationMax = satMax;
    psd->OutputSaturationMin = satMin;
}

void psd_SetDesiredValue(PSDStructTypedef *psd, float desiredValue){
    psd->DesiredValue = desiredValue;
}

float psd_UpdatePSD(PSDStructTypedef *psd, float actualValue){
    float PSDOutput, Error, ErrorSum, ErrorDifference;
    float p, s, d, Period;
    p = psd->P;
    s = psd->S;
    d = psd->D;



    Error = psd->DesiredValue - actualValue;	//error is difference between desired and actual value


    ErrorSum = psd->ErrorSum + Error;			//error sumation - just add to previous value
    //if(ErrorSum * Error < 0) ErrorSum = 0;		//clear errorsum when error and errorsum have different sign
    ErrorDifference = Error - psd->LastError;


	Period = psd->Period;

    //handle sum saturation
    if ( psd->SumSaturationMax == 0 && psd->SumSaturationMin == 0 ){
        //sum saturation not set
    }
    else{
        if( ErrorSum > (psd->SumSaturationMax) ) {
            ErrorSum = psd->SumSaturationMax;
        }
        else if( ErrorSum < (psd->SumSaturationMin) ) {
            ErrorSum = psd->SumSaturationMin;
        }
    }

    psd->ErrorSum = ErrorSum;
    psd->LastError = Error;

    PSDOutput = p * ( Error + ( ( (float)ErrorSum / s ) * Period ) + ( ( (float)ErrorDifference * d ) / Period ) );
    //PSDOutput = p * ( Error );

    //handle output saturation
    if ( psd->OutputSaturationMax == 0 && psd->OutputSaturationMin ==0 ) {
        //output saturation not set
    }
    else {
        if( PSDOutput > (psd->OutputSaturationMax) ) {
            PSDOutput = psd->OutputSaturationMax;
        }
        else if( PSDOutput < (psd->OutputSaturationMin) )  {
            PSDOutput = psd->OutputSaturationMin;
        }
    }
	psd->LastOutput = PSDOutput;
    return PSDOutput;
}

#else
void psd_Init(PSDStructTypedef *psd, slidavg_BufferStructTypedef* slidAvgBufStruct, float *floatBuffer, int  cnt){
    psd->P = 0;
    psd->S = 0;
    psd->D = 0;

    psd->Period = 0;
    psd->DesiredValue = 0;
    psd->LastError = 0;
    psd->ErrorSum = 0;
    psd->SumSaturationMax = 0;
    psd->SumSaturationMin = 0;
    psd->OutputSaturationMax = 0;
    psd->OutputSaturationMin = 0;
    psd->SlidAvgStruct = slidAvgBufStruct;
    slidavg_Init(slidAvgBufStruct, floatBuffer, cnt);
}

void psd_Reset(PSDStructTypedef *psd){
    psd->DesiredValue = 0;
    psd->LastError = 0;
    psd->ErrorSum = 0;
    slidavg_Reset(psd->SlidAvgStruct);
}

void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d){
	if( p > 0) psd->P = p;
    if( s > 0) psd->S = s;
    if( d > 0) psd->D = d;
}

void psd_IncrementConstants(PSDStructTypedef *psd, float p, float s, float d){
	psd->P += p;
	psd->S += s;
	psd->D += d;

}
void psd_SetPeriod(PSDStructTypedef *psd, float newPeriodS){
	if( newPeriodS < 0 ) newPeriodS = 0;
	psd->Period = newPeriodS;
}

void psd_SetSumSaturation(PSDStructTypedef *psd, int satMin, int satMax){
    psd->SumSaturationMax = satMax;
    psd->SumSaturationMin = satMin;
}

void psd_SetOutputSaturation(PSDStructTypedef *psd, int satMin, int satMax){
    psd->OutputSaturationMax = satMax;
    psd->OutputSaturationMin = satMin;
}

void psd_SetDesiredValue(PSDStructTypedef *psd, float desiredValue){
    psd->DesiredValue = desiredValue;
}

float psd_UpdatePSD(PSDStructTypedef *psd, float actualValue){
    float PSDOutput, Error, ErrorSum, ErrorDifference;
    float p, s, d, Period;
    p = psd->P;
    s = psd->S;
    d = psd->D;



    Error = psd->DesiredValue - actualValue;	//error is difference between desired and actual value


    ErrorSum = psd->ErrorSum + Error;			//error sumation - just add to previous value
    //if(ErrorSum * Error < 0) ErrorSum = 0;		//clear errorsum when error and errorsum have different sign
    //ErrorDifference = Error - psd->LastError;
	slidavg_AddVal(psd->SlidAvgStruct, Error - psd->LastError);
	ErrorDifference = slidavg_GetAvg(psd->SlidAvgStruct);

	Period = psd->Period;

    //handle sum saturation
    if ( psd->SumSaturationMax == 0 && psd->SumSaturationMin == 0 ){
        //sum saturation not set
    }
    else{
        if( ErrorSum > (psd->SumSaturationMax) ) {
            ErrorSum = psd->SumSaturationMax;
        }
        else if( ErrorSum < (psd->SumSaturationMin) ) {
            ErrorSum = psd->SumSaturationMin;
        }
    }

    psd->ErrorSum = ErrorSum;
    psd->LastError = Error;

    PSDOutput = p * ( Error + ( ( (float)ErrorSum / s ) * Period ) + ( ( (float)ErrorDifference * d ) / Period ) );
    //PSDOutput = p * ( Error );

    //handle output saturation
    if ( psd->OutputSaturationMax == 0 && psd->OutputSaturationMin ==0 ) {
        //output saturation not set
    }
    else {
        if( PSDOutput > (psd->OutputSaturationMax) ) {
            PSDOutput = psd->OutputSaturationMax;
        }
        else if( PSDOutput < (psd->OutputSaturationMin) )  {
            PSDOutput = psd->OutputSaturationMin;
        }
    }
	psd->LastOutput = PSDOutput;
    return PSDOutput;
}
#endif





