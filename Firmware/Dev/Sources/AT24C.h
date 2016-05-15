/*
*Filename: AT24C.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef AT24C_H
#define AT24C_H

extern util_ErrTd AT24C_Init(void);
extern util_ErrTd AT24C_VerifyConnection(void);
extern util_ErrTd AT24C_ReadByte(uint16_t readAddr, uint8_t* dest);
extern util_ErrTd AT24C_WriteByte(uint16_t writeAddr, uint8_t data);

extern util_ErrTd AT24C_WriteStruct(uint16_t writeAddr, void* pStruct, uint8_t structSize);
extern util_ErrTd AT24C_ReadStruct(uint16_t readAddr, void* pStruct, uint8_t structSize);

#endif  //end of AT24C_H
