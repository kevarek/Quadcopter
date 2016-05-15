/*
*Filename: 	motorcontrol.c
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

#include "motorcontrol.h"
#include "main.h"

static uint8_t StringBufer[128];


//mc_MotorTd Motor1 = { "m1", &StepOut1, mc_DirTd_Inverted};
//mc_MotorTd Motor2 = { "m2", &StepOut2, mc_DirTd_Normal};
//mc_MotorTd Motor3 = { "m3", &StepOut3, mc_DirTd_Inverted};
//mc_MotorTd Motor4 = { (uint8_t*)"m4", SetMot4State, SetMot4Throttle, {-5000, 5000}, mc_DirTd_Inverted};


mc_MotorTd* mc_MotorList[MC_MAX_MOTORS] = {
//		&Motor1,
//		&Motor2,
//		&Motor3,
//		&Motor4,
	};


//Motor speed in DegPerSec
//Speed PID regulation for stepper motors is not needed at it is directly controlled by feed forward constant. Speed[degpersec] = Stepspersec * FFConst
//Motor 200stepsperrev, driver with miccrostepping->3200 steps per rev. -> 8.888888889 pulse per deg
mc_MotSensRegSettingsTd MotSensRegSettingsList[MC_MAX_REG_SETTINGS] = {
//		{ &Motor1,	&Encoder1,	{0, 100, 0, 0}, {8.888888889, 0, 0, 0} },
//		{ &Motor2,	&Encoder2,	{0, 100, 0, 0}, {8.888888889, 0, 0, 0} },
//		{ &Motor3,	&Encoder3,	{0, 0, 0, 0}, {8.888888889, 0, 0, 0} },
	};


//////////////////////////////////////////////////////////////////////
////////// REGULATOR SETUP ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


util_ErrTd mc_SetRegulationParameters(uint32_t listPos, mc_MotSensRegSettingsTd *regParams){
	if( regParams == 0 ){
		return util_ErrTd_Null;
	}
	else if( ( listPos < 1 ) || ( listPos > MC_MAX_REG_SETTINGS ) ){
		return util_ErrTd_OutOfRange;
	}
	else if( ( regParams->Motor == 0 ) || ( regParams->Sensor == 0 ) ){
		return util_ErrTd_Uninitialized;
	}

	memcpy(&(MotSensRegSettingsList[listPos-1]), regParams, sizeof(mc_MotSensRegSettingsTd));
	return util_ErrTd_Ok;
}


mc_MotSensRegSettingsTd* mc_GetRegulationParameters(uint32_t listPos){

	if( ( listPos < 1 ) || ( listPos > MC_MAX_REG_SETTINGS ) ){
		return 0;
	}

	return &(MotSensRegSettingsList[listPos-1]);
}


mc_MotSensRegSettingsTd* mc_GetRegSettingsForMotSens(const mc_MotorTd *motor, const ih_HandleTd* sensor){
	uint32_t i;

	for( i=0; i<UTIL_SIZEOFARRAY(MotSensRegSettingsList); i++ ){
		if( MotSensRegSettingsList[i].Motor == motor && MotSensRegSettingsList[i].Sensor == sensor ){
			return &(MotSensRegSettingsList[i]);
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////
////////// MOTOR CONTROL /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

util_ErrTd mc_SetState(mc_MotorTd *motor, util_StateTd newState){
	if( motor == 0 ){
		return util_ErrTd_Null;
	}

	return oh_SetState(motor->Ctrl, newState);
}


util_ErrTd mc_SetThrottle(mc_MotorTd *motor, float val){
	if( motor == 0 ){
		return util_ErrTd_Null;
	}

	return oh_SetVal(motor->Ctrl, val * motor->DirInv );
}


util_ErrTd mc_GetThrottle(mc_MotorTd *motor, float* val){
	if( motor == 0 ){
		return util_ErrTd_Null;
	}
	return oh_GetVal(motor->Ctrl, val);
}


util_ErrTd mc_SetThrottleRange(mc_MotorTd *motor, float valMin, float valMax){
	if( motor == 0 ){
		return util_ErrTd_Null;
	}

	return oh_SetRange(motor->Ctrl, valMin, valMax);
}


util_ErrTd mc_SetDirInv(mc_MotorTd *motor, mc_DirTd newDir){
	if( motor == 0 ){
		return util_ErrTd_InvalidParam;
	}

	motor->DirInv = newDir;
	return util_ErrTd_Ok;
}


mc_RegTypeTd mc_GetRegTypeByName(char* n){
	if( ( n[0] == 'p' ) || ( n[0] == mc_RegTypeTd_Pos ) ){
		return mc_RegTypeTd_Pos;
	}
	else if( ( n[0] == 's' ) || ( n[0] == mc_RegTypeTd_Speed ) ){
		return mc_RegTypeTd_Speed;
	}
	else {
		return mc_RegTypeTd_None;
	}
}

util_ErrTd mc_MoveEx(mc_MotorTd *motor, mc_SetPointTd *setPoint, mc_TermTd* termCond, int ackPos){
	mc_MotSensRegSettingsTd* RegSettings;

	if( ( motor == 0 ) || ( setPoint == 0 ) ){
		return util_ErrTd_InvalidParam;
	}

	//Find regulator settings and apply them on motor structure
	RegSettings = mc_GetRegSettingsForMotSens(motor, setPoint->RegCtrl);
	if( RegSettings ){
		if( setPoint->RegType == mc_RegTypeTd_Pos ){
			motor->Reg = &(RegSettings->PosReg);

			if(setPoint->IsRelative){
				motor->SetPoint.TargetVal = setPoint->RegCtrl->FilteredVal + setPoint->TargetVal;
			}
			else{
				motor->SetPoint.TargetVal = setPoint->TargetVal;
			}
		}
		else if( setPoint->RegType == mc_RegTypeTd_Speed ){
			motor->Reg = &(RegSettings->SpeedReg);

			if(setPoint->IsRelative){
				motor->SetPoint.TargetVal = setPoint->RegCtrl->FilteredValSpeed + setPoint->TargetVal;
			}
			else{
				motor->SetPoint.TargetVal = setPoint->TargetVal;
			}
		}



		motor->SetPoint.Val = setPoint->Val;	//Target value is the same as current value, no acceleration takes place
		motor->SetPoint.Delta = setPoint->Delta;
		motor->SetPoint.RegCtrl = setPoint->RegCtrl;
		motor->SetPoint.RegType = setPoint->RegType;
		ih_ResetFilteredMinMax(setPoint->RegCtrl);
		util_ResetPid(motor->Reg);

		//Check if termination condition is specified and copy values. If not disable it in motor structure.
		if( termCond != 0 ){
			memcpy( &(motor->Termination), termCond, sizeof(mc_TermTd) );
			ih_ResetFilteredMinMax(motor->Termination.Cond1Ctrl);
			ih_ResetFilteredMinMax(motor->Termination.Cond2Ctrl);
		}
		else{
			motor->Termination.Logic = mc_CondLogicTd_OR;

			motor->Termination.Cond1Ctrl = 0;
			motor->Termination.Cond1Mode = 0;
			motor->Termination.Cond1Val = 0;

			motor->Termination.Cond2Ctrl = 0;
			motor->Termination.Cond2Mode = 0;
			motor->Termination.Cond2Val = 0;

			motor->Termination.TermSetPoint.IsRelative = 0;
			motor->Termination.TermSetPoint.RegCtrl = 0;
			motor->Termination.TermSetPoint.RegType = 0;
			motor->Termination.TermSetPoint.Val = 0;
			motor->Termination.TermSetPoint.TargetVal = 0;
			motor->Termination.TermSetPoint.Delta = 0;
		}
	}
	else{
		motor->SetPoint.RegCtrl = 0;
		motor->SetPoint.RegType = 0;
		motor->SetPoint.Val = 0;
		motor->SetPoint.IsRelative = 0;

		motor->Termination.Logic = mc_CondLogicTd_OR;

		motor->Termination.Cond1Ctrl = 0;
		motor->Termination.Cond1Mode = 0;
		motor->Termination.Cond1Val = 0;

		motor->Termination.Cond2Ctrl = 0;
		motor->Termination.Cond2Mode = 0;
		motor->Termination.Cond2Val = 0;

		motor->Termination.TermSetPoint.IsRelative = 0;
		motor->Termination.TermSetPoint.RegCtrl = 0;
		motor->Termination.TermSetPoint.RegType = 0;
		motor->Termination.TermSetPoint.Val = 0;
		motor->Termination.TermSetPoint.TargetVal = 0;
		motor->Termination.TermSetPoint.Delta = 0;

		return util_ErrTd_Uninitialized;
	}

	if( ackPos ){
		motor->Termination.DestAck = 1;
	}
	else{
		motor->Termination.DestAck = 0;
	}

	return util_ErrTd_Ok;
}


util_ErrTd mc_MovePosSpe(mc_MotorTd *motor, ih_HandleTd *sens, float pos, float spe, int holdPos, int ackPos){
	mc_SetPointTd SetP = {0};
	mc_TermTd TermC = {0};


	if( motor == 0 ){
		return util_ErrTd_InvalidParam;
	}
	else if( sens == 0 ){
		mc_SetThrottle(motor, 0);
	}

	if( spe < 0 ){
		spe *= -1;
	}


	//Setup setpoint and adjust speed sign according to where motor must move to reach desired position
	SetP.IsRelative = 0;
	SetP.RegCtrl = sens;
	SetP.RegType = mc_RegTypeTd_Speed;

	TermC.Cond1Ctrl = sens;
	TermC.Cond1Val = pos;
	TermC.Cond2Ctrl = 0;
	TermC.Cond2Mode = 0;
	TermC.Cond2Val = 0;
	TermC.Logic = mc_CondLogicTd_OR;

	if( sens->FilteredVal > pos ){
		SetP.Val = -1 * spe;
		SetP.TargetVal = SetP.Val;
		SetP.Delta = 0;
		TermC.Cond1Mode = mc_CondModeTd_Below;
	}
	else{
		SetP.Val = spe;
		SetP.TargetVal = SetP.Val;
		SetP.Delta = SetP.Val;
		TermC.Cond1Mode = mc_CondModeTd_Above;
	}

	if( holdPos ){
		TermC.TermSetPoint.RegCtrl = sens;
		TermC.TermSetPoint.RegType = mc_RegTypeTd_Pos;
		TermC.TermSetPoint.Val = pos;
		TermC.TermSetPoint.IsRelative = 0;
	}
	else{
		TermC.TermSetPoint.RegCtrl = 0;
		TermC.TermSetPoint.RegType = 0;
		TermC.TermSetPoint.Val = 0;
		TermC.TermSetPoint.IsRelative = 0;
	}

	return mc_MoveEx(motor, &SetP, &TermC, ackPos);
}



util_ErrTd mc_MovePosSpeAcc(mc_MotorTd *motor, ih_HandleTd *sens, float pos, float spe, float acc, float dec, int ackPos){
	mc_SetPointTd SetP = {0};
	mc_TermTd TermC = {0};
	float BreakDist = 0;


	if( motor == 0 ){
		return util_ErrTd_Null;
	}
	else if( sens == 0 ){
		mc_SetThrottle(motor, 0);
	}

	spe = abs(spe);
	acc = abs(acc);
	dec = abs(dec);

	//Setup setpoint and adjust speed sign according to where motor must move to reach desired position
	if( motor->SetPoint.RegType == mc_RegTypeTd_Speed ){	//If speed regulation is active we know actual value of speed
		SetP.Val = motor->SetPoint.Val;
	}
	else{
		SetP.Val = 0;
	}

	SetP.IsRelative = 0;
	SetP.RegCtrl = sens;
	SetP.RegType = mc_RegTypeTd_Speed;


	BreakDist = 0.5 * dec * (spe / dec);				//s=0.5*a*t2; t=deltav/a
	TermC.Cond1Ctrl = sens;


	//Get direction of speed to reach specified destination and set terminating condition to the point when deceleration should take place
	if( sens->FilteredVal > pos ){
		SetP.TargetVal = -1 * spe;
		TermC.Cond1Mode = mc_CondModeTd_Below;
		TermC.Cond1Val = pos + BreakDist;
	}
	else{
		SetP.TargetVal = spe;
		TermC.Cond1Mode = mc_CondModeTd_Above;
		TermC.Cond1Val = pos - BreakDist;
	}

	//Get increment sign to reach desired value

	if( SetP.TargetVal - SetP.Val > 0 ){
		SetP.Delta = +0.001 * acc;										//Increment per ms, but parameter unit is per sec.
	}
	else if( SetP.TargetVal - SetP.Val < 0 ){
		SetP.Delta = -0.001 * acc;										//Increment per ms, but parameter unit is per sec.
	}


	TermC.TermSetPoint.RegCtrl = sens;
	TermC.TermSetPoint.RegType = mc_RegTypeTd_Speed;
	TermC.TermSetPoint.Val = SetP.TargetVal;
	if( SetP.TargetVal > 0 ){
		TermC.TermSetPoint.Delta = -0.001 * dec;
	}
	else{
		TermC.TermSetPoint.Delta = +0.001 * dec;
	}


	TermC.TermSetPoint.TargetVal = 0;
	TermC.TermSetPoint.IsRelative = 0;


	return mc_MoveEx(motor, &SetP, &TermC, ackPos);
}



util_ErrTd mc_StopAll(void){
	int i;
	mc_SetPointTd sp = {0};

	for( i=0; i<UTIL_SIZEOFARRAY(mc_MotorList); i++){
		mc_MoveEx(mc_MotorList[i], &sp, 0, 0);
	}
	return util_ErrTd_Ok;
}


mc_MotorTd* mc_GetMotorByName(char* n){		// m1 ... motor1, m2, m3, m4
	uint32_t i;

	for( i=0; i<UTIL_SIZEOFARRAY(mc_MotorList); i++ ){
		if( strcmp((char*)mc_MotorList[i]->Name, (char*)n) == 0 ){
			return mc_MotorList[i];
		}
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////
////////// TERMINATING CONDITIONS ////////////////////////////////////
//////////////////////////////////////////////////////////////////////

struct{
	char* Name;
	mc_CondLogicTd Logic;
} mc_CondLogicList[] = {
	{"and", mc_CondLogicTd_AND},
	{"or", mc_CondLogicTd_OR},
};


struct{
	char* Name;
	mc_CondModeTd Mode;
} mc_CondList[] = {
	{"non", mc_CondModeTd_None},
	{"iab", mc_CondModeTd_Above},
	{"ibe", mc_CondModeTd_Below},
	{"wab", mc_CondModeTd_WasAbove},
	{"wbe", mc_CondModeTd_WasBelow},
	{"pmax", mc_CondModeTd_PercMax},
	{"pmin", mc_CondModeTd_PercMin},
};


mc_CondModeTd mc_GetCondModeByName(char *n){
	int i;

	for( i=0; i<UTIL_SIZEOFARRAY(mc_CondList); i++ ){
		if( strcmp(n,mc_CondList[i].Name) == 0 ){
			return mc_CondList[i].Mode;
		}
	}
	return mc_CondModeTd_None;
}


mc_CondModeTd mc_GetCondLogicByName(char *n){
	int i;

	for( i=0; i<UTIL_SIZEOFARRAY(mc_CondLogicList); i++ ){
		if( strcmp(n,mc_CondLogicList[i].Name) == 0 ){
			return mc_CondLogicList[i].Logic;
		}
	}
	return mc_CondLogicTd_OR;
}


int mc_EvalCond(ih_HandleTd *sens, mc_CondModeTd mode, float limit){
	if( sens == 0 ){
		return 0;
	}

	switch(mode){
		case mc_CondModeTd_None:
			return 0;
		break;

		case mc_CondModeTd_WasAbove:
			if( ( sens->FilteredValMax ) > limit ){
				return 1;
			}
		break;

		case mc_CondModeTd_WasBelow:
			if( ( sens->FilteredValMin ) < limit ){
				return 1;
			}
		break;

		case mc_CondModeTd_Above:
			if(  ( sens->FilteredVal ) > limit ){
				return 1;
			}
		break;

		case mc_CondModeTd_Below:
			if( ( sens->FilteredVal ) < limit ){
				return 1;
			}
		break;

		case mc_CondModeTd_PercMax:
			if( ( sens->FilteredValMax - sens->FilteredVal ) > ( sens->FilteredValMax * limit * 0.01 ) ){
				return 1;
			}
		break;

		case mc_CondModeTd_PercMin:
			if( ( sens->FilteredVal - sens->FilteredValMin ) > ( sens->FilteredValMin * limit * 0.01 ) ){
				return 1;
			}
		break;

		default:
			return 0;
		break;
	}
	return 0;
}



int mc_EvalTermCond(mc_MotorTd *motor){
	int Cond1 = 0, Cond2 = 0;

	if( motor == 0 ){
		return 0;
	}
	Cond1 = mc_EvalCond(motor->Termination.Cond1Ctrl, motor->Termination.Cond1Mode, motor->Termination.Cond1Val);
	Cond2 = mc_EvalCond(motor->Termination.Cond2Ctrl, motor->Termination.Cond2Mode, motor->Termination.Cond2Val);
	motor->Termination.Cond1Status = Cond1;
	motor->Termination.Cond2Status = Cond2;

	if( motor->Termination.Logic == mc_CondLogicTd_AND ){
		return ( Cond1 && Cond2 );
	}
	else if( motor->Termination.Logic == mc_CondLogicTd_OR ){
		return ( Cond1 || Cond2 );
	}
	else{
		return 0;
	}
}


//////////////////////////////////////////////////////////////////////
////////// MOTOR REPORTS /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void mc_SendMotorStatus(mc_MotorTd *motor){
	if( motor == 0 ){
		return;
	}
	if( osMutexWait(com_MutexId, osWaitForever) == osOK ){		//Get exclusive access
		sprintf((char*)StringBufer, "<mte %d %s %d %d>\r\n", bl_GetTick(), (char*)(motor->Name), motor->Termination.Cond1Status, motor->Termination.Cond2Status);
		com_SendData(StringBufer, strlen((char*)StringBufer));
		osMutexRelease(com_MutexId);
	}
}


//////////////////////////////////////////////////////////////////////
////////// RTOS TASK /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void mc_Task(void const *argument) {
	uint32_t i;
	float RegVal;

	mc_StopAll();																				//Stop all motors


	for (;;){																					//Infinite regulation loop
		for( i=0; i<UTIL_SIZEOFARRAY(mc_MotorList); i++){										//For each motor in motorlist

			if( mc_MotorList[i] == 0 ){															//Check if valid pointer to motor is in list
				continue;																		//And if not just continue to next motor in list
			}

			if(mc_MotorList[i]->SetPoint.RegCtrl == 0){											//Check if motor has regulation sensor
				if( mc_MotorList[i]->SetPoint.RegType != mc_RegTypeTd_External ){				//If not it is error unless external regulation is set
					mc_SetThrottle(mc_MotorList[i], 0);
					continue;
				}
			}

			if( mc_EvalTermCond(mc_MotorList[i]) == 1 ){										//Check termination condition if exists
				if( mc_MotorList[i]->Termination.DestAck ){
					mc_SendMotorStatus(mc_MotorList[i]);										//Report termination and condition status
				}
				mc_MoveEx(mc_MotorList[i], &(mc_MotorList[i]->Termination.TermSetPoint), 0, 0);	//If termination condition applies change regulation according to setup
			}



			//Handle desired value increments or decrements
			mc_MotorList[i]->SetPoint.Val += mc_MotorList[i]->SetPoint.Delta;
			if( mc_MotorList[i]->SetPoint.Delta > 0 && mc_MotorList[i]->SetPoint.Val > mc_MotorList[i]->SetPoint.TargetVal ){		//If desired value is incrementing and curent desired value is already greater than target value
				mc_MotorList[i]->SetPoint.Val = mc_MotorList[i]->SetPoint.TargetVal;
			}
			else if( mc_MotorList[i]->SetPoint.Delta < 0 && mc_MotorList[i]->SetPoint.Val < mc_MotorList[i]->SetPoint.TargetVal ){	//If desired value is decrementing
				mc_MotorList[i]->SetPoint.Val = mc_MotorList[i]->SetPoint.TargetVal;
			}


			//According to regulation type (speed or position) calculate regulation value and apply it to motor
			switch ( mc_MotorList[i]->SetPoint.RegType ){
				case mc_RegTypeTd_Pos:
					RegVal = util_UpdatePid(mc_MotorList[i]->Reg, mc_MotorList[i]->SetPoint.Val, mc_MotorList[i]->SetPoint.RegCtrl->FilteredVal);
				break;

				case mc_RegTypeTd_Speed:
					RegVal = util_UpdatePid(mc_MotorList[i]->Reg, mc_MotorList[i]->SetPoint.Val, mc_MotorList[i]->SetPoint.RegCtrl->FilteredValSpeed);
				break;

				case mc_RegTypeTd_External:
					continue;																	//If external regulation is set throttle is regulated elsewhere so skip to next motor
				break;

				default:
					RegVal = 0;
				break;
			}
			mc_SetThrottle(mc_MotorList[i], RegVal);
		}
		osDelay(1);
	}
}
