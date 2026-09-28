# 硬件接线方案

## 1. 推荐 GPIO 分配

| 功能 | Pico GPIO | Pico 排针脚位 |
|---|---:|---:|
| INMP441 SCK/BCLK | GP18 | 24 |
| INMP441 WS/LRCLK | GP19 | 25 |
| INMP441 SD/DATA | GP20 | 26 |
| PTT | GP2 | 4 |
| 功能键 1 | GP3 | 5 |
| 功能键 2 | GP4 | 6 |
| 功能键 3 | GP5 | 7 |
| 功能键 4 | GP6 | 9 |
| 功能键 5 | GP7 | 10 |
| 功能键 6 | GP8 | 11 |
| 功能键 7 | GP9 | 12 |
| 功能键 8 | GP10 | 14 |
| 状态 LED | GP25 | Pico 板载 LED |

GP18、GP19、GP20 连续，适合使用一个 PIO 状态机实现 I²S 接收。USB D+/D- 不占用这些 GPIO，USB 复合设备由 Pico 的 USB 控制器处理。

## 2. INMP441 接线

| INMP441 模块 | Raspberry Pi Pico |
|---|---|
| VDD | 3V3(OUT)，物理脚 36 |
| GND | 任意 GND |
| SCK | GP18，物理脚 24 |
| WS | GP19，物理脚 25 |
| SD | GP20，物理脚 26 |
| L/R | GND |
| CHIPEN（如果模块引出） | 3V3 |

注意：

- INMP441 只能接 3.3V 逻辑，不能接 Pico 的 VBUS/5V。
- L/R 接 GND 时，麦克风输出左声道；固件只读取左声道槽。
- 在 VDD 和 GND 之间、靠近麦克风模块放置 0.1 µF 去耦电容。
- 如果 SD 数据线出现浮动或噪声，可在 SD 和 GND 之间增加约 100 kΩ 下拉电阻；部分模块已经带有该电阻。
- 不要在 INMP441 未供电时给 SCK 或 WS 提供活动时钟。

## 3. 按键接法

每个按键一端接对应 GPIO，另一端接 GND。固件把 GPIO 配置为内部上拉：

- 未按下：GPIO 读取高电平
- 按下：GPIO 被接地，读取低电平

因此不需要为每个按键额外购买上拉电阻。PTT 和其他功能键都采用相同接法，并由固件进行去抖。

## 4. LED 接法

第一阶段直接使用 Pico 板载 LED（GP25）。如果需要外接 LED，建议使用 GP22，并串联 330 Ω～1 kΩ 电阻后接 LED，再接 GND。

## 5. 音频实现约束

INMP441 的 I²S 数据格式为 24-bit、MSB first，并要求每个立体声帧包含 64 个 SCK 周期。固件建议按每声道 32-bit 槽接收，然后提取有效 24-bit 数据并转换为单声道 16-bit、16 kHz USB 音频。
