# 开发状态与工程约定

> 这个文件是项目的「记忆体」。每次开始新会话、或隔了几天回来，先读它。
> 最后更新：2026-11-11

## 一句话

STM32F103 消防监测终端：MQ-2 烟雾 + DHT11 温湿度融合，三态告警状态机，继电器联动喷水。
仓库：`https://github.com/mfw-breeze/stm32-smart-fire-system`
本地：`C:\Users\21142\Desktop\stm32-smart-fire-system`

## 当前状态

| 项 | 状态 | 最后验证 |
|---|---|---|
| 版本 | **v0.3-dev**（IWDG 已加，其余待做） | 2026-11-11 |
| PC 单元测试 | ✅ 8 组场景 / **71 项断言全过** | MSVC 2022（`/utf-8`）实跑 |
| GitHub Actions | ✅ **passing** | 徽章 SVG 确认 |
| Keil 编译 | ✅ **0 Error / 0 Warning**，Code=**7044 B**（v0.2 时 5916 B） | UV4 命令行实编 |
| 上机实测 | ⏳ IWDG 改动未上机（复位原因上报待验证） | — |

## 代码体积（Keil 实编，ARMCC V5.06 update 6）

| 版本 | Code | 相对上一行 | 说明 |
|---|---|---|---|
| v0.2 基线 | 5916 B | — | CSV 只用 `%u` / `%lu` |
| + IWDG（`reset=%u`） | 6336 B | +420 B | iwdg.o 本身 270 B，其余是 RCC / DBGMCU 库代码 |
| + `%02X` | 6908 B | **+572 B** | 十六进制格式化 |
| + `%s`（成品） | **7044 B** | **+136 B** | 复位原因文字 |

**结论：MicroLIB 的 printf 是按格式串「选变体」的**，用了 `%X` / `%s` 就会链接更大的实现，
`printf8.o` 单独占 1154 B。当前 7044 / 65536 ≈ **10.7%**，不紧张；
日后 Flash 吃紧时，第一个该砍的就是「把复位原因打成字符串」——改回 `%u` 可省 708 B。

## 目录与职责

```
app/       main.c（调度+执行器+显示）  alarm.c/h（判定核心，禁止 include 任何 MCU 头）
           alarm_config.h（全部阈值）   stm32f10x_it.c  stm32f10x_conf.h
bsp/       Delay.c  MyADC.c  USART1.c             ← 板级：延时 / ADC / 串口
driver/    MQ2  DHT11  OLED  Relay  Buzzer  LED  Key  ← 器件驱动
vendor/    ST 标准库 + CMSIS + 启动文件            ← 不动
tests/     test_alarm.c + Makefile                 ← PC 端，不需要开发板
docs/      state-machine.md  hardware.md  calibration.md  status.md
project.uvprojx                                    ← Keil 工程
```

**核心约束：`app/alarm.c` 只依赖 `<stdint.h>`**。它是唯一同时被固件和 PC 测试使用的文件，
所以「测试通过」= 「固件判定逻辑正确」。这个约束不能破。

## 开发循环（每次加功能都走这 5 步）

1. 想清楚：这次改动要不要动 `app/alarm.c`（判定核心）？
2. 要动 → **先**去 `tests/test_alarm.c` 加场景，`make test` 跑通
3. Keil 里实现硬件侧（`driver/` `bsp/`）或改 `main.c` 调度
4. `F7` 编译 → `F8` 下载 → 上机实测
5. GitHub Desktop 提交（`feat:` / `fix:`）→ push → 确认 CI 是绿的

## 待办（Backlog）

### v0.3（下一步，按顺序）

- [x] **IWDG 独立看门狗**（2026-11-11）：`bsp/IWDG.c` 提供 `IWDG_Init` / `IWDG_Feed` /
      `IWDG_TakeResetCause` / `IWDG_CauseText`；2.0s 超时（LSI 40kHz ÷ 64，reload 1250）；
      主循环末尾喂狗，**预热倒计时内部也喂**；启动时读 `RCC_CSR` 六个复位标志后清零，
      串口打印 `reset=IWDG`，OLED 第三行同时提示；用 `DBGMCU_Config(DBGMCU_IWDG_STOP, ENABLE)`
      让调试器停住内核时冻结看门狗，避免「一打断点就复位」
