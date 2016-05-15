/*
*Filename: batmon.c
*Author: Stanislav Subrt
*Year: 2014
*/

#include "batmon.h"
#include "main.h"

#define BAT_ADC_RESOLUTION				0xFFF+1
#define BAT_ADC_INSTANCE				ADC1
#define BAT_ADC_ENABLEPERIPHCLK			__ADC1_CLK_ENABLE

#define BAT_VOL_ADC_CHANNEL				ADC_CHANNEL_1
#define BAT_VOL_PORTINSTANCE			GPIOA
#define BAT_VOL_PORT_ENABLEPERIPHCLK	__GPIOA_CLK_ENABLE
#define BAT_VOL_PIN						GPIO_PIN_0

#define BAT_CUR_ADC_CHANNEL				ADC_CHANNEL_2
#define BAT_CUR_PORTINSTANCE			GPIOA
#define BAT_CUR_PORT_ENABLEPERIPHCLK	__GPIOA_CLK_ENABLE
#define BAT_CUR_PIN						GPIO_PIN_1


static ADC_HandleTypeDef Converter;
static ADC_ChannelConfTypeDef ChannelConfig;

//Initializes battery monitoring module
void bat_Init(void){
	GPIO_InitTypeDef GPIO_InitStruct;

	//Enable peripheral clocks
	BAT_ADC_ENABLEPERIPHCLK();
	BAT_VOL_PORT_ENABLEPERIPHCLK();
	BAT_CUR_PORT_ENABLEPERIPHCLK();

	//Initialize GPIO pin for analog mode
	GPIO_InitStruct.Pin = BAT_VOL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BAT_VOL_PORTINSTANCE, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BAT_CUR_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BAT_CUR_PORTINSTANCE, &GPIO_InitStruct);

	Converter.Instance = BAT_ADC_INSTANCE;
	Converter.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
	Converter.Init.Resolution            = ADC_RESOLUTION12b;
	Converter.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	Converter.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	Converter.Init.EOCSelection          = EOC_SINGLE_CONV;
	Converter.Init.LowPowerAutoWait      = DISABLE;
	Converter.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
	Converter.Init.NbrOfConversion       = 1;                             /* Parameter discarded because sequencer is disabled */
	Converter.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
	Converter.Init.NbrOfDiscConversion   = 1;                             /* Parameter discarded because sequencer is disabled */
	Converter.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
	Converter.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	Converter.Init.DMAContinuousRequests = ENABLE;
	Converter.Init.Overrun               = OVR_DATA_OVERWRITTEN;
	HAL_ADC_Init(&Converter);

	HAL_ADCEx_Calibration_Start(&Converter, ADC_SINGLE_ENDED);
}


//Returns battery voltage in volts
#define BAT_INPUTDIVIDERRATIO	( (float)10 / (float)110 )				//Board resistor values
float bat_GetBatteryVoltage(void){
	unsigned short Dat;
	float Voltage;

	//Configure ADC channel
	ChannelConfig.Channel      = BAT_VOL_ADC_CHANNEL;
	ChannelConfig.Rank         = ADC_REGULAR_RANK_1;
	ChannelConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
	ChannelConfig.SingleDiff   = ADC_SINGLE_ENDED;
	ChannelConfig.OffsetNumber = ADC_OFFSET_NONE;
	ChannelConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&Converter, &ChannelConfig);

	HAL_ADC_Start(&Converter);
	HAL_ADC_PollForConversion(&Converter, 10);
	Dat = HAL_ADC_GetValue(&Converter);								//Get converted data
	Voltage = 0.001 * ( (float)Dat * (float)MAIN_BRD_VREF_MV * ( 1 / BAT_INPUTDIVIDERRATIO ) / (float)BAT_ADC_RESOLUTION );

	return Voltage;
}


#define BAT_CUR_RSENS_MILIOHM	5
#define BAT_CUR_OPAMPGAIN	20.0
//Returns battery current in amps
float bat_GetBatteryCurrent(void){;
	float Dat, Current, Voltage_mV;

	//Configure ADC channel
	ChannelConfig.Channel      = BAT_CUR_ADC_CHANNEL;
	ChannelConfig.Rank         = ADC_REGULAR_RANK_1;
	ChannelConfig.SamplingTime = ADC_SAMPLETIME_61CYCLES_5;
	ChannelConfig.SingleDiff   = ADC_SINGLE_ENDED;
	ChannelConfig.OffsetNumber = ADC_OFFSET_NONE;
	ChannelConfig.Offset = 0;
	HAL_ADC_ConfigChannel(&Converter, &ChannelConfig);

	HAL_ADC_Start(&Converter);
	HAL_ADC_PollForConversion(&Converter, 10);
	Dat = HAL_ADC_GetValue(&Converter);								//Get converted data
	Voltage_mV = ( ( (float)Dat * (float)MAIN_BRD_VREF_MV ) / (float)BAT_ADC_RESOLUTION ) ;
	Current =  Voltage_mV / ( BAT_CUR_RSENS_MILIOHM * (float)BAT_CUR_OPAMPGAIN );

	return Current;
}


//Returns remaining capacity of battery based of simplification that battery is full at 12.6V and empty at 9V and voltage vs capacity is linear function.
float bat_GetRemainingCapacity(void){
//	float CapacityRemaining;
//
//	CapacityRemaining = ( 27.778 * bat_GetBatteryVoltage() - 250 );		//Linear aproximation of discharging curve - Tracer 12V 8Ah battery is full at 12.6V and empty at 9V(9V is for calculatin, 8.4V is cut off)
//	if( CapacityRemaining < 0 ) CapacityRemaining = 0;
//	else if ( CapacityRemaining > 100 ) CapacityRemaining = 100;
//	return CapacityRemaining;
	return 0;
}

