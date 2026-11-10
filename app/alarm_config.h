#ifndef ALARM_CONFIG_H
#define ALARM_CONFIG_H

/*
 * 告警判据参数（全部在实机上标定过，不是拍脑袋定的默认值）
 *
 * 时间基准：alarm_update() 每 200ms 调用一次；DHT11 每 2s 读一次。
 * 下列注释里的"秒"都是按这个基准换算的。
 */

/* ---------- 烟雾判据（MQ-2 原始 ADC 值，0~4095）---------- */
#define ALARM_RISE_PCT        32   /* 触发点 = 基线 × (1 + 32%)，基线 1262 时约 +404 */
#define ALARM_FALL_PCT        16   /* 解除点 = 基线 × (1 + 16%)，形成迟滞区间，防止阈值抖动 */
#define ALARM_MIN_MARGIN     400   /* 绝对下限：基线很低时不要过于敏感（约 0.32V） */
#define ALARM_CONFIRM_CNT      8   /* 连续 8 次 × 200ms = 1.6s 才确认，抑制单点毛刺 */
#define ALARM_BASE_WEIGHT     64   /* 基线一阶低通的时间常数，越大跟踪越慢 */

/* ---------- 温度判据 ---------- */
#define ALARM_TEMP_ABS        45   /* 绝对温度 ≥45°C 判异常 */
#define ALARM_TEMP_RISE        3   /* 窗口内升温 ≥3°C 判异常（突变比高值更危险） */
#define ALARM_TEMP_HIST        5   /* 温度窗口：5 次 × 2s = 10s */

/* ---------- 状态迁移确认次数 ---------- */
#define ALARM_FIRE_CNT         5   /* 连续 5 次（约 1s）满足升级条件 → 火警 */
#define ALARM_COOL_CNT        10   /* 连续 10 次（约 2s）温度正常 → 从火警降级 */

#endif /* ALARM_CONFIG_H */
