/*
 * alarm 状态机的 PC 端单元测试（不需要开发板）
 *
 * 编译：make test        （或 gcc -std=c99 -Ii../app test_alarm.c ../app/alarm.c -o test_alarm）
 * 期望：全部 PASS，退出码 0
 *
 * 所有期望值都是先用等效模型跑过一遍得到的，不是"大概应该这样"。
 */

#include <stdio.h>
#include "alarm.h"

static int g_pass = 0;
static int g_fail = 0;

static void check_eq(const char *what, long expect, long actual)
{
    if (expect == actual)
    {
        g_pass++;
    }
    else
    {
        g_fail++;
        printf("  FAIL: %s  |  期望 %ld, 实际 %ld\n", what, expect, actual);
    }
}

/* 场景用到的常量 */
#define SMOKE_CLEAN   1262   /* 洁净空气本底（实机标定值） */
#define SMOKE_HEAVY   2500   /* 明显高于触发线的烟雾 */

/* ---------- S1 静置：不报警，基线不漂 ---------- */
static void case_quiet(void)
{
    int i;
    printf("[S1] 洁净空气静置\n");

    alarm_init();
    alarm_feed_temp(25);
    check_eq("首帧只建立基线", ALARM_EVT_NONE, alarm_update(SMOKE_CLEAN));
    for (i = 0; i < 4; i++)
    {
        check_eq("静置无事件", ALARM_EVT_NONE, alarm_update(SMOKE_CLEAN));
    }
    check_eq("保持正常态", ALARM_STATE_NORMAL, alarm_state());
    check_eq("基线稳定在 1262", 1262, alarm_baseline());
}

/* ---------- S2 预警：连续 8 次高值才触发 ---------- */
static void case_warning(void)
{
    int i;
    printf("[S2] 烟雾持续偏高 → 预警\n");

    alarm_init();
    alarm_feed_temp(25);
    alarm_update(SMOKE_CLEAN);                 /* 建立基线 */

    for (i = 0; i < 7; i++)
    {
        check_eq("第 1~7 次高值不触发", ALARM_EVT_NONE, alarm_update(SMOKE_HEAVY));
    }
    check_eq("第 8 次高值触发预警", ALARM_EVT_WARN, alarm_update(SMOKE_HEAVY));
    check_eq("进入预警态", ALARM_STATE_WARNING, alarm_state());
    check_eq("基线被跟踪到 1405", 1405, alarm_baseline());
}

/* ---------- S3 单点毛刺：不误报 ---------- */
static void case_spike(void)
{
    static const uint16_t seq[] = {
        SMOKE_CLEAN, SMOKE_HEAVY, SMOKE_CLEAN, SMOKE_CLEAN, SMOKE_HEAVY,
        SMOKE_CLEAN, SMOKE_CLEAN, SMOKE_CLEAN, SMOKE_HEAVY, SMOKE_CLEAN
    };
    unsigned i;
    printf("[S3] 断续毛刺不应误报\n");

    alarm_init();
    alarm_feed_temp(25);
    for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i++)
    {
        check_eq("毛刺序列无事件", ALARM_EVT_NONE, alarm_update(seq[i]));
    }
    check_eq("始终正常态", ALARM_STATE_NORMAL, alarm_state());
}

/* ---------- S4 只有烟雾、温度正常 → 停在预警，不喷水 ---------- */
static void case_no_fire_without_temp(void)
{
    int i;
    printf("[S4] 温度正常时不升级火警\n");

    alarm_init();
    alarm_feed_temp(25);
    alarm_update(SMOKE_CLEAN);
    for (i = 0; i < 8; i++)  alarm_update(SMOKE_HEAVY);   /* 进入预警 */
    for (i = 0; i < 12; i++)
    {
        check_eq("无温度异常则无升级事件", ALARM_EVT_NONE, alarm_update(SMOKE_HEAVY));
    }
    check_eq("停在预警态", ALARM_STATE_WARNING, alarm_state());
}

