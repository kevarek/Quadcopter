/*
*Filename: main.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "main.h"
#include "stabilisation.h"
#include "sensors.h"
#include "comm.h"

#include "perftests.h"

volatile int xxx = 0;


void Init(void){

	#ifdef DEBUG
	//stop peripherals with core while debugging
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_DBGMCU, ENABLE);
	DBGMCU_APB1PeriphConfig(0xFFFFFFFF, ENABLE);
	DBGMCU_APB2PeriphConfig(0xFFFFFFFF, ENABLE);
	#endif

    blinky_Init();
	comm_Init();
    eng_Init();
	sensors_Init();
    stabilisation_Init();

    perftests_Init();


}

void Tests(void){
volatile int i = 10000;







	while(i--);
	#define READYSTRING	"Quadrocopter has been initialized, calibrated and tested.\r\n"
	comm_SendData(READYSTRING, sizeof(READYSTRING));
}

int main(void)
{
    Init();
    Tests();
    while(1)
    {
        comm_HandleReceivedData();	//check for received commands from USART
        sensors_HandleUpdates();
        stabilisation_HandleStabilisation();
        xxx = perftests_ReadTimer();
    }
}
