/*
*Filename: slidavg.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "slidavg.h"


void slidavg_Init(slidavg_BufferStructTypedef* bufStruct, float *floatBuffer, int  cnt){
	int i;
	( bufStruct->DataBuffer ) = floatBuffer;
	( bufStruct->DataCnt ) = cnt;
	for( i=0; i<cnt; i++) (floatBuffer[i]) = 0;
}

void slidavg_Reset(slidavg_BufferStructTypedef* bufStruct){
	int i;
	for( i=0; i<(bufStruct->DataCnt); i++) ( bufStruct->DataBuffer[i] ) = 0;
}

void slidavg_AddVal(slidavg_BufferStructTypedef* bufStruct, float valToAdd){
 int i;

 for( i=1; i<(bufStruct->DataCnt); i++){
	(bufStruct->DataBuffer)[i-1] = (bufStruct->DataBuffer)[i];
 }
 (bufStruct->DataBuffer)[i-1] = valToAdd;
}


float slidavg_GetAvg(slidavg_BufferStructTypedef* bufStruct){
	int i;
	float Sum = 0;;
	for( i=0; i<(bufStruct->DataCnt); i++){
		Sum += (bufStruct->DataBuffer)[i];
	}
	Sum /= i;
	return Sum;
}
