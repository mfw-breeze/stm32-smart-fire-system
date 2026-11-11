#include "stm32f10x.h"                  // Device header
#include <stdio.h>

void USART1_Init(void)
{
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;     
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;				//TX :PA9引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;				//RX :PA10引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //硬件流控制 无
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;    //无校验位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1 ,&USART_InitStructure);
	
	USART_Cmd(USART1,ENABLE);

}

//底层，发送单字节
void USART1_SendData(uint8_t Data)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);  //等TXE 不然会丢字节
	USART_SendData(USART1,Data);

}

uint8_t USART1_ReciverData(void)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == RESET);  //等待收到数据
	return USART_ReceiveData(USART1);
}
//上层，printf重定向，内部调底层函数
int fputc(int ch,FILE* f)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
	USART_SendData(USART1,(uint8_t )ch);
	return ch;

}
