#ifndef DHT11_H
#define DHT11_H
#include "stm32f10x.h"                  // Device header


void DHT11_TimerInit(void);
void DHT11_Init(void);
void DHT11_Out(void);
void DHT11_In(void);
uint8_t DHT11_ReadByte(void);
uint8_t DHT11_ReadData(uint8_t *humi_int,uint8_t *temp_int);








#endif
