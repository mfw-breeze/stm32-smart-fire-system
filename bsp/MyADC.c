#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void MyADC_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOA,ENABLE );
	
	/*设置ADC时钟*/
	RCC_ADCCLKConfig (RCC_PCLK2_Div6);   //选择时钟6分频，ADCCLK = 72MHz / 6 = 12MHz
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init (GPIOA,&GPIO_InitStructure);
	/*规则组*/
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,1,ADC_SampleTime_55Cycles5);
	
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init (ADC1 ,&ADC_InitStructure);
	
	ADC_Cmd (ADC1,ENABLE );
	ADC_TempSensorVrefintCmd(ENABLE);   /* ★ 使能内部参考电压 + 温度传感器 */
	Delay_ms(1);
	/*ADC校准*/
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1) == SET);
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1) == SET);

}

/* 单次转换：触发 → 等 EOC → 读 DR（读 DR 会自动清 EOC） */
uint16_t MyADC_GetValue(void)
{
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	return ADC_GetConversionValue(ADC1);
}

/* 多次平均，压噪声 */
uint16_t MyADC_GetAverage(uint8_t Times)
{
	uint32_t Sum = 0;
	uint8_t  i;

	if (Times == 0) Times = 1;
	for (i = 0; i < Times; i++)
	{
		Sum += MyADC_GetValue();
	}
	return (uint16_t)(Sum / Times);
}

/* 在 MyADC.c 加一个能指定通道读的函数 */
uint32_t MyADC_GetVdda_mV(void)
{
	uint16_t vref;

	/* VREFINT 要求采样时间 ≥5.1us；55.5 周期只有 4.6us，不够。
	   用最长档 239.5 周期（约 20us） */
	ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 1, ADC_SampleTime_239Cycles5);
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	vref = ADC_GetConversionValue(ADC1);

	/* ★ 关键：立刻切回通道 1，否则 MQ2 会一直读通道 17 */
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);

	if (vref == 0) return 0;
	return (uint32_t)1200 * 4095 / vref;   /* 单位 mV */
}

