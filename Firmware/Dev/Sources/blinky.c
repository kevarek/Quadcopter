/*
*Filename: blinky.c
*Author: Stanislav Subrt
*Year: 2014

PURPOSE OF THIS MODULE:
>	General timebase - This module serves as general timebase module. Interrupt is fired every 1ms, that increments Tick variable,
	which can be used for synchronous busy waiting and is also a timebase for STM32 HAL.
>	CPU performance monitor - Part of this module is cpu performance monitor, which increments a variable in loop in iddle task of operating system.
	Ratio of that variable under load and under no load is used to calculate cpu utilization.
	User must call var incrementing function UpdatePerformanceMonitor in each iteration of idle task.
>	RAM monitor - During initialization of MCU is whole RAM filled with pattern like 0xDEADC0DE and than initialized as always. Iddle task reads
	periodically whole RAM and calculates stack+heap usage, RAM data usage and free space in between these two parts. RAM painting procedure
	must be called immediately after reset and stack pointer initialization.

//STACKPAINTING
.equ  ram_paint_clr, 0xDEADC0DE
			LDR R0, =ram_paint_clr		//painted stack color
			LDR R1, =_RAMSTART			//RAM start address and later actual address
			LDR R2, =_RAMEND			//RAM end address

ram_paint_copy_byte:
			CMP R1, R2					//Compare actual ram pointer and end of RAM pointer
			BCS ram_paint_end			//End loop if actual RAM pointer is higher than end of RAM pointer
			STR R0,[R1]					//Else paint memory with pattern
			ADD R1, R1, #4				//And increment actual ram address

			B	ram_paint_copy_byte		//Loop
ram_paint_end:
*/


#include "blinky.h"
#include "main.h"


//////////////////////////////////////////////////////////////////////
////////// SETTINGS //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


#define MEAS_ENABLED			1				//Enable or disable performance monitoring
#define MEAS_PERIOD				1000			//Period in [ms] of cpu performance monitoring
#define NOLOAD_PERF				2012678			//Hard coded value - result of performance monitoring under no load.
#define RAM_PATTERN				0xDEADC0DE		//Default value of RAM. Must be same as defined in startup code
#define	PATTERN_TRESHOLD		500				//When pattern is found in RAM more times than this treshlold, we may have found new border of stackheap/freespace/data


//////////////////////////////////////////////////////////////////////
////////// MODULE INIT ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


#define TIM_INSTANCE				TIM4
#define TIM_ENPERCLK				__TIM4_CLK_ENABLE
#define TIM_IRQH					TIM4_IRQHandler
#define TIM_IRQN					TIM4_IRQn
#define TIM_PRESC					36
#define TIM_CNTPERMS				2000


bl_PerfTd Perf;
static TIM_HandleTypeDef Tmr;
static uint32_t Tick = 0;
static uint32_t PerformanceCounter = 0;

//Enables timer and setup 1ms periodic interrupt.
void bl_Init(void){
	TIM_ENPERCLK();
	Tmr.Instance = TIM_INSTANCE;
	Tmr.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	Tmr.Init.CounterMode = TIM_COUNTERMODE_UP;
	Tmr.Init.Period = TIM_CNTPERMS - 1;
	Tmr.Init.Prescaler = TIM_PRESC - 1;
	HAL_TIM_Base_Init(&Tmr);

	HAL_NVIC_SetPriority(TIM_IRQN, 3, 3);
	HAL_NVIC_EnableIRQ(TIM_IRQN);
	__HAL_TIM_ENABLE_IT(&Tmr, TIM_IT_UPDATE);
	HAL_TIM_Base_Start(&Tmr);
}


//Gets actual value of tick.
inline unsigned bl_GetTick(void){
	return Tick;
}


uint32_t HAL_GetTick(void){
	return bl_GetTick();
}


float bl_GetPreciseTick(void){
	return (float)Tick + (float)Tmr.Instance->CNT / (float)TIM_CNTPERMS;
}

//////////////////////////////////////////////////////////////////////
////////// PERFORMANCE MODULE - STACK CHECKING ///////////////////////
//////////////////////////////////////////////////////////////////////

#if MEAS_ENABLED
extern uint8_t _RAMEND;
extern uint8_t _RAMSTART;

//Performs ram check routine which measures total, used and free ram and also stack and data usage.
void bl_CheckStack(void){
	uint8_t* Addr;
	uint32_t AddrDataEnd = 0, AddrStackEnd = 0, Data = 0;
	static uint32_t Cntr = 0;

	//Find end of data section in RAM
	Addr = &_RAMEND - 3;					//Calculate pointer to last word in ram
	while( Addr > &_RAMSTART ){				//Cycle for each of ram words except at address 0x00000000
		Data = *((uint32_t*)Addr);			//Get data at actual address
		if( Data == RAM_PATTERN ){				//and if value here matches with stack painted value
			Cntr++;							//Increment counter, which remembers how many PATTERN values in raw has been found
		}
		else{
			Cntr = 0;						//If value at actual address is different to stack paint pattern clear counter.
		}

		if( Cntr > PATTERN_TRESHOLD ){		//If more matches than treshold value has already been found
			AddrDataEnd = (uint32_t)Addr;	//Save actual address which is most probably end of data section
		}
		Addr -=4;							//Decrement address and repeat
	}

	//Find end of stack and heap section in RAM
	Addr = &_RAMSTART + 4;					//Calculate pointer to first word in ram
	while( Addr < &_RAMEND ){				//Cycle for each of ram words except at address 0x00000000
		Data = *((uint32_t*)Addr);			//Get data at actual address
		if( Data == RAM_PATTERN ){			//and if value here matches with stack painted value
			Cntr++;							//Increment counter, which remembers how many PATTERN values in raw has been found
		}
		else{
			Cntr = 0;						//If value at actual address is different to stack paint pattern clear counter.
		}

		if( Cntr > PATTERN_TRESHOLD ){		//If more matches than treshold value has already been found
			AddrStackEnd = (uint32_t)Addr;	//Save actual address which is probably end of stack and heap section
		}
		Addr +=4;							//Increment address and repeat
	}

	//Calculate RAM performance statistics
	Perf.TotalRAM = (uint32_t)(&_RAMEND) - (uint32_t)(&_RAMSTART) + 1;
	Perf.StackUsage = (uint32_t)(&_RAMEND) - AddrStackEnd + 1;
	Perf.DataUsage = AddrDataEnd - (uint32_t)(&_RAMSTART) + 1;
	Perf.FreeRAM = Perf.TotalRAM - Perf.StackUsage - Perf.DataUsage;
}
#endif


