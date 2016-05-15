/*
*Filename: sensors.c
*Author: Stanislav Subrt
*Year: 2014
*/

#include "sensors.h"
#include "main.h"
#include "I2C.h"


#define SENS_I2C_ADDR_GYRO					(0x69 & 0x7f) << 1
#define SENS_I2C_ADDR_ACC					(0x53 & 0x7f) << 1
#define SENS_I2C_ADDR_MAG					(0x1E & 0x7f) << 1
#define SENS_I2C_ADDR_BARO					(0x77 & 0x7f) << 1


util_ErrTd sens_Init(void){

	//Make sure all sensors are connected and initialize them on success
	if( sens_VerifyAccConnection() == util_ErrTd_Ok ){
		sens_ConfigureAcc();
	}
	else{
	}

	if( sens_VerifyGyroConnection() == util_ErrTd_Ok ){
		sens_ConfigureGyro();
	}
	else{
	}

	if( sens_VerifyMagConnection() == util_ErrTd_Ok ){
		sens_ConfigureMag();
	}
	else{
	}

	if( sens_VerifyBarConnection() == util_ErrTd_Ok ){
		sens_ConfigureBar();
	}
	else{
	}

	return util_ErrTd_Ok;
}


//////////////////////////////////////////////////////////////////////
////////// ADXL345 ACCELEROMETER /////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define ACC_REG_DEVID			0x00
#define ACC_REG_DEVID_CONTENT	0xE5

#define ACC_REG_BWRATE			0x2C
#define ACC_REG_POWERCTL		0x2D
#define ACC_REG_DATAFORMAT		0x31
#define ACC_REG_INTENABLE		0x2E
#define ACC_REG_INTMAP			0x2F
#define ACC_REG_DATAX0			0x32
#define ACC_REG_DATAY0			0x34
#define ACC_REG_DATAZ0			0x36


util_ErrTd sens_VerifyAccConnection(void){
	uint8_t DevId = 0;

	I2C_ReadRegs(SENS_I2C_ADDR_ACC, ACC_REG_DEVID, 1, &DevId);

	if( DevId == ACC_REG_DEVID_CONTENT ){
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


util_ErrTd sens_ConfigureAcc(void){
	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_BWRATE, 0x0B);			//Set output rate to 200Hz with 100Hz bandwidth
	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_DATAFORMAT, 0x00);		//Set range to +-2g, left align of data with 10 bit resolution (LSB~4mg)

	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_INTENABLE, 0x80);		//Data ready interrupt
	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_INTMAP, 0x00);			//Data ready interrupt mapped on INT1 pin

	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_POWERCTL, 0x00);		//Enable
	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_POWERCTL, 0x10);		//Enable
	I2C_WriteReg(SENS_I2C_ADDR_ACC, ACC_REG_POWERCTL, 0x08);		//Enable
	return util_ErrTd_Ok;
}


