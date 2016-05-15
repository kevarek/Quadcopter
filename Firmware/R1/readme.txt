timer 2 for PWM
timer 3 for blinky -> also app ms timer



AccX = roll ... negative means left wing is going down
AccY = pitch ... negative means nose down


GyroXRate = ... left wing going down is positive value
GyroYRate = .. nose going down is negative value
GyroZrate = ... nose going left is negative








#define MPU6050_WRITETIMEOUT	10000
#define LM75_LONG_TIMEOUT	MPU6050_WRITETIMEOUT
int mpu6050_WriteReg(char regAddr, char regVal){
//int LM75_Timeout = LM75_LONG_TIMEOUT;
	int Timeout;

	CB_Add(&TxBufferI2CStruct, regAddr);	//add register address and register new value to buffer
	CB_Add(&TxBufferI2CStruct, regVal);
	#define MPU6050_WRITEREGBYTESNUMBER	2	//transferhandling byte with address and readwrite flag is not counted. One byte is register address and second byte is register value
	Timeout = 0;
	while ( I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_BUSY) != RESET ){	//wait until last transfer is ready or for timeout
		if( Timeout++ == MPU6050_READWRITE_TIMEOUT ) return MPU6050_READWRITE_NOTSTARTED;	//return error flag if timeout happend
	}

	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, MPU6050_WRITEREGBYTESNUMBER, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);	//generate start
	I2C_ITConfig(MPU6050_I2C, I2C_IT_TXI, ENABLE);
	return MPU6050_READWRITE_STARTED;
	//now interrupt TXIS will fire and we will read each byte from buffer one by one and send
}

int mpu6050_ReadReg(char regAddr){
	int Timeout;

	//add wait for
	CB_Add(&TxBufferI2CStruct, regAddr);	//add address of register to be read into write buffer

	Timeout = 0;
	while ( I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_BUSY) != RESET ){	//wait until last transfer is ready or for timeout
		if( Timeout++ == MPU6050_READWRITE_TIMEOUT ) return MPU6050_READWRITE_NOTSTARTED;	//return error flag if timeout happend
	}

	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_AutoEnd_Mode, I2C_Generate_Start_Write);	//generate start write condition - one byte will be written with register address
	I2C_ITConfig(MPU6050_I2C, I2C_IT_TXI, ENABLE);

	Timeout = 0;
	while ( I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_STOPF) != SET ){
		if( Timeout++ == MPU6050_READWRITE_TIMEOUT ){
			return MPU6050_READWRITE_NOTSTARTED;	//return error flag if timeout happend
		}
	}

	I2C_TransferHandling(MPU6050_I2C, MPU6050_DEFAULTADDR, 1, I2C_AutoEnd_Mode, I2C_Generate_Start_Read);	//generate start read condition - one byte will be read from slave

	Timeout = 0;
	while ( I2C_GetFlagStatus(MPU6050_I2C, I2C_FLAG_STOPF) != SET ){
		if( Timeout++ == MPU6050_READWRITE_TIMEOUT ){
			return MPU6050_READWRITE_NOTSTARTED;	//return error flag if timeout happend
		}
	}

	return MPU6050_READWRITE_STARTED;
}
