#ifndef __MYADC_H
#define __MYADC_H

#include "stm32f10x.h"                  // Device header
#include <stdio.h>

void MyADC_Init(void);
uint16_t MyADC_GetValue(void);
uint16_t MyADC_GetAverage(uint8_t Times);

uint32_t MyADC_GetVdda_mV(void);




#endif
