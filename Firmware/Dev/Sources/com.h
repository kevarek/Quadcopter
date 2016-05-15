/*
*Filename: com.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef	COM_H
#define COM_H

#include <stdint.h>

//Initializes communication peripheral and buffers
extern void com_Init(void);

//Sends data via communication peripheral
extern void com_SendData(uint8_t *data, uint16_t sizeB);

//RTOS task which handles data reception and parsing
#include "cmsis_os.h"
extern osThreadId com_ModuleTaskId;
extern void com_ModuleTask(void const *argument);

#endif	//COM_H

