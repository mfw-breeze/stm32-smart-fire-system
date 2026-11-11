#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include <stdio.h>


#define DHT11_PORT	GPIOB
#define DHT11_PIN	GPIO_Pin_15



//1MHz的自由运行定时器
void DHT11_TimerInit(void)
{
	RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2,ENABLE );
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;					 //自由运行，65.5ms才绕回一次
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;				 //72MHz/72=1MHz,1个计数=1us
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);
	
	TIM_Cmd (TIM2,ENABLE );
}



void DHT11_Out(void)		//推挽输出
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DHT11_PORT,&GPIO_InitStructure);

}
void DHT11_In(void)		//上拉输入
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DHT11_PORT,&GPIO_InitStructure);

}

void DHT11_Init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOB,ENABLE);
	DHT11_In();


}

static void DHT11_Start(void)
{
	DHT11_Out();
	GPIO_ResetBits (DHT11_PORT,DHT11_PIN); //拉低
	Delay_ms (20);     					   //至少18ms
	GPIO_SetBits (DHT11_PORT,DHT11_PIN);	//释放为高
	Delay_us (30);     					   //20~40us
	DHT11_In();								//切输入，等应答

}

static uint8_t DHT11_WaitACK(void)
{
	uint32_t retry = 0;
	while(GPIO_ReadInputDataBit(DHT11_PORT ,DHT11_PIN ) == SET)
		if(++retry > 8000) return 1;
	retry = 0;
	while(GPIO_ReadInputDataBit(DHT11_PORT ,DHT11_PIN ) == RESET)
		if(++retry > 8000) return 1;
	/* 等应答的 80us 高电平结束，让线回到"下一个位的低电平" */
	retry = 0;
	while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == SET)
		if(++retry > 8000) return 1;

	
	return 0;
	

}

uint8_t DHT11_ReadByte(void)
{
	uint8_t i,Data = 0;
	uint32_t retry;
	for(i = 0; i < 8; i++)
	{
		retry = 0;
		while(GPIO_ReadInputDataBit(DHT11_PORT ,DHT11_PIN ) == RESET); //等低电平结束
			if(++retry > 8000) return 0;  //超时返回0
		Delay_us(40); 		//40us后采样
	
		if(GPIO_ReadInputDataBit(DHT11_PORT ,DHT11_PIN ) == SET)
			Data |= (0x80 >> i); //把数据填进去
		retry = 0;
		while(GPIO_ReadInputDataBit(DHT11_PORT ,DHT11_PIN ) == SET); //等高电平结束
			if(++retry  > 8000) return 0;
	
	}
	
	return Data;

}

uint8_t DHT11_ReadData(uint8_t *humi_int,uint8_t *temp_int)
{
	uint8_t i,Buff[5]={0};
	DHT11_Start();		//主机启动
	if (DHT11_WaitACK() != 0) return 1;   // 没有应答就直接返回错误
	/* 3. 读 5 字节：湿度整/小 + 温度整/小 + 校验 */
    for (i = 0; i < 5; i++)
        Buff[i] = DHT11_ReadByte();

    /* 4. 校验：前 4 字节之和 == 校验字节 */
    if ((Buff[0] + Buff[1] + Buff[2] + Buff[3]) != Buff[4]) return 2;   // 校验失败

    *humi_int = Buff[0];    // 湿度整数
    *temp_int = Buff[2];    // 温度整数
    return 0;

}



