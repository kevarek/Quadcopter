/*
*Filename: mpu6050.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "mpu6050.h"

#include "perftests.h"


#define MPU6050_DEFAULTADDR		 (0x68 & 0x7f) << 1

#define MPU6050_I2CPERIPHCLK			RCC_APB1Periph_I2C1
#define MPU6050_GPIOPERIPHCLK			RCC_AHBPeriph_GPIOA
#define MPU6050_I2C						I2C1
#define MPU6050_I2CPORT					GPIOA
#define MPU6050_PIN_SDA					GPIO_Pin_10
#define MPU6050_PIN_SCL					GPIO_Pin_9
#define MPU6050_PINSRC_SDA				GPIO_PinSource10
#define MPU6050_PINSCR_SCL				GPIO_PinSource9


void mpu6050_Init(void){
	//initialize I2C bus
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//enable I2C and GPIO clocks
	RCC_APB1PeriphClockCmd(MPU6050_I2CPERIPHCLK, ENABLE);
	RCC_AHBPeriphClockCmd(MPU6050_GPIOPERIPHCLK, ENABLE);

	//configure SDA and SCL pins
	GPIO_InitStructure.GPIO_Pin =  MPU6050_PIN_SDA | MPU6050_PIN_SCL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(MPU6050_I2CPORT, &GPIO_InitStructure);
	GPIO_PinAFConfig(MPU6050_I2CPORT, MPU6050_PINSRC_SDA, GPIO_AF_4);
	GPIO_PinAFConfig(MPU6050_I2CPORT, MPU6050_PINSCR_SCL, GPIO_AF_4);

	//configure I2C
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_Timing = 0x00201D2B;		//master, standard speed mode, 100KHz, analog filter on, raise/fall time 100/10ns
	I2C_InitStructure.I2C_OwnAddress1 = MPU6050_DEFAULTADDR; // MPU6050 7-bit adress = 0x68, 8-bit adress = 0xD0;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
	I2C_InitStructure.I2C_DigitalFilter = 0;
	I2C_Init(MPU6050_I2C, &I2C_InitStructure);

	I2C_Cmd(MPU6050_I2C, ENABLE);


	mpu6050_WriteRegSync(MPU6050_RA_PWR_MGMT_1, 0);		//turn on
	mpu6050_WriteRegSync(MPU6050_RA_ACCEL_CONFIG, 0);	//+-2g range
	mpu6050_WriteRegSync(MPU6050_RA_GYRO_CONFIG, 24);	//+-2000deg/s
	mpu6050_WriteRegSync(MPU6050_RA_CONFIG, 3);			//DLPF 200Hz, 2ms
}



int mpu6050_WriteRegSync(char regAddr, char regVal){
	int Timeout;

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while( I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_BUSY) != RESET ){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_Reload_Mode, I2C_Generate_Start_Write);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TXIS) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_SendData(MPU6050_I2C, regAddr);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TCR) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_AutoEnd_Mode, I2C_No_StartStop);


	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TXIS) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_SendData(MPU6050_I2C, regVal);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_STOPF) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_ClearFlag(MPU6050_I2C, I2C_ICR_STOPCF);

	return MPU6050_TRANSACTION_OK;
}


/*
int mpu6050_ReadRegSync(char regAddr, int* destBuffer){
	int Timeout;

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while( I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_BUSY) != RESET ){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TXIS) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_SendData(MPU6050_I2C, regAddr);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TC) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_AutoEnd_Mode, I2C_Generate_Start_Read);


    while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_RXNE) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	*destBuffer = I2C_ReceiveData(MPU6050_I2C);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_STOPF) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_ClearFlag(MPU6050_I2C, I2C_ICR_STOPCF);

	return MPU6050_TRANSACTION_OK;
}
*/


int mpu6050_ReadBurstSync(char baseRegAddr, char byteCount, char* destBuffer){
	int Timeout, i;

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while( I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_BUSY) != RESET ){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TXIS) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_SendData(MPU6050_I2C, baseRegAddr);

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_TC) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, byteCount, I2C_AutoEnd_Mode, I2C_Generate_Start_Read);


	for( i=0; i< byteCount; i++){
		Timeout = MPU6050_TRANSACTION_TIMEOUT;
		while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_RXNE) == RESET){
			if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
		}
		destBuffer[i] = I2C_ReceiveData(MPU6050_I2C);
	}

	Timeout = MPU6050_TRANSACTION_TIMEOUT;
	while(I2C_GetFlagStatus(MPU6050_I2C, I2C_ISR_STOPF) == RESET){
		if( --Timeout == 0 ) return MPU6050_TRANSACTION_ERROR;
	}
	I2C_ClearFlag(MPU6050_I2C, I2C_ICR_STOPCF);
	return i;
}



//////////////////////////////////////////////////////////////////////
////////// MPU6050 FUNCTIONS /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////



int mpu6050_GetRawData(char* destBuffer){
	return mpu6050_ReadBurstSync(0x3B, 14, destBuffer);
}

/*
//#define MPU6050_NOTCONNECTED	-1
//#define MPU6050_CONNECTED		-2
int mpu6050_TestConnection(void){
	int Result;
	mpu6050_ReadRegSync(MPU6050_RA_WHO_AM_I, &Result);

	if( Result !=  104){	//this is always content of who am i register
		return	MPU6050_NOTCONNECTED;
	}
	else{
		return MPU6050_CONNECTED;
	}
}
*/