/* ---------- S5 升温 → 升级火警 ---------- */
static void case_fire(void)
{
    int i;
    printf("[S5] 烟雾高 + 温度异常 → 火警\n");

    alarm_init();
    alarm_feed_temp(25);
    alarm_update(SMOKE_CLEAN);
    for (i = 0; i < 8; i++)  alarm_update(SMOKE_HEAVY);   /* 进入预警 */

    alarm_feed_temp(50);                                   /* 温度异常 */
    for (i = 0; i < 4; i++)
    {
        check_eq("升级条件的前 4 次不切换", ALARM_EVT_NONE, alarm_update(SMOKE_HEAVY));
    }
    check_eq("第 5 次触发火警", ALARM_EVT_FIRE, alarm_update(SMOKE_HEAVY));
    check_eq("进入火警态", ALARM_STATE_FIRE, alarm_state());
}

/* ---------- S6 火警降级：温度恢复连续 10 次 ---------- */
static void case_fire_downgrade(void)
{
    int i;
    printf("[S6] 温度恢复 → 火警降级为预警\n");

    alarm_init();
    alarm_feed_temp(25);
    alarm_update(SMOKE_CLEAN);
    for (i = 0; i < 8; i++)  alarm_update(SMOKE_HEAVY);
    alarm_feed_temp(50);
    for (i = 0; i < 5; i++)  alarm_update(SMOKE_HEAVY);    /* 进入火警 */

    alarm_feed_temp(25);                                   /* 温度恢复 */
    for (i = 0; i < 9; i++)
    {
        check_eq("降级条件的前 9 次不切换", ALARM_EVT_NONE, alarm_update(SMOKE_HEAVY));
    }
    check_eq("第 10 次降级", ALARM_EVT_DOWN, alarm_update(SMOKE_HEAVY));
    check_eq("回到预警态", ALARM_STATE_WARNING, alarm_state());
}

/* ---------- S7 预警解除：烟雾回落到解除线以下 ---------- */
static void case_clear(void)
{
    int i;
    printf("[S7] 烟雾回落 → 预警解除\n");

    alarm_init();
    alarm_feed_temp(25);
    alarm_update(SMOKE_CLEAN);
    for (i = 0; i < 8; i++)  alarm_update(SMOKE_HEAVY);    /* 进入预警 */

    for (i = 0; i < 7; i++)
    {
        check_eq("解除条件的前 7 次不切换", ALARM_EVT_NONE, alarm_update(SMOKE_CLEAN));
    }
    check_eq("第 8 次解除预警", ALARM_EVT_CLEAR, alarm_update(SMOKE_CLEAN));
    check_eq("回到正常态", ALARM_STATE_NORMAL, alarm_state());
}

/* ---------- S8 温度判据：绝对超温 / 温升过快 ---------- */
static void case_temp_judge(void)
{
    printf("[S8] 温度判据\n");

    alarm_init();
    alarm_feed_temp(25);
    check_eq("25°C 正常", 0, alarm_temp_bad());

    alarm_feed_temp(25);
    alarm_feed_temp(26);
    alarm_feed_temp(27);
    alarm_feed_temp(28);          /* 窗口内 25 → 28，升 3°C */
    check_eq("10s 内升温 3°C 判异常", 1, alarm_temp_bad());

    alarm_init();
    alarm_feed_temp(45);
    check_eq("≥45°C 判异常", 1, alarm_temp_bad());

    alarm_feed_temp(20);
    check_eq("温度正常后标志清除", 0, alarm_temp_bad());
}

int main(void)
{
    printf("=== alarm 状态机单元测试 ===\n\n");

    case_quiet();
    case_warning();
    case_spike();
    case_no_fire_without_temp();
    case_fire();
    case_fire_downgrade();
    case_clear();
    case_temp_judge();

    printf("\n--------------------------------\n");
    printf("通过 %d 项，失败 %d 项\n", g_pass, g_fail);
    printf("%s\n", (g_fail == 0) ? "OK" : "FAILED");
    return (g_fail == 0) ? 0 : 1;
}
