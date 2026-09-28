# 硬件接线方案

## 1. 推荐 GPIO 分配

| 功能 | Pico GPIO | Pico 排针脚位 |
|---|---:|---:|
| INMP441 SCK/BCLK | GP18 | 24 |
| INMP441 WS/LRCLK | GP19 | 25 |
| INMP441 SD/DATA | GP20 | 26 |
| PTT | GP2 | 4 |
| 功能键 1：Enter | GP3 | 5 |
| 功能键 2：Backspace | GP4 | 6 |
| 功能键 3：Ctrl+A | GP5 | 7 |
| 功能键 4：Ctrl+C | GP6 | 9 |
| 功能键 5：Ctrl+V | GP7 | 10 |
| 功能键 6：Ctrl+L | GP8 | 11 |
| 功能键 7：Esc | GP9 | 12 |
| 功能键 8：长时语音切换 | GP10 | 14 |
| 扩展状态 LED 1 | GP11 | 15 |
| 扩展状态 LED 2 | GP12 | 16 |
| 扩展状态 LED 3 | GP13 | 17 |
| 蜂鸣器 | GP14 | 19 |
| 备用扩展接口 | GP15 | 20 |
| OLED SDA（I²C0） | GP16 | 21 |
| OLED SCL（I²C0） | GP17 | 22 |
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

### 功能键映射

| 按键 | 默认 HID 输出 | 用途 |
|---|---|---|
| 1 | Enter | 提交 Codex、WorkBuddy、Claude CLI 当前输入 |
| 2 | Backspace | 删除输入 |
| 3 | Ctrl+A | 全选当前输入 |
| 4 | Ctrl+C | 中断/复制；CLI 中也常用于停止当前命令 |
| 5 | Ctrl+V | 粘贴 |
| 6 | Ctrl+L | 清理终端显示，适合 CLI |
| 7 | Esc | 取消当前菜单、建议或操作 |
| 8 | Ctrl+Win+Shift+F9 | 微信输入法长时语音：按一次开始，再按一次停止 |

第 8 个组合键故意选择了不常见的组合，避免误触 Windows 常用快捷键。请在微信输入法中把“长时语音输入”的快捷键设置为 `Ctrl+Win+Shift+F9`。

## 4. LED 接法

第一阶段直接使用 Pico 板载 LED（GP25）。扩展状态输出预留如下：

- GP11：USB/设备就绪 LED
- GP12：PTT/音频活动 LED
- GP13：长时语音锁存状态 LED
- GP14：蜂鸣器，建议使用三极管或小型有源蜂鸣器驱动，不要让 GPIO 直接带动大功率负载

每个外接 LED 都应串联 330 Ω～1 kΩ 电阻后接 GND。GP15 为备用扩展，GP16/GP17 已专门预留给 I²C OLED。

### 单色 OLED 4 针

推荐使用常见 SSD1306/SH1106 I²C 单色 OLED：

| OLED 引脚 | Pico |
|---|---|
| VCC | 3V3(OUT)，物理脚 36 |
| GND | GND |
| SDA | GP16，物理脚 21 |
| SCL | GP17，物理脚 22 |

OLED 与按键、INMP441 共用 3.3 V 和 GND。固件后续使用 I²C0 驱动 OLED；显示内容计划包括 USB 状态、PTT/音频状态、长时语音状态、当前模式和错误码。OLED 未接入前，GP25 和外接状态 LED 仍可独立工作。

## 5. 音频实现约束

INMP441 的 I²S 数据格式为 24-bit、MSB first，并要求每个立体声帧包含 64 个 SCK 周期。固件建议按每声道 32-bit 槽接收，然后提取有效 24-bit 数据并转换为单声道 16-bit、16 kHz USB 音频。
