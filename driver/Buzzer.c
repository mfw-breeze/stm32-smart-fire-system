#include "stm32f10x.h"
#include "buzzer.h"

#define BUZZER_PORT   GPIOB
#define BUZZER_PIN    GPIO_Pin_0

static uint8_t buzzMode = BUZZ_OFF;

void Buzzer_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin   = BUZZER_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BUZZER_PORT, &GPIO_InitStructure);
	Buzzer_Off();
}

/* 低电平触发 */
void Buzzer_On(void)  { GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN); }
void Buzzer_Off(void) { GPIO_SetBits  (BUZZER_PORT, BUZZER_PIN); }

void Buzzer_SetMode(uint8_t mode) { buzzMode = mode; }

void Buzzer_Task(void)
{
	static uint8_t cnt = 0;
	cnt++;

	switch (buzzMode)
	{
	case BUZZ_SLOW:                    /* 5 × 200ms = 1 秒响一次 */
		if ((cnt % 5) == 0) Buzzer_On(); else Buzzer_Off();
		break;

	case BUZZ_FAST:                    /* 2 × 200ms = 2.5Hz */
		if ((cnt % 2) == 0) Buzzer_On(); else Buzzer_Off();
		break;

	default:
		Buzzer_Off();
		cnt = 0;
		break;
	}
}
