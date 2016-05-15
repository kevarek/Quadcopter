/*
*Filename: comm.c
*Author: Stanislav Subrt
*Year: 2013
*/

#include "comm.h"

#include "stm32f0xx.h"
#include "board.h"
#include "circbuffer.h"
#include "engines.h"
#include "stabilisation.h"
#include "perftests.h"
#include "sensors.h"
#include "psd.h"
#include "blinky.h"


#define USART1BUFSIZE	128
//RX buffer
static char USART1RXBuf[USART1BUFSIZE];
CircBufferStructTypedef USART1RXBufStruct;
//TX buffer
static char USART1TXBuf[USART1BUFSIZE];
CircBufferStructTypedef USART1TXBufStruct;
//command buffer
static char CmdRXBuffer[USART1BUFSIZE];


#define USART1_TXPIN    GPIO_Pin_2
#define USART1_TXPINSOURCE  GPIO_PinSource2
#define USART1_RXPIN    GPIO_Pin_3
#define USART1_RXPINSOURCE  GPIO_PinSource3
#define USART1_PORT     GPIOA
void comm_Init(void){
    NVIC_InitTypeDef NVIC_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;


	//init RX and TX circular buffers
	CB_Init(&USART1RXBufStruct, USART1RXBuf, USART1BUFSIZE, '\n');
	CB_Init(&USART1TXBufStruct, USART1TXBuf, USART1BUFSIZE, '\n');

    //enable the gloabal Interrupt
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    //configure gpio pins for usart
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStruct.GPIO_Pin  = USART1_TXPIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(USART1_PORT, &GPIO_InitStruct);
    GPIO_PinAFConfig(USART1_PORT, USART1_TXPINSOURCE, GPIO_AF_1);

    GPIO_InitStruct.GPIO_Pin  = USART1_RXPIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(USART1_PORT, &GPIO_InitStruct);
    GPIO_PinAFConfig(USART1_PORT, USART1_RXPINSOURCE, GPIO_AF_1);

    USART_InitStruct.USART_BaudRate = 57600;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStruct);
	USART_Cmd(USART1, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);							//enable RX not empty interrupt
	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
}


void comm_SendData(char *dataToSend, int lengthInBytes){

	CB_AddMultiple(&USART1TXBufStruct, dataToSend, lengthInBytes);
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);	//enable TX empty interrupt which will send all data
}


/*
void comm_DiscardRXBuffer(void){
	CB_RemoveExisting(&USART1RXBufStruct, 0);
}
*/


#define COMM_CHARTOINT_INVALID_CHAR_PTR	-1
#define COMM_CHARTOINT_CHAR_OUT_OF_RANGE	-2
#define COMM_CHARTOINT_SIGNPLUS		-3
#define COMM_CHARTOINT_SIGNMINUS	-4
int comm_CharToInt(char *pChar){
	if(pChar == 0) return COMM_CHARTOINT_INVALID_CHAR_PTR;

	if(*pChar == '+') {
		return COMM_CHARTOINT_SIGNPLUS;
	}
	else if(*pChar == '-'){
		return COMM_CHARTOINT_SIGNMINUS;
	}
	else if(*pChar > '9' || *pChar < '0'){
		return COMM_CHARTOINT_CHAR_OUT_OF_RANGE;
	}
	else{
		return *pChar - '0';
	}
}


int comm_StrToInt(char* desiredStr, int strLength){
	int i = strLength, d = 1;
	int Result = 0;
	int ResultSign = 1;
	int CharValue;
	while( --i >= 0 ){
		CharValue = comm_CharToInt(desiredStr+i);
		if( CharValue >= 0 ){
			Result += CharValue * d;
			d *= 10;
		}
		else if( CharValue == COMM_CHARTOINT_SIGNPLUS ) ResultSign = 1;
		else if( CharValue == COMM_CHARTOINT_SIGNMINUS ) ResultSign = -1;
		//i--;
	}
	Result *= ResultSign;
	return Result;
}


