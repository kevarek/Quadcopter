/*
*Filename: blinky.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef BLINKY_H
#define BLINKY_H

extern void blinky_Init(void);



#define BLINKY_NOTIFICATION_CALIBRATING	100
#define BLINKY_NOTIFICATION_NORMAL		1000
extern void blinky_SetNotificationPeriod(int newPeriodMs);

#endif  //end of BLINKY_H

