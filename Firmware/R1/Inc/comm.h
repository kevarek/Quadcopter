/*
*Filename: comm.h
*Author: Stanislav Subrt
*Year: 2013
*/

#ifndef COMM_H
#define COMM_H

extern void comm_Init(void);
extern void comm_SendData(char *dataToSend, int lengthInBytes);
extern void comm_HandleReceivedData(void);

extern int comm_IntToStr(int desiredInt, char* destStr, int addTermZero);


#endif  //end of COMM_H
