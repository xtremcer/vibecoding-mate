# 第一阶段：USB HID 测试固件

本阶段只验证：

- Pico 能枚举为 USB HID 键盘
- GP2 接地时，发送并保持 Left Win + Grave
- GP2 松开时，释放全部键
- PTT 按下时点亮 GP25 板载 LED
- GP3～GP10 提供 8 个编辑、CLI 和长时语音快捷键

长时语音键默认发送 `Win+\\`。请在微信输入法中将该组合键设置为长时语音的开始/停止快捷键。

扩展规划：GP11～GP13 为状态 LED，GP14 为蜂鸣器，GP16/GP17 为 I²C OLED（SDA/SCL），GP15 保留备用。

## INMP441 独立测试

工程还会生成 `vibecoding_mate_i2s_test.uf2`。它只测试 GP18/GP19/GP20 上的 INMP441 PIO + DMA 数据流；测试固件由 Pico 主动产生 I²S 的 SCK/WS 时钟，不枚举 USB Audio，也不会替换稳定 HID 功能。测试固件中，GP25 板载 LED 在检测到非零 I²S 数据时点亮。

测试完成后必须重新刷回 `vibecoding_mate_hid.uf2`，才能恢复键盘功能。

工程依赖 Raspberry Pi Pico SDK 和 TinyUSB。音频功能将在 HID 验证通过后加入。
