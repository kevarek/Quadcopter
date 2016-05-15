/*
*Filename: 	outhub.c
*Author: 	Ing. Stanislav Subrt
*Year:		2016
*/

/* DESCRIPTION

*/

#include "outhub.h"
#include "leds.h"


oh_HandleTd GreenLed = {"GreenLED", 0, leds_SetGreenLed, 0, {0, 1} };


oh_HandleTd* oh_List[] = {
	&GreenLed,
};


util_ErrTd oh_Init(void){
	int32_t i;

	for( i=0; i< UTIL_SIZEOFARRAY(oh_List); i++ ){
		oh_SetMode(oh_List[i], util_OutModeTd_None, 0, 0);
		oh_SetState(oh_List[i], util_StateTd_On);
		oh_SetVal(oh_List[i], 0);
	}

	return util_ErrTd_Ok;
}


util_ErrTd oh_SetMode(oh_HandleTd* h, util_OutModeTd newMode, float param1, float param2){
	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( h->SetModeFn == 0 ){
		h->Mode = util_OutModeTd_None;
		return util_ErrTd_NotSupported;
	}
	else{
		h->Mode = newMode;
		return h->SetModeFn(newMode, param1, param2);
	}


}


util_ErrTd oh_GetMode(oh_HandleTd* h, util_OutModeTd* m){
	if( h == 0 || m == 0 ){
		return util_ErrTd_Null;
	}

	*m = h->Mode;
	return util_ErrTd_Ok;
}


util_ErrTd oh_SetState(oh_HandleTd* h, util_StateTd newState){
	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( h->SetStateFn == 0 ){
		h->State = util_StateTd_NotInitialized;
		return util_ErrTd_Null;
	}

	h->SetStateFn(newState);
	h->State = newState;
	return util_ErrTd_Ok;
}


util_ErrTd oh_GetState(oh_HandleTd* h, util_StateTd* s){
	if( h == 0 || s == 0 ){
		return util_ErrTd_Null;
	}

	*s = h->State;
	return util_ErrTd_Ok;
}


util_ErrTd oh_SetVal(oh_HandleTd* h, float newVal){
	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( h->SetValFn == 0 ){
		return util_ErrTd_Null;
	}

	if( newVal < h->OutputRange[0] ){
		newVal = h->OutputRange[0];
	}
	else if( newVal > h->OutputRange[1] ){
		newVal = h->OutputRange[1];
	}

	h->SetValFn(newVal);
	h->Value = newVal;
	return util_ErrTd_Ok;
}


util_ErrTd oh_GetVal(oh_HandleTd* h, float* val){
	if( h == 0 || val == 0 ){
		return util_ErrTd_Null;
	}

	*val = h->Value;
	return util_ErrTd_Ok;
}


util_ErrTd oh_SetRange(oh_HandleTd* h, float newMin, float newMax){
	if( h == 0 ){
		return util_ErrTd_Null;
	}

	if( UTIL_ISNAN(newMin) || UTIL_ISNAN(newMax) ){
		return util_ErrTd_NotValid;
	}

	h->OutputRange[0] = newMin;
	h->OutputRange[1] = newMax;
	return util_ErrTd_Ok;
}


util_ErrTd oh_GetRange(oh_HandleTd* h, float* retMin, float* retMax){
	if( h == 0 || retMin == 0 || retMax == 0 ){
		return util_ErrTd_Null;
	}

	*retMin = h->OutputRange[0];
	*retMax = h->OutputRange[1];
	return util_ErrTd_Ok;
}
