/*
*Filename: main.c
*Author: Ing. Stanislav Subrt
*Year: 2016
*/


#include "main.h"
#include "stabilisation.h"
#include "supervisor.h"
#include "clocks.h"
#include "leds.h"
#include "engines.h"
#include "I2C.h"
#include "AT24C.h"
#include "sensors.h"
#include "batmon.h"
#include "settings.h"


//////////////////////////////////////////////////////////////////////
////////// CMSIS RTX RTOS THREAD DEFINITIONS /////////////////////////
//////////////////////////////////////////////////////////////////////

osThreadId bl_TaskId;
osThreadDef(bl_Task, osPriorityLow, 1, 0);

osThreadId com_ModuleTaskId;
osThreadDef(com_ModuleTask, osPriorityHigh, 1, 0);

osThreadId st_StabilityTaskId;
osThreadDef(st_StabilityTask, osPriorityRealtime, 1, 0);

osThreadId su_TaskId;
osThreadDef(su_Task, osPriorityNormal, 1, 0);


//////////////////////////////////////////////////////////////////////
////////// HARDWARE INITIALIZATION ///////////////////////////////////
//////////////////////////////////////////////////////////////////////

static void BoardInit(void){
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));	//Enable FPU
	clk_Init();										//Clocks
    bl_Init();										//Initialize tick counting for HAL
    leds_Init();									//Leds
	bat_Init();										//Battery monitoring
	eng_Init();										//Engine control
    I2C_Init();										//Initialize I2C peripheral for sensors and eeprom communication
	AT24C_Init();									//Initialize AT24C EEPROM
	sens_Init();									//Sensor reading
	com_Init();										//Initialize communication
	set_Init();										//Initialize settings module
}


//////////////////////////////////////////////////////////////////////
////////// MAIN + RTOS INIT AND START ////////////////////////////////
//////////////////////////////////////////////////////////////////////


int main(void){
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);							//Set priority grouping
    osKernelInitialize();														//Start RTOS kernel and default tasks
    BoardInit();																//Board initialization

    bl_TaskId = osThreadCreate(osThread(bl_Task), NULL);						//Basic timing functionality and various benchmarks task
    com_ModuleTaskId = osThreadCreate(osThread(com_ModuleTask), NULL);			//Communication module task
    st_StabilityTaskId = osThreadCreate(osThread(st_StabilityTask), NULL);		//Stability task
    su_TaskId = osThreadCreate(osThread(su_Task), NULL);						//Supervisor task

    osKernelStart();
    return 0;
}


