# USB 音频方案调研与决策

## 结论

当前基于 TinyUSB 内置 UAC2 的方案在 Windows 上出现过 Code 10、`STATUS_RANGE_NOT_FOUND`、设置页面卡顿、音频设备消失以及复合设备 HID 按键释放异常。INMP441 的 PIO、DMA 和 I2S 时钟已经通过独立测试确认正常，因此下一阶段改为 UAC1。

UAC1 方案关闭 `CFG_TUD_AUDIO`，通过 TinyUSB 的 `usbd_app_driver_get_cb()` 注册自定义音频类驱动，同时保留 TinyUSB HID 类驱动。音频接口先固定为 48 kHz、16 bit、单声道、等时 IN，便于先验证 Windows 稳定枚举，再逐步扩展采样率和完整按键功能。

## 参考项目

- [WeebLabs USBrx](https://github.com/WeebLabs/USBrx)：RP2040 自定义 UAC1 类驱动、手写 UAC1 描述符、等时 IN 端点和数据环形缓冲。它是本项目 USB 音频类驱动的主要参考。
- [pico-usb-microphone-bin](https://github.com/hyx0329/pico-usb-microphone-bin)：INMP441 + RP2040 的 UAC1 成功案例，使用 GP18/GP19/GP20，并在 Windows 10 测试通过；但只发布二进制固件。
- [pico-usb-headset](https://github.com/denisgav/pico-usb-headset)：可参考 PIO/I2S/DMA 数据路径，但其 USB 音频仍基于 UAC2，不作为 Windows 兼容性方案。
- [TinyUSB UAC2 headset example](https://docs.tinyusb.org/en/stable/examples/device/uac2_headset.html)：用于确认 SDK 内置音频驱动的接口边界。

## 当前实验目标

`vibecoding_mate_uac1_hid` 是独立实验目标，生成 `firmware/build/vibecoding_mate_uac1_hid.uf2`。它包含：

- 自定义 UAC1 麦克风接口；
- TinyUSB HID 键盘接口；
- INMP441 的现有 PIO + DMA 环形缓冲；
- GP2 PTT；
- GP2 按下时发送 Win+`，松开时发送释放报告；
- 首轮固定 48 kHz、16 bit、单声道，音频未按下 PTT 时发送静音。

该实验目标不会替换 `vibecoding_mate_hid.uf2`。只有在 Windows 设备枚举、声音输入设备稳定存在、HID 不再卡键之后，才合并完整实体按键和最终 PTT 行为。

## 测试顺序

1. 只验证设备管理器中是否出现音频接口和 HID 接口。
2. 验证 Windows 声音输入中能否选择设备，设置页面是否保持流畅。
3. 使用录音工具观察静音/有声切换，不先测试微信输入法。
4. 验证 GP2 按下发送 Win+`，松开释放全部键。
5. 失败时立即刷回稳定 HID UF2，不继续让异常复合设备占用 Windows 音频栈。