//Updates performance counter used for CPU load monitoring
inline void bl_UpdatePerfCntr(void){
	#if MEAS_ENABLED
	__disable_irq();
	PerformanceCounter++;
	__enable_irq();
	#endif
}



//////////////////////////////////////////////////////////////////////
////////// TIMEOUT HANDLING //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bl_TimeoutTd DefTimeout = { 0, 0, 0, 0, 0 };

void bl_InitTimeout(bl_TimeoutTd* timeoutStr, uint32_t timeoutValMs, void (*expiredCallbackFn)(void), void (*notExpiredCallbackFn)(void)){

	if( timeoutStr == 0 ){
		return;
	}

	timeoutStr->ExpiredFn = expiredCallbackFn;
	timeoutStr->NotExpiredFn = notExpiredCallbackFn;

	bl_SetTimeout(timeoutStr, timeoutValMs);
}


//TODO callbacky
void bl_SetTimeout(bl_TimeoutTd* timeoutStr, uint32_t timeoutValMs){
	bl_TimeoutTd *p = &DefTimeout;
	bl_TimeoutTd *pLast;

	if( timeoutStr == 0 ){
		return;
	}

	//Cycle through linked list
	while( p != 0 ){
		if( p == timeoutStr ){									//If timeout node with same address already exists
			p->Expiration = bl_GetTick() + timeoutValMs;		//Update existing
			return;												//And return
		}
		pLast = p;												//Save this pointer so we can return to this node
		p = p->Next;											//Move to next timeout node
	}

	//p now points to latest node in linked list
	pLast->Next = timeoutStr;									//Link new node to list
	timeoutStr->Prev = pLast;									//Save a ptr to prev node
	timeoutStr->Next = 0;										//This is last node in list
	timeoutStr->Expiration = bl_GetTick() + timeoutValMs;		//Update expiration time
}


void bl_CheckTimeouts(void){
	bl_TimeoutTd *p = &DefTimeout;

	if( p->Next != 0 ){
		p = p->Next;								//Skip default timeout which is just a starting point of linked list
		while( p != 0 ){							//Repeat for every node in list
			if( p->Expiration > bl_GetTick() ){		//Check nodes expiry
				if( p->NotExpiredFn != 0 ){			//Invoke callback if specified
					p->NotExpiredFn();
				}
			}
			else{									//If node has expired
				p->Prev->Next = p->Next;			//Remove timeout node from list
				if( p->Next != 0 ){
					p->Next->Prev = p->Prev;
				}
				if( p->ExpiredFn != 0 ){			//Invoke callback if specified
					p->ExpiredFn();
				}
			}
			p = p->Next;							//Jump to next node and repeat
		}
	}
}


//////////////////////////////////////////////////////////////////////
////////// RTOS TASK /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
uint8_t tmp[256];
//Blinky task which loops with period of 100ms.
void bl_Task(void const *argument) {
	#if MEAS_ENABLED
	static uint32_t StackCheckTmr = 0;
	#endif


	for (;;){
		#if MEAS_ENABLED
		if( StackCheckTmr < bl_GetTick() ){
			bl_CheckStack();
			StackCheckTmr = bl_GetTick() + MEAS_PERIOD;
		}
		#endif
		bl_CheckTimeouts();

		osDelay(100);
	}
}


//////////////////////////////////////////////////////////////////////
////////// TIMER INTERRUPT ROUTINE ///////////////////////////////////
//////////////////////////////////////////////////////////////////////

void TIM_IRQH(void){
	#if MEAS_ENABLED
	static volatile uint32_t PerformaceTimer = MEAS_PERIOD;
	#endif

	//If update interrupt is pending and update interrupt is enabled, clear update int flag, increment tick variable and if needed calculate cpu usage
	if( ( __HAL_TIM_GET_FLAG(&Tmr, TIM_FLAG_UPDATE) != RESET ) && ( __HAL_TIM_GET_ITSTATUS(&Tmr, TIM_IT_UPDATE) !=RESET ) ){
		__HAL_TIM_CLEAR_IT(&Tmr, TIM_IT_UPDATE);
		Tick++;

		#if MEAS_ENABLED
		if( PerformaceTimer-- <= 0 ){
			PerformaceTimer = MEAS_PERIOD;
			Perf.CpuLoadVal = PerformanceCounter;
			Perf.CPULoad = 1 - (float)PerformanceCounter/(float)NOLOAD_PERF;
			PerformanceCounter = 0;
		}
		#endif
	}
}
