/*
*Filename: sensors.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef SENSORS_H
#define SENSORS_H


#define SENSORS_SAMPLINGPERIOD_S	0.004		//in seconds



typedef struct {
	float Pitch;
	float Roll;
	float Yaw;
} FlightAnglesStructTypedef;


typedef struct {
	int AccX;
	int AccY;
	int AccZ;

	int GyroX;
	int GyroY;
	int GyroZ;

	int Temperature;
} RawDataStructTypedef;


typedef struct {
	float AccXAngle;
	float AccYAngle;

	float GyroXRate;
	float GyroYRate;
	float GyroZRate;
} ProcessedDataStructTypedef;



extern FlightAnglesStructTypedef	AircraftActualAngles;		//structure with actual airsrafts angles - pitch, roll, yaw
extern RawDataStructTypedef 		CalibrationRawDataStruct;	//structure with raw data from calibration

extern void sensors_Init(void);
extern void sensors_RequestUpdate(void);
extern void sensors_HandleUpdates(void);
extern void sensors_SetUserYawEstimation(float newValue);


void sensors_CalibrateGyros(void);
void sensors_InitializeBatteryMonitoring(void);
void sensors_HandleBatteryMonitoring(void);
void sensors_RequestBatteryVoltageUpdate(void);

#endif  //end of SENSORS_H
