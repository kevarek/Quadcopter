/*
*Filename: main.h
*Author: Ing. Stanislav Subrt
*Year: 2015
*/

#ifndef MAIN_H
#define MAIN_H


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "stm32f3xx.h"
#include "cmsis_os.h"
#include "core_cm4.h"


#include "utility.h"
#include "blinky.h"
#include "com.h"




/*
	HW----------------------------------
		1HW0 - First quad, STM32F05 based, wooden frame, XBEE Pro, Joystick controlled via PC
		2HW0 - STM32F303 based, EEPROM, solid carbon frame, XBEE Pro, PS2 controller


	FW---------------------------------
		1FW0 - For 1HW0, While(1) architecture
		2FW0 - For 2HW0, RTOS based

*/



#define VERSIONSTRING		"StandaSubrt Quadrocopter 002 2HW0 2FW0"		//Company name, device name, device ID, HW version, FW version, reserved, reserved reserved
																			//[CompanyName] [DeviceName] [SerialNumber] [HWVersion] [FWVersion]

#define MAIN_BRD_VREF_MV	3300
#endif  //end of MAIN_H