util_ErrTd sens_GetAccData(sens_AccDataTd *destBuf){
	uint8_t tmp[6];

	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	if( I2C_ReadRegs(SENS_I2C_ADDR_ACC, ACC_REG_DATAX0, 6, tmp) == util_ErrTd_Ok ){
		//Shift values to create properly formed integer
		destBuf->X = ( (tmp[1] << 8) | (tmp[0]) );
		destBuf->Y = ( (tmp[3] << 8) | (tmp[2]) );
		destBuf->Z = ( (tmp[5] << 8) | (tmp[4]) );

		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


//////////////////////////////////////////////////////////////////////
////////// L3G4200D GYROSCOPE ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define GYRO_REG_WHOAMI				0x0F
#define GYRO_REG_WHOAMI_CONTENT		0b11010011

#define GYRO_REG_REG1				0x20
#define GYRO_REG_REG4				0x23
#define GYRO_REG_DATAXL				0x28
#define GYRO_REG_DATAYL				0x2A
#define GYRO_REG_DATAZL				0x2C


static struct {
	int16_t XOffset;
	int16_t YOffset;
	int16_t ZOffset;
} GyroSettings;


util_ErrTd sens_VerifyGyroConnection(void){
	uint8_t DevId = 0;

	I2C_ReadRegs(SENS_I2C_ADDR_GYRO, GYRO_REG_WHOAMI, 1, &DevId);
	if( DevId == GYRO_REG_WHOAMI_CONTENT ){
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


#define SENS_SET_GYRO_CALIBCYCLES				4096
util_ErrTd sens_ConfigureGyro(void){
	int32_t i = 0, gox = 0, goy = 0, goz = 0;
	sens_GyroDataTd gd;

	I2C_WriteReg(SENS_I2C_ADDR_GYRO, GYRO_REG_REG1, 0x7F);				//Set output rate to 200Hz with 70Hz cutoff
	I2C_WriteReg(SENS_I2C_ADDR_GYRO, GYRO_REG_REG4, 0xA0);				//Block data update, 2000dps range

	//Offset calibration
	GyroSettings.XOffset = 0;
	GyroSettings.YOffset = 0;
	GyroSettings.ZOffset = 0;
	for( i=0; i<SENS_SET_GYRO_CALIBCYCLES; i++ ){
		sens_GetGyroData(&gd);
		gox += gd.X;
		goy += gd.Y;
		goz += gd.Z;
	}
	GyroSettings.XOffset = (int16_t)( gox / SENS_SET_GYRO_CALIBCYCLES );
	GyroSettings.YOffset = (int16_t)( goy / SENS_SET_GYRO_CALIBCYCLES );
	GyroSettings.ZOffset = (int16_t)( goz / SENS_SET_GYRO_CALIBCYCLES );

	return util_ErrTd_Ok;
}


#define DPSPERDIGIT	0.07
inline float sens_GetGyroSensitivity(void){
	return (float)DPSPERDIGIT;
}


#define SENS_GYRO_READMULTIPLE_BIT	0x80	//This is kind of hack from datasheet. To read multiple regs in burst, BIT7 of reg address must be one
util_ErrTd sens_GetGyroData(sens_GyroDataTd *destBuf){
	uint8_t tmp[6];

	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	if( I2C_ReadRegs(SENS_I2C_ADDR_GYRO, GYRO_REG_DATAXL | SENS_GYRO_READMULTIPLE_BIT, 6, tmp) == util_ErrTd_Ok ){
		//Shift values to create properly formed integer
		destBuf->X = ( ( (tmp[1] << 8) | (tmp[0]) ) - GyroSettings.XOffset );
		destBuf->Y = ( ( (tmp[3] << 8) | (tmp[2]) ) - GyroSettings.YOffset );
		destBuf->Z = ( ( (tmp[5] << 8) | (tmp[4]) ) - GyroSettings.ZOffset );
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


//////////////////////////////////////////////////////////////////////
////////// HMC5883L MAGNETOMETER /////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define MAG_REG_IRA				0x0A
#define MAG_REG_IRA_CONTENT		0b01001000
#define MAG_REG_IRB				0x0B
#define MAG_REG_IRB_CONTENT		0b00110100
#define MAG_REG_IRC				0x0C
#define MAG_REG_IRC_CONTENT		0b00110011

#define MAG_REG_CRA				0x00
#define MAG_REG_CRB				0x01
#define MAG_REG_MODER			0x02
#define MAG_REG_DATAXH			0x03
#define MAG_REG_DATAYH			0x07
#define MAG_REG_DATAZH			0x05
#define MAG_REG_STATUS			0x09


util_ErrTd sens_VerifyMagConnection(void){
	uint8_t DevId = 0;

	I2C_ReadRegs(SENS_I2C_ADDR_MAG, MAG_REG_IRA, 1, &DevId);
	if( DevId == MAG_REG_IRA_CONTENT ){
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


util_ErrTd sens_ConfigureMag(void){
	I2C_WriteReg(SENS_I2C_ADDR_MAG, MAG_REG_CRA, 0x18);				//Set output rate to 75Hz with 1 sample averaging
	I2C_WriteReg(SENS_I2C_ADDR_MAG, MAG_REG_CRB, 0x20);				//Gain 1x
	I2C_WriteReg(SENS_I2C_ADDR_MAG, MAG_REG_MODER, 0x00);			//Low speed(still 400kHz), continuous mode

	return util_ErrTd_Ok;
}


#define MAG_HARD_CAL_X	-236
#define MAG_HARD_CAL_Y	-124
#define MAG_HARD_CAL_Z	2
util_ErrTd sens_GetMagData(sens_MagDataTd *destBuf){
	uint8_t tmp[6];

	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	if( I2C_ReadRegs(SENS_I2C_ADDR_MAG, MAG_REG_DATAXH, 6, tmp) == util_ErrTd_Ok ){
		//Shift values to create properly formed integer
		destBuf->X = ( ( (tmp[0] << 8) | (tmp[1]) ) - MAG_HARD_CAL_X );
		destBuf->Y = ( ( (tmp[4] << 8) | (tmp[5]) ) - MAG_HARD_CAL_Y );
		destBuf->Z = ( ( (tmp[2] << 8) | (tmp[3]) ) - MAG_HARD_CAL_Z );
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Err;
	}
}


//////////////////////////////////////////////////////////////////////
////////// BMP085 BAROMETER //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct {
	uint16_t AC1;
	uint16_t AC2;
	uint16_t AC3;
	int16_t AC4;
	int16_t AC5;
	int16_t AC6;
	uint16_t B1;
	uint16_t B2;
	uint16_t MB;
	uint16_t MC;
	uint16_t MD;
} BarFactoryCal;


#define MAG_REG_CALIB			0xAA
util_ErrTd sens_VerifyBarConnection(void){
	 I2C_ReadRegs(SENS_I2C_ADDR_BARO, MAG_REG_CALIB, 22, (uint8_t*)(&BarFactoryCal));

	if( BarFactoryCal.AC1 == 0x0 || BarFactoryCal.AC2 == 0x0 || BarFactoryCal.AC3 == 0x0 || BarFactoryCal.AC4 == 0x0 ||
					BarFactoryCal.AC5 == 0x0 || BarFactoryCal.AC6 == 0x0 || BarFactoryCal.B1 == 0x0 || BarFactoryCal.B2 == 0x0 ||
								BarFactoryCal.MB == 0x0 || BarFactoryCal.MC == 0x0 || BarFactoryCal.MD == 0x0 ){
		return util_ErrTd_Err;
	}
	else if( BarFactoryCal.AC1 == 0xFFFF || BarFactoryCal.AC2 == 0xFFFF || BarFactoryCal.AC3 == 0xFFFF || BarFactoryCal.AC4 == 0xFFFF ||
					BarFactoryCal.AC5 == 0xFFFF || BarFactoryCal.AC6 == 0xFFFF || BarFactoryCal.B1 == 0xFFFF || BarFactoryCal.B2 == 0xFFFF ||
								BarFactoryCal.MB == 0xFFFF || BarFactoryCal.MC == 0xFFFF || BarFactoryCal.MD == 0xFFFF ){
		return util_ErrTd_Err;
	}
	else{
		return util_ErrTd_Ok;
	}
}


util_ErrTd sens_ConfigureBar(void){
	return util_ErrTd_Ok;
}


util_ErrTd sens_GetBarData(uint8_t *destBuf){
	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	//TODO READ AND CALC BAR DATA

	return util_ErrTd_Ok;
}
