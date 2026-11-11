#include "stm32f10x.h"                  // Device header
#include "MyADC.h"




void MQ2_Init(void)
{
	MyADC_Init();


}
/* 原始 ADC 值（8 次平均，0~4095） */
uint16_t MQ2_GetRaw(void)
{
	return MyADC_GetAverage(8);
}

/* 换算成 mV（Vref = 3.3V） */
uint16_t MQ2_GetMillivolt(void)
{
	return (uint16_t)((uint32_t)MQ2_GetRaw() * 3300 / 4095);
}


