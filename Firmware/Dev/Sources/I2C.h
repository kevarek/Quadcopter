/*
*Filename: I2C.h
*Author: Ing. Stanislav Subrt
*Year: 2015
*/

#ifndef I2C_H
#define I2C_H

extern util_ErrTd I2C_Init(void);

extern util_ErrTd I2C_LockWait(void);
extern util_ErrTd I2C_Unlock(void);

extern util_ErrTd I2C_ReadRegs(uint8_t devAddr, uint8_t regAddr, uint8_t regCount, uint8_t *destBuf);
extern util_ErrTd I2C_ReadRegs16(uint8_t devAddr, uint16_t regAddr, uint8_t regCount, uint8_t *destBuf);

extern util_ErrTd I2C_WriteReg(uint8_t devAddr, uint8_t regAddr, uint8_t data);
extern util_ErrTd I2C_WriteReg16(uint8_t devAddr, uint16_t regAddr, uint8_t data);

#endif  //end of I2C_H
