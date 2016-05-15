/*
*Filename: 	settings.c
*Author: 	Stanislav Subrt
*Year:		2015
*/


#include "settings.h"
#include "main.h"
#include "I2C.h"
#include "AT24C.h"
#include "stabilisation.h"



#define MODULE_ENABLED			1
#define VALIDITY_BYTE			(char)0x1B				//If validity byte is present at the start of non-volatile memory dedicated for settings storage, it means there is valid settings stored in memory


//////////////////////////////////////////////////////////////////////
////////// HARDWARE ABSTRACTION LAYER ////////////////////////////////
//////////////////////////////////////////////////////////////////////

static int WriteData(void *dest, void *src, unsigned cnt){
	int i;

	if( src == 0 /*|| dest == 0 */ ){
		return 0;
	}

	for( i=0; i<cnt; i++ ){
		AT24C_WriteByte((uint32_t)dest + i, *( (char*)src + i ) );
	}
	return i;
}


static int ReadData(void *src, void *dest, unsigned cnt){
	int i;

	if( dest == 0 /*|| src == 0 */){
		return 0;
	}
	for( i=0; i<cnt; i++){
		AT24C_ReadByte((uint32_t)src + i, (uint8_t*)dest + i);
	}
	return i;
}


//////////////////////////////////////////////////////////////////////
////////// DATA MANAGING FUNCTION ////////////////////////////////////
//////////////////////////////////////////////////////////////////////

static int WriteOrLoadSettings(OpTd op){
	unsigned ByteCnt = 0;
	int (*OpHandler)(void*, void*, unsigned);

	ByteCnt += sizeof(VALIDITY_BYTE);													//Skip validity word

	//Get pointer to propper operation handler (read or write function)
	if( op == OpTd_Write ){
		OpHandler = WriteData;
	}
	else{
		OpHandler = ReadData;
	}

	if( OpHandler == 0 ){
		return 0;
	}
	else{
		//HERE DECLARE ALL THE ELEMENTS TO SAVE OR LOAD
		//ByteCnt += OpHandler( (void*)ByteCnt, &st_Settings, sizeof( st_Settings ));
		ByteCnt += st_ManageSettings(OpHandler, (void*)ByteCnt);
		return ByteCnt;
	}
}


//////////////////////////////////////////////////////////////////////
////////// SETTINGS MODULE BODY //////////////////////////////////////
//////////////////////////////////////////////////////////////////////

util_ErrTd set_Init(void){
	char ValidityByte;

	//Load settings data from that flash
	if( MODULE_ENABLED ){
		ReadData(0, &ValidityByte, 1);
		if( ValidityByte == VALIDITY_BYTE ){	//If first byte in the eeprom is equal to VALIDITY_BYTE, there is valid settings in non-volatile memory.
			WriteOrLoadSettings(OpTd_Load);				//Read and copy byte by byte settings data from FLASH to respective structures
		}
	}
	return util_ErrTd_Ok;
}


util_ErrTd set_Save(void){

	I2C_LockWait();								//Get exclusive access to I2C peripheral
	WriteOrLoadSettings(OpTd_Write);			//Write structure by structure into non-volatile memory
	AT24C_WriteByte(0, VALIDITY_BYTE);			//Settings are stored in memory, mark it as valid
	I2C_Unlock();								//Release exclusive access to I2C peripheral
	return util_ErrTd_Ok;
}


util_ErrTd set_Load(void){
	I2C_LockWait();								//Get exclusive access to I2C peripheral
	WriteOrLoadSettings(OpTd_Load);				//Read and copy byte by byte settings data from FLASH to respective structures
	I2C_Unlock();								//Release exclusive access to I2C peripheral
	return util_ErrTd_Ok;
}


util_ErrTd set_ResetToDefaults(void){
	//TODO
	I2C_LockWait();								//Get exclusive access to I2C peripheral
	AT24C_WriteByte(0, VALIDITY_BYTE+1);		//Make validity byte invalid
	I2C_Unlock();								//Release exclusive access to I2C peripheral
	NVIC_SystemReset();
	return util_ErrTd_Ok;
}
