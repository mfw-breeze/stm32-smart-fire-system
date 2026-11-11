#include "IWDG.h"
#include "stm32f10x_dbgmcu.h"

/*
 * 独立看门狗实现
 *
 * 为什么一开始就调 DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE)：
 *   默认情况下，Keil 里打断点让内核停机时 IWDG 照样在计数，
 *   于是"停下来看变量"这个最常用的调试动作会变成"板子每隔 2 s 复位一次"。
 *   DBGMCU 的 IWDG_STOP 位能让内核被调试器停住时冻结 IWDG 计数，
 *   这样调试和看门狗可以共存，不用反复注释 IWDG_Init()。
 *   注意：只在调试器停住内核时生效，全速运行时不冻结，保护照旧。
 */
void IWDG_Init(void)
{
    DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE);

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    IWDG_SetPrescaler(IWDG_Prescaler_64);   /* LSI 40kHz / 64 = 625 Hz */
    IWDG_SetReload(1250);                   /* 1250 / 625 = 2.0 s */

    IWDG_ReloadCounter();
    IWDG_Enable();                          /* 使能后只能靠复位关闭 */
}

void IWDG_Feed(void)
{
    IWDG_ReloadCounter();
}

/*
 * 读复位原因，读完立刻清零。
 *
 * 必须先读后清：RCC_CSR 里的复位标志一旦置位就不会自己掉，
 * 不调用 RCC_ClearFlag() 的话，下一次（哪怕是正常上电）读到的
 * 还是上一次的标志，就永远判断不出"这次到底为什么复位"。
 *
 * 所以这个函数要在 main() 的最前面调用，早于任何初始化。
 */
uint8_t IWDG_TakeResetCause(void)
{
    uint8_t cause = 0;

    if (RCC_GetFlagStatus(RCC_FLAG_PORRST)  == SET) cause |= IWDG_RST_POR;
    if (RCC_GetFlagStatus(RCC_FLAG_PINRST)  == SET) cause |= IWDG_RST_PIN;
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST)  == SET) cause |= IWDG_RST_SOFTWARE;
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) cause |= IWDG_RST_IWDG;
    if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) == SET) cause |= IWDG_RST_WWDG;
    if (RCC_GetFlagStatus(RCC_FLAG_LPWRRST) == SET) cause |= IWDG_RST_LOWPOWER;

    RCC_ClearFlag();
    return cause;
}

/*
 * 取最主要的那个原因做显示。
 * 顺序有讲究：看门狗复位时 NRST 也被拉低，PIN 标志同样是置位的，
 * 所以必须先把 IWDG / WWDG / SOFTWARE 判掉，最后才轮到 PIN / POR。
 */
const char *IWDG_CauseText(uint8_t cause)
{
    if (cause & IWDG_RST_IWDG)     return "IWDG";
    if (cause & IWDG_RST_WWDG)     return "WWDG";
    if (cause & IWDG_RST_SOFTWARE) return "SOFTWARE";
    if (cause & IWDG_RST_LOWPOWER) return "LOWPOWER";
    if (cause & IWDG_RST_PIN)      return "PIN";
    if (cause & IWDG_RST_POR)      return "POR";
    return "UNKNOWN";
}
