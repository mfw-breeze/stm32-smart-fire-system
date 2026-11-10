#include "alarm.h"
#include "alarm_config.h"

/*
 * 三态告警状态机
 *
 *   NORMAL ──(烟雾连续 8 次 > 基线+margin)──> WARNING
 *   WARNING ─(烟雾连续 8 次 < 基线+release)─> NORMAL
 *   WARNING ─(烟雾仍高 && 温度异常，连续 5 次)> FIRE
 *   FIRE ────(温度连续 10 次正常)────────────> WARNING
 *
 * 说明：
 *   - 基线只在 NORMAL 态跟踪，进入 WARNING 后冻结，
 *     这样告警期间阈值不会跟着烟雾一起抬高（否则会自己把自己"劝退"）
 *   - WARNING 是"确认期"：光有烟不喷水，烟 + 温升同时成立才升级到 FIRE
 *   - 所有计数都是"连续 N 次"，任何一次不满足就清零，用于抑制毛刺
 */

typedef struct {
    alarm_state_t state;
    uint16_t      baseline;
    uint8_t       initialized;

    uint8_t over_cnt;    /* NORMAL：连续超过触发线的次数 */
    uint8_t under_cnt;   /* WARNING：连续回落到解除线以下的次数 */
    uint8_t fire_cnt;    /* WARNING：连续满足升级条件的次数 */
    uint8_t cool_cnt;    /* FIRE：连续温度正常的次数 */

    uint8_t temp_hist[ALARM_TEMP_HIST];
    uint8_t temp_idx;
    uint8_t temp_count;
    uint8_t temp_bad;
} alarm_ctx_t;

static alarm_ctx_t ctx;

void alarm_init(void)
{
    uint8_t i;

    ctx.state       = ALARM_STATE_NORMAL;
    ctx.baseline    = 0;
    ctx.initialized = 0;

    ctx.over_cnt  = 0;
    ctx.under_cnt = 0;
    ctx.fire_cnt  = 0;
    ctx.cool_cnt  = 0;

    ctx.temp_idx   = 0;
    ctx.temp_count = 0;
    ctx.temp_bad   = 0;
    for (i = 0; i < ALARM_TEMP_HIST; i++)
    {
        ctx.temp_hist[i] = 0;
    }
}

/* 每读到一次 DHT11 温度调用：更新窗口内最小值和 temp_bad 标志 */
void alarm_feed_temp(uint8_t temp_c)
{
    uint8_t i;
    uint8_t min_temp;

    ctx.temp_hist[ctx.temp_idx] = temp_c;
    ctx.temp_idx = (uint8_t)((ctx.temp_idx + 1) % ALARM_TEMP_HIST);
    if (ctx.temp_count < ALARM_TEMP_HIST)
    {
        ctx.temp_count++;
    }

    /* 10 秒窗口内的最低温度 */
    min_temp = temp_c;
    for (i = 0; i < ctx.temp_count; i++)
    {
        if (ctx.temp_hist[i] < min_temp)
        {
            min_temp = ctx.temp_hist[i];
        }
    }

    /* 绝对超温 或 升温过快 → 温度异常 */
    if ((temp_c >= ALARM_TEMP_ABS) ||
        ((uint8_t)(temp_c - min_temp) >= ALARM_TEMP_RISE))
    {
        ctx.temp_bad = 1;
    }
    else
    {
        ctx.temp_bad = 0;
    }
}

alarm_event_t alarm_update(uint16_t smoke_raw)
{
    uint32_t margin;
    uint32_t release;

    /* 第一帧只用来建立基线，不做判断 */
    if (!ctx.initialized)
    {
        ctx.baseline    = smoke_raw;
        ctx.initialized = 1;
        return ALARM_EVT_NONE;
    }

    /* 只在正常态跟踪基线；告警期间冻结，避免阈值被烟雾带着走 */
    if (ctx.state == ALARM_STATE_NORMAL)
    {
        ctx.baseline = (uint16_t)(((uint32_t)ctx.baseline * (ALARM_BASE_WEIGHT - 1) + smoke_raw)
                                  / ALARM_BASE_WEIGHT);
    }

    /* 触发线：基线 + 相对量，但不低于绝对下限 */
    margin = (uint32_t)ctx.baseline * ALARM_RISE_PCT / 100;
    if (margin < ALARM_MIN_MARGIN)
    {
        margin = ALARM_MIN_MARGIN;
    }

    /* 解除线：比触发线低，形成迟滞 */
    release = (uint32_t)ctx.baseline * ALARM_FALL_PCT / 100;
    if (release < (ALARM_MIN_MARGIN / 2))
    {
        release = ALARM_MIN_MARGIN / 2;
    }

    switch (ctx.state)
    {
    case ALARM_STATE_NORMAL:
        if (smoke_raw > (uint32_t)ctx.baseline + margin)
        {
            ctx.over_cnt++;
            if (ctx.over_cnt >= ALARM_CONFIRM_CNT)
            {
                ctx.over_cnt = 0;
                ctx.fire_cnt = 0;
                ctx.state    = ALARM_STATE_WARNING;
                return ALARM_EVT_WARN;
            }
        }
        else
        {
            ctx.over_cnt = 0;
        }
        break;

    case ALARM_STATE_WARNING:
        /* 1) 先看是否已回落到解除线以下 */
        if (smoke_raw < (uint32_t)ctx.baseline + release)
        {
            ctx.under_cnt++;
            if (ctx.under_cnt >= ALARM_CONFIRM_CNT)
            {
                ctx.under_cnt = 0;
                ctx.fire_cnt  = 0;
                ctx.state     = ALARM_STATE_NORMAL;
                return ALARM_EVT_CLEAR;
            }
        }
        else
        {
            ctx.under_cnt = 0;
        }

        /* 2) 再看是否升级：烟雾仍高 且 温度异常 */
        if ((smoke_raw > (uint32_t)ctx.baseline + margin) && ctx.temp_bad)
        {
            ctx.fire_cnt++;
            if (ctx.fire_cnt >= ALARM_FIRE_CNT)
            {
                ctx.fire_cnt = 0;
                ctx.cool_cnt = 0;
                ctx.state    = ALARM_STATE_FIRE;
                return ALARM_EVT_FIRE;
            }
        }
        else
        {
            ctx.fire_cnt = 0;
        }
        break;

    case ALARM_STATE_FIRE:
        /* 温度回正常 → 降级为预警（不直接回正常，需人工确认现场） */
        if (!ctx.temp_bad)
        {
            ctx.cool_cnt++;
            if (ctx.cool_cnt >= ALARM_COOL_CNT)
            {
                ctx.cool_cnt = 0;
                ctx.under_cnt = 0;
                ctx.state     = ALARM_STATE_WARNING;
                return ALARM_EVT_DOWN;
            }
        }
        else
        {
            ctx.cool_cnt = 0;
        }
        break;

    default:
        ctx.state = ALARM_STATE_NORMAL;
        break;
    }

    return ALARM_EVT_NONE;
}

alarm_state_t alarm_state(void)
{
    return ctx.state;
}

uint16_t alarm_baseline(void)
{
    return ctx.baseline;
}

uint8_t alarm_temp_bad(void)
{
    return ctx.temp_bad;
}
