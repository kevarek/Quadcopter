/*
*Filename: com.c
*Author: Stanislav Subrt
*Year: 2014
*/

#include "main.h"
#include "com.h"
#include "gamepad.h"
#include "stabilisation.h"
#include "I2C.h"
#include "engines.h"
#include "settings.h"

//////////////////////////////////////////////////////////////////////
////////// SETTINGS //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define RX_TIMEOUT_MS				1000
#define BTCOMDEBUG	0

//////////////////////////////////////////////////////////////////////
////////// HARDWARE ABSTRACTION LAYER ////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define PORT_INST     				GPIOA
#define PORT_ENPERCLK				__GPIOA_CLK_ENABLE
#define PIN_TX    					GPIO_PIN_9
#define PIN_RX    					GPIO_PIN_10
#define PORT_AF						GPIO_AF7_USART1

#define USART_INST					USART1
#define USART_ENPERCLK				__USART1_CLK_ENABLE
#define USART_BAUDRATE				115200
#define USART_IRQN					USART1_IRQn
#define USART_IRQH					USART1_IRQHandler

#define DMA_INST					DMA1
#define DMA_ENPERCLK				__DMA1_CLK_ENABLE

#define DMA_TX						DMA1_Channel4
#define DMA_TX_IRQN					DMA1_Channel4_IRQn
#define DMA_TX_IRQH					DMA1_Channel4_IRQHandler

#define DMA_RX						DMA1_Channel5
#define DMA_RX_IRQN					DMA1_Channel5_IRQn
#define DMA_RX_IRQH					DMA1_Channel5_IRQHandler

#define RXBUFSIZE					1024								//Must be power of two because of some math magic
#define RXBUFTAILMASK				( RXBUFSIZE - 1 )
#define SINGLECMDBUFSIZE			256
#define ENDOFCMDCHAR				'~'

#define TXBUFSIZE					1024								//Must be power of two because of some math magic
#define TXBUFHEADMASK				( TXBUFSIZE - 1 )


static uint8_t	RxBuffer[RXBUFSIZE];
static uint32_t RxTail = 0;
static uint32_t RxCmdCnt = 0;
static uint8_t Cmd[SINGLECMDBUFSIZE];

static uint8_t TxBuffer[TXBUFSIZE];
static uint32_t TxHead = 0;
static uint32_t TxTail = 0;

static UART_HandleTypeDef ComPort;
static DMA_HandleTypeDef hDMA_TX;

static int32_t BtnDebounce = 0;
static gp_ParsedDataTd GamepadStatus;
static bl_TimeoutTd RxTimeout;


//////////////////////////////////////////////////////////////////////
////////// TIMEOUT CALLBACKS /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void RxTimeoutCallback(void){
	eng_StopAll();
}


//////////////////////////////////////////////////////////////////////
////////// INITIALIZATION ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//Initializes communication peripheral and buffers
void com_Init(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	DMA_HandleTypeDef hDMA_RX;

	#if BTCOMDEBUG
	#else
	bl_InitTimeout(&RxTimeout, RX_TIMEOUT_MS, RxTimeoutCallback, 0);
	#endif

	//Configure GPIO pins for UART
	PORT_ENPERCLK();
	GPIO_InitStructure.Pin = PIN_TX | PIN_RX;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Alternate = PORT_AF;
	HAL_GPIO_Init(PORT_INST, &GPIO_InitStructure);

	//Configure USART peripheral
	USART_INST->CR2 = 0xFF000000;					//I wish I could remember what this line was supposed to do then ... :)
	USART_ENPERCLK();
	ComPort.Instance = USART_INST;
	ComPort.Init.BaudRate = USART_BAUDRATE;
	ComPort.Init.WordLength = UART_WORDLENGTH_8B;
	ComPort.Init.StopBits = UART_STOPBITS_1;
	ComPort.Init.Parity = UART_PARITY_NONE;
	ComPort.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	ComPort.Init.Mode = UART_MODE_TX_RX;
	HAL_UART_Init(&ComPort);

	//Set character match and disable error interrupts (this can be done only while Receiver is enabled and USART is disabled (RE=1, UE=0)
	__HAL_UART_DISABLE(&ComPort);
	//__HAL_UART_DISABLE_IT(&ComPort, UART_IT_ERR);
	MODIFY_REG(USART_INST->CR3, USART_CR3_OVRDIS, USART_CR3_OVRDIS);
	MODIFY_REG(USART_INST->CR2, USART_CR2_ADD, ((unsigned)ENDOFCMDCHAR << UART_CR2_ADDRESS_LSB_POS));
	__HAL_UART_ENABLE(&ComPort);

	//Configure DMA for TX and RX
	DMA_ENPERCLK();
	hDMA_TX.Instance = DMA_TX;
	hDMA_TX.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hDMA_TX.Init.PeriphInc = DMA_PINC_DISABLE;
	hDMA_TX.Init.MemInc = DMA_MINC_ENABLE;
	hDMA_TX.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hDMA_TX.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hDMA_TX.Init.Mode = DMA_NORMAL;
	hDMA_TX.Init.Priority = DMA_PRIORITY_MEDIUM;
	HAL_DMA_Init(&hDMA_TX);
	__HAL_LINKDMA(&ComPort, hdmatx, hDMA_TX);
	HAL_NVIC_SetPriority(DMA_TX_IRQN, 3, 0);
	HAL_NVIC_EnableIRQ(DMA_TX_IRQN);

	hDMA_RX.Instance = DMA_RX;
	hDMA_RX.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hDMA_RX.Init.PeriphInc = DMA_PINC_DISABLE;
	hDMA_RX.Init.MemInc = DMA_MINC_ENABLE;
	hDMA_RX.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hDMA_RX.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	hDMA_RX.Init.Mode = DMA_CIRCULAR;
	hDMA_RX.Init.Priority = DMA_PRIORITY_MEDIUM;
	HAL_DMA_Init(&hDMA_RX);
	__HAL_LINKDMA(&ComPort, hdmarx, hDMA_RX);

	//Enable USART interrupts
	HAL_NVIC_SetPriority(USART_IRQN, 3, 0);
	HAL_NVIC_EnableIRQ(USART_IRQN);
	__HAL_UART_ENABLE_IT(&ComPort, UART_IT_CM);

	//Starts port reading
	HAL_UART_Receive_DMA(&ComPort, RxBuffer, RXBUFSIZE);
}


