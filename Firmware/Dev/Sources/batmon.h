/*
*Filename: batmon.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef BATMON_H
#define BATMON_H

//Initializes battery monitoring module
extern void bat_Init(void);

//Returns battery voltage in volts
extern float bat_GetBatteryVoltage(void);
//Returns battery current in amps
extern float bat_GetBatteryCurrent(void);

#endif  //end of BATMON_H
