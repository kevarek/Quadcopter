/*
*Filename: gamepad.c
*Author: Stanislav Subrt
*Year: 2014
*/

#include "gamepad.h"

//These are definitions according to gamepad-XBEE interconnect and some HW parameters

#define BTN12BITPOS			8
#define BTNSQUAREBITPOS		6
#define BTNCIRCLEBITPOS		7
#define BTNTRIANGLEBITPOS	4
#define BTNCROSSBITPOS		5

#define XBEEADCMASK			0x3FF	//Mask for ADC data
#define XBEEADCRES			0x400	//Resolution of XBEE ADC


gp_ErrTd gp_ParseXBEEAPI(uint8_t *data, gp_ParsedDataTd *dest){
	unsigned BtnStatus = 0;

	switch( data[2] ){
		case 0x83:

			dest->SignalStrength = data[5];

			BtnStatus = ( data[10] << 8 ) | ( data[11] );
			dest->Btn12 = !( ( BtnStatus >> BTN12BITPOS ) & 0x1 );
			dest->Square = !( ( BtnStatus >> BTNSQUAREBITPOS ) & 0x1 );
			dest->Circle = !( ( BtnStatus >> BTNCIRCLEBITPOS ) & 0x1 );
			dest->Triangle = !( ( BtnStatus >> BTNTRIANGLEBITPOS ) & 0x1 );
			dest->Cross = !( ( BtnStatus >> BTNCROSSBITPOS ) & 0x1 );

			dest->LeftJoyX = ( (float)( XBEEADCMASK & ( data[12] << 8 | data[13] ) ) / (float)XBEEADCRES - 0.5 ) * 2 ;
			dest->LeftJoyY = ( (float)( XBEEADCMASK & ( data[14] << 8 | data[15] ) ) / (float)XBEEADCRES - 0.5 ) * -2 ;
			dest->RightJoyX = ( (float)( XBEEADCMASK & ( data[16] << 8 | data[17] ) ) / (float)XBEEADCRES - 0.5 ) * 2 ;
			dest->RightJoyY = ( (float)( XBEEADCMASK & ( data[18] << 8 | data[19] ) ) / (float)XBEEADCRES - 0.5 ) * -2 ;

			return gp_ErrTd_OK;
		break;
		default:
			return gp_ErrTd_InvalidAPIFrame;
		break;
	}
}