//Transmits data from Tx circular buffer either in one part if data is linear or in two parts if data is broken by circular buffers end.
static void TransmitTxBuffer(void){

	HAL_NVIC_DisableIRQ(DMA_TX_IRQN);											//DMA interrupt must be disabled - if transfer is complete immediately (one byte only, DMA transmit function invokes interrupt before tail position is updated)
	if( TxHead != TxTail ){														//If there is data to transfer
		if( TxHead > TxTail ){													//If all data in circular buffer is linear
			if( HAL_UART_Transmit_DMA(&ComPort, TxBuffer + TxTail , TxHead - TxTail) == HAL_OK ){
				TxTail = TxHead;												//Transmit all data and move tail to head position
			}
		}
		else{																	//If data is separated by circular buffer end
			if( HAL_UART_Transmit_DMA(&ComPort, TxBuffer + TxTail , TXBUFSIZE - TxTail) == HAL_OK ){
				TxTail = 0;														//Transmit first end of buffer data part and then linear part from beginning of circular buffer
			}
		}
	}
	HAL_NVIC_EnableIRQ(DMA_TX_IRQN);
}


//Sends data via communication peripheral
void com_SendData(uint8_t *data, uint16_t sizeB){
	uint32_t i;

	for( i=0; i<sizeB; i++ ){
		TxBuffer[ TxHead++ ] = data[i];
		TxHead &= TXBUFHEADMASK;
	}

	TransmitTxBuffer();
}


//////////////////////////////////////////////////////////////////////
////////// RTOS TASK TO HANDLE RX/TX DATA /////////////////////////
//////////////////////////////////////////////////////////////////////

static uint32_t PrevBtn12Status = 0;
static float FilJoyRoll = 0, FilJoyPitch = 0, FilJoyYaw = 0, FilJoyTrust = 0;


