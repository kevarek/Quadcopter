/*
*Filename: motorcontrol.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include <stdint.h>
#include "inhub.h"
#include "stm32f3xx.h"
#include "cmsis_os.h"
#include "core_cm4.h"

#include "utility.h"
#include "outhub.h"


typedef enum{
	mc_StateTd_Off = 0,
	mc_StateTd_On = 1,
}mc_StateTd;


typedef enum{
	mc_DirTd_Inverted = -1,
	mc_DirTd_Normal = 1,
}mc_DirTd;


typedef enum{
	mc_RegTypeTd_None = 0,
	mc_RegTypeTd_Pos = 1,
	mc_RegTypeTd_Speed = 2,
	mc_RegTypeTd_External = 3,
}mc_RegTypeTd;


typedef enum{
	mc_CondModeTd_None = 0,
	mc_CondModeTd_Below,
	mc_CondModeTd_Above,
	mc_CondModeTd_PercMax,
	mc_CondModeTd_PercMin,
	mc_CondModeTd_WasAbove,
	mc_CondModeTd_WasBelow,
}mc_CondModeTd;


typedef enum{
	mc_CondLogicTd_OR = 0,
	mc_CondLogicTd_AND = 1,
}mc_CondLogicTd;


typedef struct{
	ih_HandleTd* RegCtrl;
	mc_RegTypeTd RegType;
	float Val;
	float TargetVal;
	float Delta;				//Value change per cycle
	uint8_t IsRelative;
}mc_SetPointTd;


typedef struct{
	ih_HandleTd *Cond1Ctrl;
	mc_CondModeTd Cond1Mode;
	float Cond1Val;

	ih_HandleTd *Cond2Ctrl;
	mc_CondModeTd Cond2Mode;
	float Cond2Val;

	mc_CondLogicTd Logic;
	mc_SetPointTd TermSetPoint;
	char Cond1Status;
	char Cond2Status;
	char DestAck;
}mc_TermTd;


typedef struct {
	char* Name;
	oh_HandleTd* Ctrl;
	mc_DirTd DirInv;

	util_PidTd* Reg;

	mc_SetPointTd SetPoint;
	mc_TermTd Termination;
}mc_MotorTd;


typedef struct{
	mc_MotorTd* Motor;
	ih_HandleTd* Sensor;

	util_PidTd PosReg;
	util_PidTd SpeedReg;
}mc_MotSensRegSettingsTd;


extern mc_MotorTd Motor1, Motor2, Motor3, Motor4;

#define MC_MAX_MOTORS	4
extern mc_MotorTd* mc_MotorList[MC_MAX_MOTORS];

#define MC_MAX_REG_SETTINGS		8
extern mc_MotSensRegSettingsTd MotSensRegSettingsList[MC_MAX_REG_SETTINGS];



extern util_ErrTd mc_SetRegulationParameters(uint32_t listIndex, mc_MotSensRegSettingsTd *regParams);
extern mc_MotSensRegSettingsTd* mc_GetRegulationParameters(uint32_t listIndex);
extern mc_MotSensRegSettingsTd* mc_GetRegSettingsForMotSens(const mc_MotorTd *motor, const ih_HandleTd* sensor);

extern util_ErrTd mc_SetState(mc_MotorTd *motor, util_StateTd newState);
extern util_ErrTd mc_SetThrottle(mc_MotorTd *motor, float val);
extern util_ErrTd mc_SetDirInv(mc_MotorTd *motor, mc_DirTd newDir);
extern util_ErrTd mc_SetThrottleRange(mc_MotorTd *motor, float valMin, float valMax);

extern util_ErrTd mc_MovePosSpeAcc(mc_MotorTd *motor, ih_HandleTd *sens, float pos, float spe, float acc, float dec, int ackPos);
extern util_ErrTd mc_MovePosSpe(mc_MotorTd *motor, ih_HandleTd *sens, float pos, float spe, int holdPos, int ackPos);
extern util_ErrTd mc_MoveEx(mc_MotorTd *motor, mc_SetPointTd *setPoint, mc_TermTd* termCond, int ackPos);
extern util_ErrTd mc_StopAll(void);

extern mc_MotorTd* mc_GetMotorByName(char* n);
extern mc_CondModeTd mc_GetCondModeByName(char* n);
extern mc_CondModeTd mc_GetCondLogicByName(char *n);
extern mc_RegTypeTd mc_GetRegTypeByName(char* n);


#include "cmsis_os.h"
extern osThreadId mc_TaskId;
extern void mc_Task(void const *argument);

#endif  //end of MOTORCONTROL_H
