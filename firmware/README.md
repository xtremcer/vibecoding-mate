# 第一阶段：USB HID 测试固件

本阶段只验证：

- Pico 能枚举为 USB HID 键盘
- GP2 接地时，发送并保持 Left Win + Left Ctrl
- GP2 松开时，释放两个修饰键
- PTT 按下时点亮 GP25 板载 LED

工程依赖 Raspberry Pi Pico SDK 和 TinyUSB。音频功能将在 HID 验证通过后加入。