void com_HandleReceivedData(void){
	uint8_t tmp;
	uint32_t i = 0;

	while( ( ( tmp = RxBuffer[ (RxTail++) & RXBUFTAILMASK ] ) != ENDOFCMDCHAR ) ){
		if( i < ( SINGLECMDBUFSIZE - 2 ) ){
			Cmd[i++] = tmp;								//Copy byte by byte to command buffer until end of command byte is found or command buffer is full
		}
	}
	Cmd[i++] = ENDOFCMDCHAR;
	Cmd[i] = 0;


#if	BTCOMDEBUG
	static uint32_t Toggler = 1;
	//Toggle engines
	if( Cmd[0] == 'e' ){
		if ( Toggler++ & 0x1 ){
			eng_Enable();
			st_SetDesiredThrust(0.1);
			st_InitRegulators(1);
		}
		else{
			eng_Stop();
		}
	}

	#define KP_INC	0.00001
	#define KI_INC	0.0000001
	#define KD_INC	0.001
	else if( Cmd[0] == 'p' ){		//Increment P
		st_IncrementKp(KP_INC);
	}
	else if( Cmd[0] == 'l' ){	//Decrement P
		st_IncrementKp(-KP_INC);
	}

	else if( Cmd[0] == 'd' ){		//Increment D
		st_IncrementKd(KD_INC);
	}
	else if( Cmd[0] == 'x' ){	//Decrement D
		st_IncrementKd(-KD_INC);
	}

	else if( Cmd[0] == 'i' ){		//Increment I
		st_IncrementKi(KI_INC);
	}
	else if( Cmd[0] == 'j' ){	//Decrement I
		st_IncrementKi(-KI_INC);
	}
	else if( Cmd[0] == 'r' ){	//Reset
		NVIC_SystemReset();
	}
	else if( Cmd[0] == 's' ){	//Save
		set_Save();
	}
	else if( Cmd[0] == 'a' ){
		st_AdjustPitch(-5);
	}
	else if( Cmd[0] == 'y' ){
		st_AdjustPitch(5);
	}
	else if( Cmd[0] == ',' ){
		st_AdjustRoll(5);
	}
	else if( Cmd[0] == '.' ){
		st_AdjustRoll(-5);
	}
	else if( Cmd[0] == '-' ){
		st_ResetRollPitchCalibration();
	}
	else if( Cmd[0] == '§' ){
		st_CalibratePitchRoll();
	}
	return;
#endif


	//Unfortunately XBEE API mode doesnt send terminal character which can be used as match character for interrupting when whole packet is received
	//XBEE API uses starting character '~'.
	//In this implementation this starting character is used as match character. Therefore firmware waits for second packet and evaluates first... and so on
	if( gp_ParseXBEEAPI( Cmd, &GamepadStatus ) == gp_ErrTd_OK ){					//Try to decode gamepad status

		if( GamepadStatus.Btn12 ){													//Check if btn12 of gamepad is pressed and if not stop of engines

			if( GamepadStatus.Btn12 != PrevBtn12Status ){							//If Engines are just started we want to reset regulators (especially to get rid of integral windup) and enable engines
				st_ResetRegulators();
				eng_EnableAll();
			}


			#define JOYALPHA	0.5
			#define THRUSTALPHA	0.5
			FilJoyRoll = util_ExpMovAvg(JOYALPHA, FilJoyRoll, GamepadStatus.RightJoyX);
			FilJoyPitch = util_ExpMovAvg(JOYALPHA, FilJoyPitch, GamepadStatus.RightJoyY);
			FilJoyYaw = util_ExpMovAvg(JOYALPHA, FilJoyYaw, GamepadStatus.LeftJoyX);
			FilJoyTrust = util_ExpMovAvg(THRUSTALPHA, FilJoyTrust, GamepadStatus.LeftJoyY);

			st_SetDesiredThrust( FilJoyTrust + st_GetMinThrust());
			st_SetDesiredPitch( FilJoyPitch * -30 );
			st_SetDesiredRoll( FilJoyRoll * 30 );
			st_SetDesiredYaw( FilJoyYaw * 30 );
		}

		else if( GamepadStatus.Square ){									//when we press square this means we are tuning PID controllers

			if( BtnDebounce > HAL_GetTick() ) return;						//BtnDebounce is higher when button cooldown(debounce) is running
			BtnDebounce = HAL_GetTick() + 200;

			#define GAMEPAD_TUNE_TRESHOLD	0.75
			#define KP_INC	0.0001
			#define KI_INC	0.00001
			#define KD_INC	0.00001
			#define MINTHRUST_INC	0.01

			//Pitch and roll PID tunning
			if( GamepadStatus.RightJoyY > GAMEPAD_TUNE_TRESHOLD ){			//Increment P
				st_IncrementKp(KP_INC);
			}
			else if( GamepadStatus.RightJoyY < -GAMEPAD_TUNE_TRESHOLD ){	//Decrement P
				st_IncrementKp(-KP_INC);
			}

			if( GamepadStatus.RightJoyX > GAMEPAD_TUNE_TRESHOLD ){			//Increment D
				st_IncrementKd(KD_INC);
			}
			else if( GamepadStatus.RightJoyX < -GAMEPAD_TUNE_TRESHOLD ){	//Decrement D
				st_IncrementKd(-KD_INC);
			}

			if( GamepadStatus.LeftJoyX > GAMEPAD_TUNE_TRESHOLD ){			//Increment I
				st_IncrementKi(KI_INC);
			}
			else if( GamepadStatus.LeftJoyX < -GAMEPAD_TUNE_TRESHOLD ){		//Decrement I
				st_IncrementKi(-KI_INC);
			}

			//Yaw PID tunning
			if( GamepadStatus.LeftJoyY > GAMEPAD_TUNE_TRESHOLD ){			//Increment P
				st_IncrementYawKp(KP_INC);
			}
			else if( GamepadStatus.LeftJoyY < -GAMEPAD_TUNE_TRESHOLD ){		//Decrement P
				st_IncrementYawKp(-KP_INC);
			}
		}
		else if( GamepadStatus.Circle ){

			if( BtnDebounce > HAL_GetTick() ){								//BtnDebounce is higher when button cooldown(debounce) is running
					 return;
			}

			BtnDebounce = HAL_GetTick() + 200;

			if( GamepadStatus.LeftJoyY > GAMEPAD_TUNE_TRESHOLD ){			//Increment minimal engines thrust
				st_IncrementMinThrust(MINTHRUST_INC);
			}
			else if( GamepadStatus.LeftJoyY < -GAMEPAD_TUNE_TRESHOLD ){		//Decrement minimal engines thrust
				st_IncrementMinThrust(-MINTHRUST_INC);
			}
/*
			if( GamepadStatus.RightJoyX > GAMEPAD_TUNE_TRESHOLD ){
				st_AdjustRoll(-1);
			}
			else if( GamepadStatus.RightJoyX < -GAMEPAD_TUNE_TRESHOLD ){
				st_AdjustRoll(1);
			}

			if( GamepadStatus.RightJoyY > GAMEPAD_TUNE_TRESHOLD ){
				st_AdjustPitch(-1);
			}
			else if( GamepadStatus.RightJoyY < -GAMEPAD_TUNE_TRESHOLD ){
				st_AdjustPitch(1);
			}

			if( GamepadStatus.LeftJoyX > GAMEPAD_TUNE_TRESHOLD ){
				st_ResetRollPitchCalibration();
			}
			else if( GamepadStatus.LeftJoyX < -GAMEPAD_TUNE_TRESHOLD ){
				st_CalibratePitchRoll();
			}

			else{
				st_NullYaw();
				st_SetDesiredYaw(0);
			}
*/
		}
		else if( GamepadStatus.Triangle ){

			if( BtnDebounce > HAL_GetTick() ){								//BtnDebounce is higher when button cooldown(debounce) is running
				return;
			}

			if( GamepadStatus.LeftJoyX > GAMEPAD_TUNE_TRESHOLD ){
				NVIC_SystemReset();
			}
			else if( GamepadStatus.LeftJoyX < -GAMEPAD_TUNE_TRESHOLD ){
				set_Save();
				NVIC_SystemReset();
			}
		}
		else{															//If Btn12 is not pressed stop engines and decrement axes values
			eng_StopAll();
			FilJoyRoll = util_ExpMovAvg(JOYALPHA, FilJoyRoll, 0);
			FilJoyPitch = util_ExpMovAvg(JOYALPHA, FilJoyPitch, 0);
			FilJoyYaw = util_ExpMovAvg(JOYALPHA, FilJoyYaw, 0);
			FilJoyTrust = util_ExpMovAvg(THRUSTALPHA, FilJoyTrust, 0);
		}
		PrevBtn12Status = GamepadStatus.Btn12;
		bl_SetTimeout(&RxTimeout, RX_TIMEOUT_MS);							//Restart timeout counter
	}
	else{
		eng_StopAll();															//If received packet could not be parsed also stop engines
	}
}


