/*
*Filename: supervisor.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include "stm32f3xx.h"
#include "cmsis_os.h"
#include "core_cm4.h"
#include "arm_math.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "cmsis_os.h"
extern osThreadId su_TaskId;
extern void su_Task(void const *argument);


#endif  //end of SUPERVISOR_H
