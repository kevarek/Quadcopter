/*
*Filename: gamepad.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <stdint.h>

typedef enum{
	gp_ErrTd_OK,
	gp_ErrTd_InvalidAPIFrame,
}gp_ErrTd;


typedef struct{
	float LeftJoyX;
	float LeftJoyY;
	float RightJoyX;
	float RightJoyY;

	unsigned Btn12;
	unsigned Square;
	unsigned Circle;
	unsigned Triangle;
	unsigned Cross;

	unsigned SignalStrength;
}gp_ParsedDataTd;


extern gp_ErrTd gp_ParseXBEEAPI(uint8_t *data, gp_ParsedDataTd *dest);

#endif  //end of GAMEPAD_H
