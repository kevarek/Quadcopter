/*
*filename: stabilisation.c
*author: Stanislav Subrt
*year: 2013
*/

#include "stabilisation.h"
#include "sensors.h"


/*



        Eng1>
          |
          |
          |
Eng4< -------- Eng3<
          |
          |
          |
		Eng2>


*/




static int DesiredThrust;
PSDStructTypedef                    PitchPSDStruct, RollPSDStruct, YawPSDStruct;    //structures of PSD regulators
slidavg_BufferStructTypedef			PitchSlidAvgStruct, RollSlidAvgStruct, YawSlidAvgStruct;
float								PitchSlidAvgBuf[10], RollSlidAvgBuf[10], YawSlidAvgBuf[1];


#define DEFAULT_P_VALUE 180
#define DEFAULT_S_VALUE 8
#define DEFAULT_D_VALUE 0.6

#define DEFAULT_YAW_P	100
#define DEFAULT_YAW_S	999999999
#define DEFAULT_YAW_D	0

int asdasd;
void stabilisation_InitPSDStructs(void){
	asdasd = sizeof(PitchSlidAvgBuf)/sizeof(PitchSlidAvgBuf[0]);
    psd_Init(&PitchPSDStruct, &PitchSlidAvgStruct, PitchSlidAvgBuf, sizeof(PitchSlidAvgBuf)/sizeof(PitchSlidAvgBuf[0]));
    psd_SetConstants(&PitchPSDStruct, DEFAULT_P_VALUE, DEFAULT_S_VALUE, DEFAULT_D_VALUE);    //p s d
    psd_SetPeriod(&PitchPSDStruct, SENSORS_SAMPLINGPERIOD_S);
    psd_SetDesiredValue(&PitchPSDStruct, 0);
    psd_SetSumSaturation(&PitchPSDStruct, -40000, 40000);


    psd_Init(&RollPSDStruct, &RollSlidAvgStruct, RollSlidAvgBuf, sizeof(RollSlidAvgBuf)/sizeof(RollSlidAvgBuf[0]));
    psd_SetConstants(&RollPSDStruct, DEFAULT_P_VALUE, DEFAULT_S_VALUE, DEFAULT_D_VALUE);    //p s d
    psd_SetPeriod(&RollPSDStruct, SENSORS_SAMPLINGPERIOD_S);
    psd_SetDesiredValue(&RollPSDStruct, 0);
    psd_SetSumSaturation(&RollPSDStruct, -40000, 40000);

    psd_Init(&YawPSDStruct, &YawSlidAvgStruct, YawSlidAvgBuf, sizeof(YawSlidAvgBuf)/sizeof(YawSlidAvgBuf[0]));
    psd_SetConstants(&YawPSDStruct, DEFAULT_YAW_P, DEFAULT_YAW_S, DEFAULT_YAW_D);    //p s d
    psd_SetPeriod(&YawPSDStruct, SENSORS_SAMPLINGPERIOD_S);
    psd_SetDesiredValue(&YawPSDStruct, 0);
    psd_SetSumSaturation(&YawPSDStruct, -20000, 20000);
}


static int IsEmergencyStop = 0;
void stabilisation_EmergencyStop(void){
	IsEmergencyStop = 1;
	eng_Stop();
	stabilisation_Reset();
}

void stabilisation_CancelEmergencyStop(void){
	IsEmergencyStop = 0;
}


void stabilisation_Init(void){
	stabilisation_InitPSDStructs();
}

void stabilisation_Reset(void){
	psd_Reset(&PitchPSDStruct);
	psd_Reset(&RollPSDStruct);
	psd_Reset(&YawPSDStruct);
}



static int UpdateRequestedFlag = 0;
void stabilisation_RequestUpdate(void){
	UpdateRequestedFlag = 1;
}

void stabilisation_HandleStabilisation(void){
	int PitchAction = 0, RollAction = 0, YawAction = 0;

	if( UpdateRequestedFlag == 0 ) return;
	if( IsEmergencyStop ) return;
	UpdateRequestedFlag = 0;


	PitchAction = psd_UpdatePSD(&PitchPSDStruct, AircraftActualAngles.Pitch);
	RollAction = psd_UpdatePSD(&RollPSDStruct, -1 * AircraftActualAngles.Roll);
	YawAction = psd_UpdatePSD(&YawPSDStruct, AircraftActualAngles.Yaw);

	eng_SetThrust(&Eng1, DesiredThrust + PitchAction + YawAction);
	eng_SetThrust(&Eng2, DesiredThrust - PitchAction + YawAction);
	eng_SetThrust(&Eng3, DesiredThrust - RollAction  - YawAction);
	eng_SetThrust(&Eng4, DesiredThrust + RollAction  - YawAction);
}


void stabilisation_SetDesiredThrust(int newVal){
	//todo min or max saturation may be needed
	DesiredThrust = newVal;
}


void stabilisation_SetDesiredPitchAngle(float newVal){
	psd_SetDesiredValue(&PitchPSDStruct, newVal);
}

void stabilisation_SetDesiredRollAngle(float newVal){
	psd_SetDesiredValue(&RollPSDStruct, newVal);
}


void stabilisation_SetDesiredYawAngle(float newVal){
	psd_SetDesiredValue(&RollPSDStruct, newVal);
}

void stabilisation_IncrementDesiredYawAngle(float valToInc){
	float NewVal;
	NewVal = YawPSDStruct.DesiredValue + valToInc;
	psd_SetDesiredValue(&YawPSDStruct, NewVal);
}
