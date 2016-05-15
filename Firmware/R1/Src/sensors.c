/*
*Filename: sensors.c
*Author: Stanislav Subrt
*Year: 2013
*/


#include "sensors.h"
#include "stm32f0xx.h"
#include "mpu6050.h"
#include "comm.h"
#include "stabilisation.h"
#include <math.h>
#include "blinky.h"
#include "perftests.h"

//xxxx = ProcessedData->AccXAngle;	//roll left = 0...-90deg, roll right = 0...90deg
//yyyy = ProcessedData->AccYAngle;	//pitch nose down = 0...-90deg, nose up = 0...90deg

FlightAnglesStructTypedef	AircraftActualAngles;		//structure with actual airsrafts angles - pitch, roll, yaw
RawDataStructTypedef 		CalibrationRawDataStruct;	//structure with raw data from calibration



void sensors_Init(void){
	mpu6050_Init();								//initialize inercial unit
	sensors_InitializeBatteryMonitoring();		//initialize battery monitoring and events

	sensors_CalibrateGyros();					//calibrate sensors
}


static int UpdateRequestedFlag = 0;
void sensors_RequestUpdate(void){
	UpdateRequestedFlag = 1;
}


//not threadsafe
RawDataStructTypedef* sensors_GetRawData(void){
	char InercUnitRawDataBuffer[14];
	static RawDataStructTypedef RawDataStruct;

	if( mpu6050_GetRawData(InercUnitRawDataBuffer) ==  MPU6050_TRANSACTION_ERROR ){
		return 0;	//read inerc. data and return in case of error
	}
	else{
		RawDataStruct.AccX = (signed short)((unsigned short)( InercUnitRawDataBuffer[0] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[1] ));
		RawDataStruct.AccY = (signed short)((unsigned short)( InercUnitRawDataBuffer[2] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[3] ));
		RawDataStruct.AccZ = (signed short)((unsigned short)( InercUnitRawDataBuffer[4] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[5] ));

		RawDataStruct.Temperature = (signed short)((unsigned short)( InercUnitRawDataBuffer[6] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[7] ));

		RawDataStruct.GyroX = (signed short)((unsigned short)( InercUnitRawDataBuffer[8] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[9] ));
		RawDataStruct.GyroY = (signed short)((unsigned short)( InercUnitRawDataBuffer[10] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[11] ));
		RawDataStruct.GyroZ = (signed short)((unsigned short)( InercUnitRawDataBuffer[12] << 8 ) | (unsigned short)( InercUnitRawDataBuffer[13] ));
	}

	return &RawDataStruct;
}


//not threadsafe
ProcessedDataStructTypedef* sensors_ProcessRawData(RawDataStructTypedef *rawData){
	static ProcessedDataStructTypedef ProcessedDataStruct;

	ProcessedDataStruct.AccXAngle = 57.295*atan((float)rawData->AccY/ sqrt(pow((float)rawData->AccZ,2)+pow((float)rawData->AccX,2)));
	ProcessedDataStruct.AccYAngle = 57.295*atan((float)-rawData->AccX/ sqrt(pow((float)rawData->AccZ,2)+pow((float)rawData->AccY,2)));

	ProcessedDataStruct.GyroXRate = (float)rawData->GyroX * MPU6050_GYROLSBSPERDEG;
	ProcessedDataStruct.GyroYRate = (float)rawData->GyroY * MPU6050_GYROLSBSPERDEG;
	ProcessedDataStruct.GyroZRate = (float)rawData->GyroZ * MPU6050_GYROLSBSPERDEG;
	return &ProcessedDataStruct;
}


#define SENSORS_CALIBRATION_CYCLES	1000
void sensors_CalibrateGyros(void){
	int i;
	RawDataStructTypedef *RawData;
	ProcessedDataStructTypedef *ProcessedData;

	blinky_SetNotificationPeriod(BLINKY_NOTIFICATION_CALIBRATING);
	//reset calibration struct
	CalibrationRawDataStruct.AccX = 0;
	CalibrationRawDataStruct.AccY = 0;
	CalibrationRawDataStruct.AccZ = 0;
	CalibrationRawDataStruct.GyroX = 0;
	CalibrationRawDataStruct.GyroY = 0;
	CalibrationRawDataStruct.GyroZ = 0;
	CalibrationRawDataStruct.Temperature = 0;


	//save average raw data values into calibration structure
	for( i=0; i<SENSORS_CALIBRATION_CYCLES; i++){
		RawData = sensors_GetRawData();
		if( RawData == 0 ) continue;

		CalibrationRawDataStruct.AccX += RawData->AccX;
		CalibrationRawDataStruct.AccY += RawData->AccY;
		CalibrationRawDataStruct.AccZ += RawData->AccZ;
		CalibrationRawDataStruct.GyroX += RawData->GyroX;
		CalibrationRawDataStruct.GyroY += RawData->GyroY;
		CalibrationRawDataStruct.GyroZ += RawData->GyroZ;
		CalibrationRawDataStruct.Temperature += RawData->Temperature;
	}
	#define HARDCODED_CALIBRATION_FOR_PITCH_AND_ROLL
	#ifdef HARDCODED_CALIBRATION_FOR_PITCH_AND_ROLL
	CalibrationRawDataStruct.AccX = -945;
	CalibrationRawDataStruct.AccY = -170;
	#else
	CalibrationRawDataStruct.AccX /= SENSORS_CALIBRATION_CYCLES;
	CalibrationRawDataStruct.AccY /= SENSORS_CALIBRATION_CYCLES;
	#endif
	CalibrationRawDataStruct.AccZ /= SENSORS_CALIBRATION_CYCLES;
	CalibrationRawDataStruct.GyroX /= SENSORS_CALIBRATION_CYCLES;
	CalibrationRawDataStruct.GyroY /= SENSORS_CALIBRATION_CYCLES;
	CalibrationRawDataStruct.GyroZ /= SENSORS_CALIBRATION_CYCLES;
	CalibrationRawDataStruct.Temperature /= SENSORS_CALIBRATION_CYCLES;

	//update aircrafts angles
	ProcessedData = sensors_ProcessRawData(&CalibrationRawDataStruct);

	AircraftActualAngles.Pitch = ProcessedData->AccYAngle;
	AircraftActualAngles.Roll = ProcessedData->AccXAngle;
	AircraftActualAngles.Yaw = 0;


	blinky_SetNotificationPeriod(BLINKY_NOTIFICATION_NORMAL);
}


