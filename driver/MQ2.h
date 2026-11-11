#ifndef __MQ2_H
#define __MQ2_H
#include "stm32f10x.h"                  // Device header

void MQ2_Init(void);
uint16_t MQ2_GetRaw(void);

uint16_t MQ2_GetMillivolt(void);




#endif
