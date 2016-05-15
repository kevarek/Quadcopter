/*
*Filename: sensors.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef SENSORS_H
#define SENSORS_H

#include "main.h"


typedef struct {
	int16_t X;
	int16_t Y;
	int16_t Z;
} sens_AccDataTd;

typedef struct {
	int16_t X;
	int16_t Y;
	int16_t Z;
} sens_GyroDataTd;

typedef struct {
	int16_t X;
	int16_t Y;
	int16_t Z;
} sens_MagDataTd;


extern util_ErrTd sens_Init(void);

extern util_ErrTd sens_VerifyAccConnection(void);
extern util_ErrTd sens_ConfigureAcc(void);
extern util_ErrTd sens_GetAccData(sens_AccDataTd *destBuf);

extern util_ErrTd sens_VerifyGyroConnection(void);
extern util_ErrTd sens_ConfigureGyro(void);
extern util_ErrTd sens_GetGyroData(sens_GyroDataTd *destBuf);
extern inline float sens_GetGyroSensitivity(void);

extern util_ErrTd sens_VerifyMagConnection(void);
extern util_ErrTd sens_ConfigureMag(void);
extern util_ErrTd sens_GetMagData(sens_MagDataTd *destBuf);

extern util_ErrTd sens_VerifyBarConnection(void);
extern util_ErrTd sens_ConfigureBar(void);
extern util_ErrTd sens_GetBarData(uint8_t *destBuf);


#endif  //end of SENSORS_H
