#ifndef IWDG_H
#define IWDG_H

#include "stm32f10x.h"

/*
 * 独立看门狗（IWDG）
 *
 * 时钟源：LSI ≈ 40 kHz（数据手册标称 30~60 kHz，随温度漂移很大，不能当精确时钟用）
 * 超时：prescaler = 64 → 625 Hz，reload = 1250 → 2.0 s（典型值）
 *       实际范围约 1.33 s（LSI 60 kHz）~ 2.67 s（LSI 30 kHz），所以只能做"兜底"，
 *       不能用来做定时。
 *
 * 使用约束：
 *   1. IWDG_Init() 一旦调用就无法关闭，只能靠复位。调试期想临时停掉，
 *      只能注释掉这一句。
 *   2. 喂狗位置必须在「主流程确实跑完一轮」的地方。
 *      不要放在中断里 —— 中断正常但主循环卡死时看门狗会失效，等于没装。
 *   3. 任何超过 2 s 的阻塞流程（例如 MQ-2 预热倒计时）内部都必须喂狗，
 *      否则会陷入"上电 → 2 s 复位 → 再上电"的复位循环。
 */

/* 复位原因位掩码，可能同时置多位（上电复位时 POR 和 PIN 通常会一起置位） */
#define IWDG_RST_POR        0x01u   /* 上电复位 */
#define IWDG_RST_PIN        0x02u   /* 外部 NRST 引脚复位 */
#define IWDG_RST_SOFTWARE   0x04u   /* 软件复位（NVIC_SystemReset） */
#define IWDG_RST_IWDG       0x08u   /* 独立看门狗复位 */
#define IWDG_RST_WWDG       0x10u   /* 窗口看门狗复位 */
#define IWDG_RST_LOWPOWER   0x20u   /* 低功耗管理复位 */

void        IWDG_Init(void);
void        IWDG_Feed(void);
uint8_t     IWDG_TakeResetCause(void);
const char *IWDG_CauseText(uint8_t cause);

#endif /* IWDG_H */
