/*
 * 智能消防监测终端 —— 应用层主程序
 *
 * 职责边界：
 *   本文件只负责「调度 + 执行器输出 + 显示」，
 *   告警判定全部在 app/alarm.c 里，那个模块不依赖任何 MCU 头文件，
 *   所以同一份代码既能在 Keil 里跑，也能在 PC 上跑单元测试（tests/）。
 *
 * 时间基准：主循环 200ms 一轮
 *   - MQ-2 每轮采样一次
 *   - DHT11 每 10 轮读一次（2s，DHT11 不宜快读）
 *   - 串口 CSV 每 5 轮上报一行（1s）
 */

#include "stm32f10x.h"
#include <stdio.h>

#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Buzzer.h"
#include "Relay.h"
#include "DHT11.h"
#include "MQ2.h"
#include "MyADC.h"
#include "USART1.h"
#include "alarm.h"

#define SAMPLE_PERIOD_MS    200     /* 主循环周期 */
#define DHT_PERIOD_CNT       10     /* 10 × 200ms = 2s 读一次温湿度 */
#define PRINT_PERIOD_CNT      5     /* 5 × 200ms = 1s 上报一次 CSV */
#define WARMUP_SECONDS       30     /* MQ-2 预热时间 */

/* 硬件初始化：顺序与原工程一致，OLED 最先（用来显示预热倒计时） */
static void app_hardware_init(void)
{
    OLED_Init();
    LED_Init();
    USART1_Init();
    MQ2_Init();
    DHT11_Init();
    Buzzer_Init();
    Relay_Init();
}

/* MQ-2 预热：加热丝上电后 30s 内读数不可信，这段数据一律丢弃 */
static void app_warmup(void)
{
    uint8_t sec;

    OLED_ShowString(1, 1, "Warming up...");
    for (sec = 0; sec < WARMUP_SECONDS; sec++)
    {
        OLED_ShowNum(2, 1, WARMUP_SECONDS - sec, 2);
        Delay_ms(1000);
    }
    OLED_Clear();
}

/* 根据当前状态驱动执行器（幂等：每轮都写一遍，不依赖内部状态） */
static void app_actuate(alarm_state_t state, uint8_t *relay_now)
{
    uint8_t want_relay;

    want_relay = (state == ALARM_STATE_FIRE) ? 1 : 0;

    switch (state)
    {
    case ALARM_STATE_FIRE:
        LED1_ON();
        LED2_OFF();
        Buzzer_SetMode(BUZZ_FAST);
        break;

    case ALARM_STATE_WARNING:
        LED1_OFF();
        LED2_ON();
        Buzzer_SetMode(BUZZ_SLOW);
        break;

    default:
        LED1_OFF();
        LED2_ON();
        Buzzer_SetMode(BUZZ_OFF);
        break;
    }

    /* 继电器只在跳变时动作：吸合瞬间电源波动会污染 ADC，
     * 动作后丢弃两次采样再回到正常采样节奏 */
    if (want_relay != *relay_now)
    {
        *relay_now = want_relay;

        if (want_relay)
        {
            Relay_ON();
        }
        else
        {
            Relay_OFF();
        }

        Delay_ms(50);
        (void)MQ2_GetRaw();
        (void)MQ2_GetRaw();
    }
}

static void app_show(alarm_state_t state, uint16_t smoke, uint8_t temp, uint8_t humi)
{
    OLED_ShowString(1, 1, "S:");
    OLED_ShowNum(1, 3, smoke, 4);
    OLED_ShowString(1, 8, "B:");
    OLED_ShowNum(1, 10, alarm_baseline(), 4);

    OLED_ShowString(2, 1, "T:");
    OLED_ShowNum(2, 3, temp, 2);
    OLED_ShowString(2, 6, "H:");
    OLED_ShowNum(2, 8, humi, 2);

    if (state == ALARM_STATE_FIRE)
    {
        OLED_ShowString(3, 1, "FIRE   ");
    }
    else if (state == ALARM_STATE_WARNING)
    {
        OLED_ShowString(3, 1, "WARNING");
    }
    else
    {
        OLED_ShowString(3, 1, "NORMAL ");
    }
}

int main(void)
{
    uint16_t smoke   = 0;
    uint8_t  temp    = 0;
    uint8_t  humi    = 0;
    uint8_t  dht_cnt = 0;
    uint8_t  prt_cnt = 0;
    uint8_t  relay_now = 0;
    alarm_state_t state;

    app_hardware_init();

    LED1_OFF();
    LED2_ON();

    app_warmup();

    /* 预热完成后才建立基线 */
    alarm_init();

    for (;;)
    {
        /* 1. 采样 */
        smoke = MQ2_GetRaw();

        if (++dht_cnt >= DHT_PERIOD_CNT)
        {
            uint8_t h = 0;
            uint8_t t = 0;

            dht_cnt = 0;
            if (DHT11_ReadData(&h, &t) == 0)
            {
                humi = h;
                temp = t;
                alarm_feed_temp(temp);      /* 温度参与升级判定 */
            }
        }

        /* 2. 判定（纯逻辑，可在 PC 上测） */
        (void)alarm_update(smoke);
        state = alarm_state();

        /* 3. 输出 */
        app_actuate(state, &relay_now);
        Buzzer_Task();                      /* 蜂鸣器节奏，每轮调 */

        /* 4. 显示 */
        app_show(state, smoke, temp, humi);

        /* 5. CSV 上报：S,B,T,H,STATE,Vdda */
        if (++prt_cnt >= PRINT_PERIOD_CNT)
        {
            prt_cnt = 0;
            printf("%u,%u,%u,%u,%u,%lu\r\n",
                   (unsigned)smoke,
                   (unsigned)alarm_baseline(),
                   (unsigned)temp,
                   (unsigned)humi,
                   (unsigned)state,
                   MyADC_GetVdda_mV());
        }

        Delay_ms(SAMPLE_PERIOD_MS);
    }
}
