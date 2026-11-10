#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>

/*
 * 消防告警决策模块 —— 纯逻辑，不依赖任何 MCU 头文件。
 *
 * 设计意图：
 *   - 把"什么时候该报警/喷水"从硬件里剥出来，独立可测（见 tests/）
 *   - 只用 <stdint.h>，因此 PC 上可以直接编译运行单元测试
 *
 * 调用约定：
 *   alarm_init()        上电（MQ-2 预热完成后）调一次
 *   alarm_update(smoke) 每 200ms 调一次，传入 MQ-2 原始 ADC 值
 *   alarm_feed_temp(t)  每读到一次 DHT11 温度调一次（约 2s）
 */

typedef enum {
    ALARM_STATE_NORMAL  = 0,
    ALARM_STATE_WARNING = 1,
    ALARM_STATE_FIRE    = 2
} alarm_state_t;

typedef enum {
    ALARM_EVT_NONE  = 0,   /* 无状态变化 */
    ALARM_EVT_WARN  = 1,   /* 正常 → 预警 */
    ALARM_EVT_FIRE  = 2,   /* 预警 → 火警 */
    ALARM_EVT_DOWN  = 3,   /* 火警 → 预警（温度恢复） */
    ALARM_EVT_CLEAR = 4    /* 预警 → 正常（烟雾回落） */
} alarm_event_t;

void          alarm_init(void);
void          alarm_feed_temp(uint8_t temp_c);
alarm_event_t alarm_update(uint16_t smoke_raw);

alarm_state_t alarm_state(void);
uint16_t      alarm_baseline(void);
uint8_t       alarm_temp_bad(void);

#endif /* ALARM_H */
