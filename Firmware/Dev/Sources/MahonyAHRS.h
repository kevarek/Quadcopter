//=====================================================================================================
// MahonyAHRS.h
//=====================================================================================================
//
// Madgwick's implementation of Mayhony's AHRS algorithm.
// See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
//
// Date			Author			Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
// 23/12/2014	Standa Subrt	Optimised for reduced EYE pain
//
//=====================================================================================================
#ifndef MahonyAHRS_h
#define MahonyAHRS_h

typedef struct{
	float q0;
	float q1;
	float q2;
	float q3;
}mahony_QuatTd;


extern void mahony_SetTwoKp(float val);
extern void mahony_SetTwoKi(float val);

extern float mahony_GetTwoKp(void);
extern float mahony_GetTwoKi(void);
extern void mahony_GetQuat(mahony_QuatTd *q);
extern void mahony_GetEulerAngles(float *roll, float *pitch, float *yaw);

extern void mahony_AHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);
extern void mahony_AHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az);


#endif
