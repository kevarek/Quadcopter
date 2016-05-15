/*
*Filename: 	AT24C.c
*Author: 	Stanislav Subrt
*Year:		2014
*/

#include "main.h"
#include "AT24C.h"
#include "I2C.h"


#define AT24C_DEV_ADDRESS					(0b1010000 & 0x7f) << 1
#define AT24C_BYTEWRITE_TIMEOUT_MS			50

//Initialize AT24C driver. This fn should be called prior any driver operation.
util_ErrTd AT24C_Init(void){
	//Verify EEPROM connection
	if( AT24C_VerifyConnection() == util_ErrTd_Ok ){
		//Here may be initialization if needed
		return util_ErrTd_Ok;
	}
	else {
		return util_ErrTd_Err;
	}
}


//Checks if EEPROM is connected
util_ErrTd AT24C_VerifyConnection(void){
	uint8_t tmp;
	return AT24C_ReadByte(0, &tmp);
}


//////////////////////////////////////////////////////////////////////
////////// SINGLE BYTE READ/WRITE OPERATIONS /////////////////////////
//////////////////////////////////////////////////////////////////////

//Reads one byte from readAddr and saves it to dest pointer location with 10ms hardcoded timeout
util_ErrTd AT24C_ReadByte(uint16_t readAddr, uint8_t* dest){
	uint32_t Timeout;

	Timeout = HAL_GetTick() + AT24C_BYTEWRITE_TIMEOUT_MS;
	while( I2C_ReadRegs16(AT24C_DEV_ADDRESS, readAddr, 1, dest) != util_ErrTd_Ok ){
		if( Timeout < HAL_GetTick() ){
			return util_ErrTd_Timeout;
		}
	}
	return util_ErrTd_Ok;
}


//Writes one byte of data to specified writeAddress
util_ErrTd AT24C_WriteByte(uint16_t writeAddr, uint8_t data){
	uint32_t Timeout;

	Timeout = HAL_GetTick() + AT24C_BYTEWRITE_TIMEOUT_MS;
	while( I2C_WriteReg16(AT24C_DEV_ADDRESS, writeAddr, data) != util_ErrTd_Ok ){
		if( Timeout < HAL_GetTick() ){
			return util_ErrTd_Timeout;
		}
	}
	return util_ErrTd_Ok;
}


//////////////////////////////////////////////////////////////////////
////////// STRUCTURE READ/WRITE OPERATIONS ///////////////////////////
//////////////////////////////////////////////////////////////////////

util_ErrTd AT24C_WriteStruct(uint16_t writeAddr, void* pStruct, uint8_t structSize){
	uint32_t Timeout;
	uint8_t i;
	uint8_t Data;

	for( i=0; i<structSize; i++ ){
		Timeout = HAL_GetTick() + AT24C_BYTEWRITE_TIMEOUT_MS;
		Data = *( (uint8_t*)pStruct + i );
		while( I2C_WriteReg16(AT24C_DEV_ADDRESS, writeAddr + i, Data) != util_ErrTd_Ok ){
			if( Timeout < HAL_GetTick() ){
				return util_ErrTd_Timeout;
			}
		}
	}
	return util_ErrTd_Ok;
}


util_ErrTd AT24C_ReadStruct(uint16_t readAddr, void* pStruct, uint8_t structSize){
	uint32_t Timeout;
	uint8_t i;

	for( i=0; i<structSize; i++ ){
		Timeout = HAL_GetTick() + AT24C_BYTEWRITE_TIMEOUT_MS;
		while( I2C_ReadRegs16(AT24C_DEV_ADDRESS, readAddr + i, 1, pStruct + i) != util_ErrTd_Ok ){
			if( Timeout < HAL_GetTick() ){
				return util_ErrTd_Timeout;
			}
		}
	}
	return util_ErrTd_Ok;
}

