/*
*Filename: slidavg.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef SLIDAVG_H
#define SLIDAVG_H




typedef struct{
	float *DataBuffer;
	int DataCnt;
} slidavg_BufferStructTypedef;

extern void slidavg_Init(slidavg_BufferStructTypedef* bufStruct, float *floatBuffer, int  cnt);
extern void slidavg_Reset(slidavg_BufferStructTypedef* bufStruct);
extern void slidavg_AddVal(slidavg_BufferStructTypedef* bufStruct, float valToAdd);
extern float slidavg_GetAvg(slidavg_BufferStructTypedef* bufStruct);


#endif  //end of XXX_H
