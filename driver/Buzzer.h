#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

#define BUZZ_OFF     0
#define BUZZ_SLOW    1      /* 预警：响 200ms 停 800ms */
#define BUZZ_FAST    2      /* 火警：响 200ms 停 200ms */

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_SetMode(uint8_t mode);
void Buzzer_Task(void);     /* 每 200ms 调一次 */

#endif
