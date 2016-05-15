/*
*Filename: blinky.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef BLINKY_H
#define BLINKY_H

#include <stdint.h>

typedef struct{
	uint32_t TotalRAM;
	uint32_t FreeRAM;
	uint32_t StackUsage;
	uint32_t DataUsage;
	uint32_t CpuLoadVal;
	float CPULoad;
}bl_PerfTd;


typedef struct bl_TimeoutTd{
	uint32_t Expiration;
	struct bl_TimeoutTd* Prev;
	struct bl_TimeoutTd* Next;
	void (*ExpiredFn)(void);
	void (*NotExpiredFn)(void);
}bl_TimeoutTd;


extern void bl_Init(void);
extern unsigned bl_GetTick(void);
extern inline void bl_UpdatePerfCntr(void);
extern float bl_GetPreciseTick(void);

//Timeouts
extern void bl_InitTimeout(bl_TimeoutTd* timeoutStr, uint32_t timeoutValMs, void (*expiredCallbackFn)(void), void (*notExpiredCallbackFn)(void));
extern void bl_SetTimeout(bl_TimeoutTd* timeoutStr, uint32_t timeoutValMs);


#include "cmsis_os.h"
extern osThreadId bl_TaskId;
extern void bl_Task(void const *argument);

#endif  //end of BLINKY_H