#define SIG_DATARECEIVED	0x01
void com_ModuleTask(void const *argument){

	for(;;){
		osSignalWait(SIG_DATARECEIVED, osWaitForever);						//Wait until end of command char arrives
		com_HandleReceivedData();											//Handle command
		RxCmdCnt--;															//Decrement counter of received commands in buffer
		if( RxCmdCnt <= 0 ){
			osSignalClear(com_ModuleTaskId, SIG_DATARECEIVED);				//Clear task flag if there are no more commands in buffer
		}
		else{
			osSignalSet(com_ModuleTaskId, SIG_DATARECEIVED);				//Set signal to receiving task
		}
	}
}


//////////////////////////////////////////////////////////////////////
////////// INTERRUPT HANDLERS AND CALLBACKS //////////////////////////
//////////////////////////////////////////////////////////////////////

void USART_IRQH(void){
	if( __HAL_USART_GET_IT(&ComPort, UART_IT_CM ) ){						//If character match interrupt is pending
		__HAL_UART_CLEAR_IT(&ComPort, UART_CLEAR_CMF);						//Clear interrupt pending bit

		RxCmdCnt++;															//Increment counter of received commands in buffer
		osSignalSet(com_ModuleTaskId, SIG_DATARECEIVED);					//Set signal to receiving task
	}
}


void DMA_TX_IRQH(void){
	HAL_DMA_IRQHandler(ComPort.hdmatx);										//Generic HAL DMA interrupt handler
}


//Tx transfer complete callback, that transmits all data from TxBuffer in linear blocks via dma
//TODO FIX PROBLEM WITH UART CALLBACKS COMING FROM MULTIPLE UART INSTANCES AND MULTIPLE DEFINED ERROR!!
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if( huart == &ComPort ){
		TransmitTxBuffer();
	}
}
