/*
*Filename: psd.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef PSD_H
#define PSD_H


#define PSD_USE_SLIDING_AVERAGE



#ifndef PSD_USE_SLIDING_AVERAGE

typedef struct{
    float P;
    float S;
    float D;

	float Period;

    float DesiredValue;
	float LastOutput;

    float LastError;
    float ErrorSum;

    int SumSaturationMax;
    int SumSaturationMin;
    int OutputSaturationMax;
    int OutputSaturationMin;
} PSDStructTypedef;

extern void psd_Init(PSDStructTypedef *psd);
extern void psd_Reset(PSDStructTypedef *psd);
extern void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d);

extern void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d);
extern void psd_IncrementConstants(PSDStructTypedef *psd, float p, float s, float d);

extern void psd_SetPeriod(PSDStructTypedef *psd, float newPeriodS);
extern void psd_SetSumSaturation(PSDStructTypedef *psd, int satMin, int satMax);
extern void psd_SetOutputSaturation(PSDStructTypedef *psd, int satMin, int satMax);
extern void psd_SetDesiredValue(PSDStructTypedef *psd, float desiredValue);
extern float psd_UpdatePSD(PSDStructTypedef *psd, float actualValue);

#else

#include "slidavg.h"

typedef struct{
    float P;
    float S;
    float D;

	float Period;

    float DesiredValue;
	float LastOutput;

    float LastError;
    float ErrorSum;
    slidavg_BufferStructTypedef *SlidAvgStruct;

    int SumSaturationMax;
    int SumSaturationMin;
    int OutputSaturationMax;
    int OutputSaturationMin;
} PSDStructTypedef;

extern void psd_Init(PSDStructTypedef *psd, slidavg_BufferStructTypedef* slidAvgBufStruct, float *floatBuffer, int  cnt);
extern void psd_Reset(PSDStructTypedef *psd);
extern void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d);

extern void psd_SetConstants(PSDStructTypedef *psd, float p, float s, float d);
extern void psd_IncrementConstants(PSDStructTypedef *psd, float p, float s, float d);

extern void psd_SetPeriod(PSDStructTypedef *psd, float newPeriodS);
extern void psd_SetSumSaturation(PSDStructTypedef *psd, int satMin, int satMax);
extern void psd_SetOutputSaturation(PSDStructTypedef *psd, int satMin, int satMax);
extern void psd_SetDesiredValue(PSDStructTypedef *psd, float desiredValue);
extern float psd_UpdatePSD(PSDStructTypedef *psd, float actualValue);

#endif


#endif  //end of PSD_H
