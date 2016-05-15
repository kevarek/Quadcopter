/*
*Filename: stabilisation.c
*Author: Stanislav Subrt
*Year: 2014
*/

#include "stabilisation.h"
#include "main.h"
#include "I2C.h"
#include "engines.h"
#include "batmon.h"
#include "quaternions.h"
#include "MahonyAHRS.h"
#include "sensors.h"
#include "leds.h"


#define ST_DEFBATCUTOFF_V	10.0
#define ST_DEFMINTHRUST		0.2
#define ST_DEFACROP			0.0020
#define ST_DEFACROI			0.0000
#define ST_DEFACROD			0.0030

//quat_QuatTd RawPosQ;
//float RawRoll, RawPitch, RawYaw;
//float BatteryCurrent_A = 0, BatteryVoltage_V = 0, BatteryCapacityDepleted_mAh = 0;

static float SetPointRoll = 0, SetPointPitch = 0, SetPointYaw = 0, SetPointThrust = 0, MinThrust = ST_DEFMINTHRUST;

static util_PidTd AcroPitchPID =	{	0,	ST_DEFACROP,	ST_DEFACROI,	ST_DEFACROD, util_PidAntiWindupModeTd_Sat,	{-1,	1}	};
static util_PidTd AcroRollPID = 	{	0,	ST_DEFACROP,	ST_DEFACROI,	ST_DEFACROD, util_PidAntiWindupModeTd_Sat,	{-1,	1}	};
static util_PidTd AcroYawPID = 		{	0,	ST_DEFACROP,	ST_DEFACROI,	ST_DEFACROD, util_PidAntiWindupModeTd_Sat,	{-1,	1}	};


//This function receives handler to write or load function to persistent storage. All persistent variables for this module must be specified here.
//Returns number of bytes written
int32_t st_ManageSettings(int (*OpHandler)(void*, void*, unsigned), void *dest){
	int32_t Counter = 0;

	if( OpHandler == 0 ){
		return 0;
	}

	Counter += OpHandler(dest+Counter, &(AcroPitchPID.P), sizeof(AcroPitchPID.P));
	Counter += OpHandler(dest+Counter, &(AcroPitchPID.I), sizeof(AcroPitchPID.I));
	Counter += OpHandler(dest+Counter, &(AcroPitchPID.D), sizeof(AcroPitchPID.D));

	Counter += OpHandler(dest+Counter, &(AcroRollPID.P), sizeof(AcroRollPID.P));
	Counter += OpHandler(dest+Counter, &(AcroRollPID.I), sizeof(AcroRollPID.I));
	Counter += OpHandler(dest+Counter, &(AcroRollPID.D), sizeof(AcroRollPID.D));

	Counter += OpHandler(dest+Counter, &(AcroYawPID.P), sizeof(AcroYawPID.P));
	Counter += OpHandler(dest+Counter, &(AcroYawPID.I), sizeof(AcroYawPID.I));
	Counter += OpHandler(dest+Counter, &(AcroYawPID.D), sizeof(AcroYawPID.D));

	Counter += OpHandler(dest+Counter, &(MinThrust), sizeof(MinThrust));
	return Counter;
}


void st_ResetRegulators(void){
	util_ResetPid(&AcroPitchPID);
	util_ResetPid(&AcroRollPID);
	util_ResetPid(&AcroYawPID);
}


void st_IncrementKp(float val){
	AcroPitchPID.P += val;
	AcroRollPID.P += val;
	if( AcroPitchPID.P < 0 ){
		AcroPitchPID.P = 0;
	}
	if( AcroRollPID.P < 0 ){
		AcroRollPID.P = 0;
	}
}


void st_IncrementKi(float val){
	AcroPitchPID.I += val;
	AcroRollPID.I += val;
	if( AcroPitchPID.I < 0 ){
		AcroPitchPID.I = 0;
	}
	if( AcroRollPID.I < 0 ){
		AcroRollPID.I = 0;
	}
}


void st_IncrementKd(float val){
	AcroPitchPID.D += val;
	AcroRollPID.D += val;
	if( AcroPitchPID.D < 0 ){
		AcroPitchPID.D = 0;
	}
	if( AcroRollPID.D < 0 ){
		AcroRollPID.D = 0;
	}
}


void st_IncrementYawKp(float val){
	AcroYawPID.P += val;
	if( AcroYawPID.P < 0 ){
		AcroYawPID.P = 0;
	}
}


//Sets desired thrust within interval 0 to 1
#define ST_DESIREDTHRUST_MIN	0
#define ST_DESIREDTHRUST_MAX	1
void st_SetDesiredThrust(float val){
	UTIL_SATURATE(val, ST_DESIREDTHRUST_MIN, ST_DESIREDTHRUST_MAX);
	SetPointThrust = val;
}