//returns number of chars written
int comm_IntToStr(int desiredInt, char* destStr, int addTermZero){
	int i, j, StringIndex = 0, FirstNonZeroCharFound = 0;
	int Dividers[] = {1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};
	int NumberOfDividers = sizeof(Dividers) / sizeof(Dividers[0]);

	//save sign value and make desired int positive
	//todo handle exception when converting max negative number to positive
	if( desiredInt>0 ){
		destStr[StringIndex++] = '+';
	}
	else if(desiredInt<0){
		destStr[StringIndex++] = '-';
		desiredInt *= -1;
	}
	else {
		destStr[StringIndex++] = '0';
		if(addTermZero) destStr[StringIndex++] = '\0';
		return StringIndex;
	}

	for( i=0; i<NumberOfDividers; i++ ){
		j = 0;
		while ( (desiredInt-=Dividers[i]) >= 0 ){
			j++;
		}
		desiredInt += Dividers[i];	//resume state before zero underflow
		if( (j!=0) || (FirstNonZeroCharFound) ) {
			FirstNonZeroCharFound = 1;
			destStr[StringIndex++] = '0' + j;	//save char to string if it is not leading zero
		}
	}
	if(addTermZero) destStr[StringIndex++] = '\0';
	return StringIndex;
}


#define COMM_MANUAL_PACKET_AXIS_VALUE_RANGE	(float)10000
#define COMM_MANUAL_PACKET_THROTTLE_COEF	10
void comm_HandleReceivedData(void){
	volatile float XAxis, YAxis, ZAxis, Throttle;
	volatile int Btn1, Btn2, Btn3, Btn4;
	char FirstCharOfPacket, SecondCharOfPacket;
	char tmp[100];
	int i;
	if( CB_IsSpecialCharReceived(&USART1RXBufStruct) ){
		CB_RemoveExistingUpToSpecChar(&USART1RXBufStruct, CmdRXBuffer);
		FirstCharOfPacket = CmdRXBuffer[0];
		SecondCharOfPacket = CmdRXBuffer[1];
		switch (FirstCharOfPacket){
			case '\r':	//emergency break
				stabilisation_EmergencyStop();
			break;

			case 'M':
				// "M+10000-10000+00000-000001234";
				XAxis = (float)(comm_StrToInt(CmdRXBuffer+1, 6)) / COMM_MANUAL_PACKET_AXIS_VALUE_RANGE;
				YAxis = (float)(comm_StrToInt(CmdRXBuffer+7, 6)) / COMM_MANUAL_PACKET_AXIS_VALUE_RANGE;
				ZAxis = (float)(comm_StrToInt(CmdRXBuffer+13, 6)) / COMM_MANUAL_PACKET_AXIS_VALUE_RANGE;
				Throttle = (float)(comm_StrToInt(CmdRXBuffer+19, 6)) * COMM_MANUAL_PACKET_THROTTLE_COEF;

				Btn1 = comm_StrToInt(CmdRXBuffer+25, 1);
				Btn2 = comm_StrToInt(CmdRXBuffer+26, 1);
				Btn3 = comm_StrToInt(CmdRXBuffer+27, 1);
				Btn4 = comm_StrToInt(CmdRXBuffer+28, 1);

				if(Btn1 == 1){
					stabilisation_CancelEmergencyStop();

					stabilisation_SetDesiredThrust(Throttle);
					stabilisation_IncrementDesiredYawAngle( -1 * ZAxis );
					stabilisation_SetDesiredPitchAngle( XAxis * 15 );
					stabilisation_SetDesiredRollAngle( YAxis * 15);
				}
				else{
					stabilisation_EmergencyStop();
					if( Btn2 == 1){
						NVIC_SystemReset();
					}

				if(Btn4 == 1)
					comm_SendData(INFOSTRING, sizeof(INFOSTRING));
				}
			break;


			//PSD tunning for pitch and roll
			case 'i':
				comm_SendData(INFOSTRING, sizeof(INFOSTRING) );
			break;

			case 'p':
				psd_IncrementConstants(&PitchPSDStruct, 1, 0, 0);
				psd_IncrementConstants(&RollPSDStruct, 1, 0, 0);
			break;

			case 'l':
				psd_IncrementConstants(&PitchPSDStruct, -1, 0, 0);
				psd_IncrementConstants(&RollPSDStruct, -1, 0, 0);
			break;

			case 'ú':
				psd_IncrementConstants(&PitchPSDStruct, 0, 0.1, 0);
				psd_IncrementConstants(&RollPSDStruct, 0, 0.1, 0);
			break;

			case 'ù':
				psd_IncrementConstants(&PitchPSDStruct, 0, -0.1, 0);
				psd_IncrementConstants(&RollPSDStruct, 0, -0.1, 0);
			break;

			case ')':
				psd_IncrementConstants(&PitchPSDStruct, 0, 0, 0.01);
				psd_IncrementConstants(&RollPSDStruct, 0, 0, 0.01);
			break;

			case '§':
				psd_IncrementConstants(&PitchPSDStruct, 0, 0, -0.01);
				psd_IncrementConstants(&RollPSDStruct, 0, 0, -0.01);
			break;


			//PSD tunning for yaw
			case 's':
				psd_IncrementConstants(&YawPSDStruct, 1, 0, 0);
			break;

			case 'y':
				psd_IncrementConstants(&YawPSDStruct, -1, 0, 0);
			break;

			case 'd':
				psd_IncrementConstants(&YawPSDStruct, 0, 0.1, 0);
			break;

			case 'x':
				psd_IncrementConstants(&YawPSDStruct, 0, -0.1, 0);
			break;

			case 'f':
				psd_IncrementConstants(&YawPSDStruct, 0, 0, 0.1);
			break;

			case 'c':
				psd_IncrementConstants(&YawPSDStruct, 0, 0, -0.1);
			break;

			case 'g':
				switch (SecondCharOfPacket){
					case 'p':
						comm_SendData("p: ", 3);

						i = comm_IntToStr((int)(PitchPSDStruct.P * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData(", s: ", 5);
						i = comm_IntToStr((int)(PitchPSDStruct.S * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData(", d: ", 5);
						i = comm_IntToStr((int)(PitchPSDStruct.D * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData("\r\n", 2);
					break;

					case 'y':
						comm_SendData("p: ", 3);

						i = comm_IntToStr((int)(YawPSDStruct.P * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData(", s: ", 5);
						i = comm_IntToStr((int)(YawPSDStruct.S * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData(", d: ", 5);
						i = comm_IntToStr((int)(YawPSDStruct.D * 100), tmp, 0);
						comm_SendData(tmp, i);

						comm_SendData("\r\n", 2);
					break;

				}

			break;
		}
	}
}



//////////////////////////////////////////////////////////////////////
////////////////////////// PRIVATE FUNCTIONS /////////////////////////
//////////////////////////////////////////////////////////////////////

void USART1_IRQHandler(void){
	int tmp;
	char ReceivedChar;
	//Handle received char
	if(USART_GetITStatus(USART1, USART_IT_RXNE)==SET){						//RX Not Empty interrupt
		ReceivedChar = (char)USART_ReceiveData(USART1);
		CB_Add(&USART1RXBufStruct, ReceivedChar);
	}

	//handle char to transmit
	if(USART_GetITStatus(USART1, USART_IT_TXE)==SET){				//TX empty
		tmp = CB_RemoveTail(&USART1TXBufStruct);					//read character to send
		if( tmp != CB_BUFFER_EMPTY){	//if TX buffer contains chars to send
			USART_SendData(USART1, (char)tmp);
		}
		else{	//tx buffer is empty
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			USART_ClearITPendingBit(USART1, USART_IT_TXE);					//TXE int is cleared by SendData function or here manually
		}
	}

}
