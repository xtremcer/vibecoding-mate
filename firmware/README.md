# 第一阶段：USB HID 测试固件

本阶段只验证：

- Pico 能枚举为 USB HID 键盘
- GP2 接地时，发送并保持 Left Win + Grave
- GP2 松开时，释放全部键
- PTT 按下时点亮 GP25 板载 LED
- GP3～GP10 提供 8 个编辑、CLI 和长时语音快捷键

长时语音键默认发送 `Ctrl+Win+Shift+F9`。请在微信输入法中将该组合键设置为长时语音的开始/停止快捷键。

工程依赖 Raspberry Pi Pico SDK 和 TinyUSB。音频功能将在 HID 验证通过后加入。