#define ST_DESIREDROLL_MIN	-45
#define ST_DESIREDROLL_MAX	45
void st_SetDesiredRoll(float val){
	UTIL_SATURATE(val, ST_DESIREDROLL_MIN, ST_DESIREDROLL_MAX);
	SetPointRoll = val;
}


#define ST_DESIREDPITCH_MIN	-45
#define ST_DESIREDPITCH_MAX	45
void st_SetDesiredPitch(float val){
	UTIL_SATURATE(val, ST_DESIREDPITCH_MIN, ST_DESIREDPITCH_MAX);
	SetPointPitch = val;
}


#define ST_DESIREDYAW_MIN	-45
#define ST_DESIREDYAW_MAX	45
void st_SetDesiredYaw(float val){
	UTIL_SATURATE(val, ST_DESIREDYAW_MIN, ST_DESIREDYAW_MAX);
	SetPointYaw = val;
}


void st_IncrementMinThrust(float val){
	MinThrust += val;
	UTIL_SATURATE(MinThrust, 0, +INFINITY);
}

float st_GetMinThrust(void){
	return MinThrust;
}


//Very high priority stabilisation task which is fired each time data from gyro and acc are refreshed
void st_StabilityTask(void const *argument){
	sens_GyroDataTd GData;
//	sens_AccDataTd AData;
//	sens_MagDataTd MData;
	float GyroSens, Gx, Gy, Gz;
	float ActionRoll, ActionPitch, ActionYaw;

//	static uint32_t PrevMahonyTime = 0;
//	static uint32_t BatteryVoltageLowCntr = 0;

	GyroSens =  sens_GetGyroSensitivity();
	mahony_SetTwoKp(1);
	leds_Set(&GreenLed, util_StateTd_On);

	for(;;){

		//Read gyro data
		I2C_LockWait();
		sens_GetGyroData(&GData);
		Gx = (float)GData.X * GyroSens;
		Gy = (float)GData.Y * GyroSens;
		Gz = (float)GData.Z * GyroSens;
		I2C_Unlock();

		//Calculate PID action values for roll, pitch and yaw
		ActionRoll = util_UpdatePid(&AcroRollPID, SetPointRoll, Gx);
		ActionPitch = util_UpdatePid(&AcroPitchPID, SetPointPitch, -Gy);
		ActionYaw = util_UpdatePid(&AcroYawPID, SetPointYaw, -Gz);

		//Set thrust for each of engines accordingly to PID action values
		eng_SetThrottle(&eng_Right, SetPointThrust - ActionRoll - ActionYaw);
		eng_SetThrottle(&eng_Left, SetPointThrust + ActionRoll - ActionYaw );
		eng_SetThrottle(&eng_Front, SetPointThrust + ActionPitch + ActionYaw );
		eng_SetThrottle(&eng_Rear, SetPointThrust - ActionPitch + ActionYaw );

		/*
		if( ( bl_GetTick() - PrevMahonyTime ) >= ST_MAHONYPERIOD_MS ){
			PrevMahonyTime = bl_GetTick();

			sens_GetAccData(&AData);
			sens_GetMagData(&MData);

			mahony_AHRSupdate( 	Gx, 		Gy, 		Gz,
								AData.X,	AData.Y,	AData.Z,
								MData.X, 	MData.Y, 	MData.Z);


			//Calculate euler angle differences between home and actual quaternion
			mahony_GetQuat((mahony_QuatTd*)&RawPosQ);													//Get current rotation from mahonys AHRS referenced to world frame
			quat_GetYawPitchRollDeg(&RawPosQ, &RawYaw, &RawPitch, &RawRoll);							//Just calculate yaw, pitch and roll from raw data


			//Read battery voltage, current and calculate depleted capacity
			BatteryCurrent_A = bat_GetBatteryCurrent();
			BatteryVoltage_V = bat_GetBatteryVoltage();
			BatteryCapacityDepleted_mAh += ( BatteryCurrent_A * ST_MAHONYPERIOD_MS ) / 3600;

			if( BatteryVoltage_V < ( st_Settings.BatteryCutOffV + 0.4 ) ){
				leds_Set(&GreenLed, util_StateTd_On);
			}

			//Stop core, peripherals and engines after 1000 battery voltage readings under cutoff voltage
			if( BatteryVoltage_V < st_Settings.BatteryCutOffV ){
				if( ++BatteryVoltageLowCntr > 1000 ){
					HAL_PWR_EnterSTANDBYMode();
				}
			}
		}
		*/
		osDelay(1);
	}
}

