/*
*Filename: I2C.h
*Author: Ing. Stanislav Subrt
*Year: 2015
*/

#include "main.h"
#include "I2C.h"

#define I2C_INSTANCE					I2C1
#define I2C_ENPERCLK					__I2C1_CLK_ENABLE

#define I2C_PIN_SDA						GPIO_PIN_9
#define I2C_PORT_SDA_INST				GPIOB
#define I2C_PORT_SDA_ENPERCLK			__GPIOB_CLK_ENABLE

#define I2C_PIN_SCL						GPIO_PIN_15
#define I2C_PORT_SCL_INST				GPIOA
#define I2C_PORT_SCL_ENPERCLK			__GPIOA_CLK_ENABLE

#define I2C_IRQH_EV						I2C1_EV_IRQHandler
#define I2C_IRQH_ER						I2C1_ER_IRQHandler

#define I2C_DEFTIMEOUT_MS				10

static I2C_HandleTypeDef 	hI2C;		//Handle to I2C peripheral
static osMutexId			mI2CId;		//I2C mutex id
static osMutexDef (mI2C);				//I2C bus is shared resource and we have to protect it


util_ErrTd I2C_Init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;

	//WARNING - MANUALY MODIFY STRUCT NAMES ACCORDING TO I2C1 OR I2C2 USAGE
	//Select SYSCLK (72MHz) as clock source for I2C
	RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
	RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

	//Configure GPIO pins for I2C peripheral
	I2C_PORT_SDA_ENPERCLK();
	I2C_PORT_SCL_ENPERCLK();

	GPIO_InitStructure.Pin = I2C_PIN_SCL;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(I2C_PORT_SCL_INST, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = I2C_PIN_SDA;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(I2C_PORT_SDA_INST, &GPIO_InitStructure);

	//Configure I2C peripheral
	I2C_ENPERCLK();

	//TODO NASTAVIT 400KHz po otestovani!!!!!!!!!!!!!!!!!!!!!!!!!
	hI2C.Instance = I2C_INSTANCE;
	hI2C.Init.Timing = 0x00E0257A;	//0x6020293A;
	hI2C.Init.OwnAddress1 = 0x00;
	hI2C.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hI2C.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
	hI2C.Init.OwnAddress2 = 0x00;
	hI2C.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
	hI2C.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;
	HAL_I2C_Init(&hI2C);
	HAL_I2CEx_AnalogFilter_Config(&hI2C,I2C_ANALOGFILTER_ENABLED);

	mI2CId = osMutexCreate(osMutex(mI2C));

	return util_ErrTd_Ok;
}


util_ErrTd I2C_LockWait(void){
	osMutexWait(mI2CId, osWaitForever);
	return util_ErrTd_Ok;
}


util_ErrTd I2C_Unlock(void){
	osMutexRelease(mI2CId);
	return util_ErrTd_Ok;
}


util_ErrTd I2C_ReadRegs(uint8_t devAddr, uint8_t regAddr, uint8_t regCount, uint8_t *destBuf){
	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	if( HAL_I2C_Master_Transmit(&hI2C, devAddr, &regAddr, 1, I2C_DEFTIMEOUT_MS) == HAL_OK ){
		HAL_I2C_Master_Receive(&hI2C, devAddr, destBuf, regCount, I2C_DEFTIMEOUT_MS);
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Ack;
	}
}


util_ErrTd I2C_ReadRegs16(uint8_t devAddr, uint16_t regAddr, uint8_t regCount, uint8_t *destBuf){
	uint8_t tmp[2];

	if( destBuf == 0 ){
		return util_ErrTd_Null;
	}

	tmp[0] = regAddr >> 8;
	tmp[1] = regAddr & 0xFF;

	if( HAL_I2C_Master_Transmit(&hI2C, devAddr, tmp, 2, I2C_DEFTIMEOUT_MS) == HAL_OK ){
		HAL_I2C_Master_Receive(&hI2C, devAddr, destBuf, regCount, I2C_DEFTIMEOUT_MS);
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Ack;
	}
}


//Writes one byte into register
util_ErrTd I2C_WriteReg(uint8_t devAddr, uint8_t regAddr, uint8_t data){
	uint8_t tmp[2];

	tmp[0] = regAddr;
	tmp[1] = data;

	if( HAL_I2C_Master_Transmit(&hI2C, devAddr, tmp, 2, I2C_DEFTIMEOUT_MS) == HAL_OK ){
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Ack;
	}
}


//Writes one byte into register (16 bit register addressing)
util_ErrTd I2C_WriteReg16(uint8_t devAddr, uint16_t regAddr, uint8_t data){
	uint8_t tmp[3];

	tmp[0] = regAddr >> 8;
	tmp[1] = regAddr & 0xFF;
	tmp[2] = data;

	if( HAL_I2C_Master_Transmit(&hI2C, devAddr, tmp, 3, I2C_DEFTIMEOUT_MS) == HAL_OK ){
		return util_ErrTd_Ok;
	}
	else{
		return util_ErrTd_Ack;
	}
}


void I2C_IRQH_EV(void){
	HAL_I2C_EV_IRQHandler(&hI2C);
}


void I2C_IRQH_ER(void){
	HAL_I2C_ER_IRQHandler(&hI2C);
}