static float UserYawEstimation = 0;
void sensors_SetUserYawEstimation(float newValue){
	UserYawEstimation = newValue;
}


#define LP_COEFF	0.02
#define HP_COEFF	0.98
void sensors_HandleUpdates(void){
	RawDataStructTypedef *RawData;
	ProcessedDataStructTypedef *ProcessedData;


	if( UpdateRequestedFlag ){
		UpdateRequestedFlag = 0;


		///////////////////////'''''''''''''''''''!"!!!!!!!!!!!!!!!!
		perftests_StartTimer(1);

		RawData = sensors_GetRawData();		//read raw data from sensor
		//get rid of gyro drift from original structure
		RawData->GyroX = RawData->GyroX - CalibrationRawDataStruct.GyroX;
		RawData->GyroY = RawData->GyroY - CalibrationRawDataStruct.GyroY;
		RawData->GyroZ = RawData->GyroZ - CalibrationRawDataStruct.GyroZ;

		RawData->AccX = RawData->AccX - CalibrationRawDataStruct.AccX;
		RawData->AccY = RawData->AccY - CalibrationRawDataStruct.AccY;
		//pass modified original structure for processing
		ProcessedData = sensors_ProcessRawData(RawData);

		//complementary filter will merge gyro and acc data and save it into aircraft actual angles struct
		//angle = (0.98)*(angle + gyro * dt) + (0.02)*(x_acc);
		AircraftActualAngles.Pitch = ( HP_COEFF * (AircraftActualAngles.Pitch + ( ProcessedData->GyroYRate * SENSORS_SAMPLINGPERIOD_S ) ) ) + ( LP_COEFF * ProcessedData->AccYAngle );
		AircraftActualAngles.Roll = ( HP_COEFF * (AircraftActualAngles.Roll + ( ProcessedData->GyroXRate * SENSORS_SAMPLINGPERIOD_S ) ) ) + ( LP_COEFF * ProcessedData->AccXAngle );
		AircraftActualAngles.Yaw = 1 * ( (AircraftActualAngles.Yaw + ( ProcessedData->GyroZRate * SENSORS_SAMPLINGPERIOD_S ) ) ); //+ 0.1 * UserYawEstimation;


		sensors_HandleBatteryMonitoring();


		stabilisation_RequestUpdate();		//request action of stabilisation control
	}
}



//////////////////////////////////////////////////////////////////////
////////// BATTERY VOLTAGE SENSING ///////////////////////////////////
//////////////////////////////////////////////////////////////////////



#define SENSORS_BATMON_GPIO_PERIPHCLOCK		RCC_AHBPeriph_GPIOA
void sensors_InitializeBatteryMonitoring(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef	ADC_InitStructure;


	RCC_AHBPeriphClockCmd(SENSORS_BATMON_GPIO_PERIPHCLOCK, ENABLE);		//enable peripheral clock for ADC GPIO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);	//enable peripheral clock for ADC

	//configure GPIO pin as ADC_IN1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);


	ADC_DeInit(ADC1);
	ADC_StructInit(&ADC_InitStructure);					//initialize ADC init structure with default values which I need :) Check implementation
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_ChannelConfig(ADC1, ADC_Channel_1 , ADC_SampleTime_239_5Cycles);	//convert the ADC1 Channel 11 with 239.5 Cycles as sampling time
	ADC_GetCalibrationFactor(ADC1);						//calibrate ADC
	ADC_Cmd(ADC1, ENABLE);								//enable ADC peripheral

	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY));	//wait until ADC ready flag
}



#define VOLTAGE_DIVIDER_RATIO	5
#define ADC_VREF_MV				3300
#define ADC_RESOLUTION			0xFFF
int sensors_ReadBatteryVoltage(void){
	int AdcValue;

	ADC_StartOfConversion(ADC1);							//start conversion
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);	//wait for end of conversion
	AdcValue = ADC_GetConversionValue(ADC1);				//read results

	return (AdcValue * ADC_VREF_MV )/ ADC_RESOLUTION;		//calculate battery voltage and return value
}



#define VOLTAGE_BATTERY_EMPTY	10800
#define VOLTAGE_BATTERY_LOW		11500
void sensors_HandleBatteryMonitoring(void){
	int BatteryVoltage;

	BatteryVoltage = sensors_ReadBatteryVoltage();	//read battery voltage
	//TODO: find discharging curve to calculate capacity left

	if( BatteryVoltage < VOLTAGE_BATTERY_EMPTY){		//if battery is empty
		//do something to clearly indicate pilot to land asap
		//start panic blinking
		//start sending messages to pilot
	}
	else if( BatteryVoltage < VOLTAGE_BATTERY_LOW ){	//if battery is almost emty
		//do something
	}
}

