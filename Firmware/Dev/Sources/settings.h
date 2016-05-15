/*
*Filename: settings.h
*Author: Stanislav Subrt
*Year: 2014
*/

#ifndef SETTINGS_H
#define SETTINGS_H


#include "main.h"


typedef enum{
	OpTd_Load,
	OpTd_Write,
}OpTd;


extern util_ErrTd set_Init(void);
extern util_ErrTd set_Load(void);
extern util_ErrTd set_Save(void);


#endif  //end of SETTINGS_H