- [ ] **IWDG 上机验证**：主动制造死循环，确认约 2s 复位、且重启后串口打印 `reset=IWDG`
- [ ] **阈值掉电保存**：把 `alarm_config.h` 里的阈值改成运行时参数，存到内部 Flash 最后一页，开机读回（参考 demo 30）
- [ ] **串口协议升级**：`帧头(0xAA 0x55) + 长度 + 命令 + 数据 + CRC16`，替代现在的裸 CSV；配套上位机脚本
- [ ] **传感器自检**：MQ-2 读数长期贴近 0 或满量程 → 报传感器故障（当前失效时会被误判成"环境干净"）
- [ ] **主循环周期用定时器中断驱动**：现在靠 `Delay_ms(200)`，会累积漂移

### 工程债（小，随时可做）

- [ ] `alarm.c` / `alarm.h` 在 Keil 里挂在 `driver` 组下（物理在 `app/`），挪进 `app` 组
- [ ] `tests/Makefile` 加 `-Werror`，让 CI 对警告也不放过
- [ ] README 补实物照片 / 演示 GIF
- [ ] 打 tag `v0.2.0` + Release 说明

### v0.4

- [ ] CAN / RS485 多节点组网，接入工业测控终端项目

## 约定

- **提交信息**：`feat:` `fix:` `docs:` `test:` `ci:` `chore:` + 中文简短说明
- **一个功能一个 commit**，不要攒
- **改核心必须补测试**，否则 CI 绿的是旧行为，测试就失去意义
- **`vendor/` 不动**，那是 ST 的代码
- **旧工程冻结**：`STM32project\16-1智能消防检测喷水系统` 只当备份，所有开发都在本仓库
- 驱动保留原命名（`MQ2_GetRaw`、`Relay_ON`），应用层用 snake（`alarm_update`）

## 已踩的坑（别再踩）

| # | 坑 | 现象 | 解决 |
|---|---|---|---|
| 1 | `.uvprojx` 带 UTF-8 BOM | Keil 提示 `Cannot read project file` | 存成 **无 BOM** 的 UTF-8（原文件就是无 BOM） |
| 2 | Keil 组归属与物理路径不一致 | `app/` 里的文件出现在 `driver` 组 | 只是展示问题，编译不受影响；待整理 |
| 3 | VS 的 `cl.exe` 默认按 GBK 读源码 | `error C2001: 常量中有换行符` | 编译时加 `/utf-8` |
| 4 | 直读 GitHub 不稳 | `Could not connect to github.com:443` | 重试 / 换网络；push 失败不会丢本地提交 |
| 5 | 测试数量会变 | README 曾写「40 项」，实际 71 项 | 改测试后同步 README |
| 6 | **看门狗启用后，预热期间不喂狗** | 上电后每 2s 黑屏重启一次（30s 预热 >> 2s 超时），看起来像"板子坏了" | 任何超过超时时间的阻塞循环内部都要 `IWDG_Feed()` |
| 7 | **从备份还原源码后 Keil 不重新编译** | 改了代码但板子行为没变 —— `Copy-Item` 保留了旧时间戳，比 `.o` 还旧，Keil 判定"无需重编" | 还原后刷新文件时间戳，或直接 `Rebuild` 全量编译 |
| 8 | **printf 格式串要花 Flash** | 加 `%X` 多 572 B、加 `%s` 多 136 B（MicroLIB 会换成更大的 printf 变体） | 见「代码体积」一节；功能够了就别再堆格式串 |

## 环境信息（换机器时用得上）

| 项 | 值 |
|---|---|
| IDE | Keil µVision5，目标 STM32F103C8，STM32F10x 标准库 |
| 下载器 | ST-Link（Connect under Reset 可解 SWD 掉线） |
| git | GitHub Desktop 自带：`C:\Users\21142\AppData\Local\GitHubDesktop\app-3.6.10\resources\app\git\cmd\git.exe` |
| PC 编译器 | 无 gcc/make；可用 VS 2022 的 `cl.exe`（先 `call vcvars64.bat`，编译加 `/utf-8`） |
| 串口 | USART1，115200 8N1，PA9/PA10 |

## 会话恢复方式

新会话第一句直接说：

> 继续 stm32-smart-fire-system，先读 `docs/status.md`

然后我会读这个文件 + `README.md` + `app/alarm.c` + `tests/test_alarm.c`，接上上下文再动手。
