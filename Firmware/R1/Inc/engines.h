/*
*Filename: engines.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef ENGINES_H
#define ENGINES_H

#include "stm32f0xx.h"


#define ENGINES_MINTHRUST   0
#define ENGINES_MAXTHRUST   100000
#define ENGINES_THRUSTRANGE	100000


typedef struct {
    int ActualThrust;
    int SaturationMin;
    int SaturationMax;
    void (*SetThrustFn)(int newValueInPromile);
} EngineStructTypedef;


extern EngineStructTypedef Eng1, Eng2, Eng3, Eng4;


extern void eng_Init(void);
extern void eng_Stop(void);
extern void eng_SetThrust(EngineStructTypedef *eng, int newVal);
extern int eng_GetThrust(EngineStructTypedef *eng);



//////////////////////////////////////////////////////////////////////
////////// INTERNAL DECLARATIONS /////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void eng_InitEngineStructs(void);
void eng_SetEng1Thrust(int newVal);
void eng_SetEng2Thrust(int newVal);
void eng_SetEng3Thrust(int newVal);
void eng_SetEng4Thrust(int newVal);

#endif  //end of ENGINES_H
