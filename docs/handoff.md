# Vibecoding Mate 项目存档与 AI 交接说明

更新时间：2026-09-29  
当前分支：`main`  
远程仓库：`git@github.com:xtremcer/vibecoding-mate.git`

这份文档是本项目当前的“存档点”。后续接手者应先读完本文，再操作 Pico 或修改固件。

## 1. 项目目标

使用 Raspberry Pi Pico（RP2040）、INMP441、实体按键和 LED，制作一个单 USB 复合设备：

1. USB Audio Class 数字麦克风；
2. USB HID 自定义键盘；
3. PTT 按住时采集并传输音频，同时保持 Win+`；
4. PTT 松开时停止有效音频传输并释放全部按键；
5. 其他实体键只发送单键或组合键，不启动麦克风；
6. Windows 优先，不依赖常驻上位机；微信输入法负责语音识别；
7. 后续加入状态 LED、蜂鸣器和 I²C OLED。

## 2. 当前已验证成果

### 已稳定验证

- `vibecoding_mate_hid.uf2`：稳定 HID 固件。
- Pico 能枚举为 HID 键盘。
- GP2 接地能够触发 PTT，板载 LED 能反映状态。
- HID 设备在 Windows 设备管理器中可见。
- `Win+\`` 曾用于短时 PTT 触发测试；Windows/微信输入法快捷键行为需要继续以最终固件复测。
- GitHub SSH 已验证：`ssh -T git@github.com` 返回 `Hi xtremcer!`。

### 已独立证明

- 自定义 UAC1 + HID 复合设备能在 Windows 枚举。
- Windows 能识别 `Vibecoding Mate UAC1` 音频输入设备。
- Windows DirectShow 能持续录音。
- 1 kHz 方波固件录音结果正确，证明 USB UAC1 描述符、等时音频端点、Windows 接收链路和 WAV 录音链路均可工作。
- 当前真正未解决的问题集中在 `INMP441 SD → RP2040 PIO/DMA` 的有效采样数据，不应再优先怀疑 Windows USB 驱动。

## 3. 当前硬件接线

### INMP441

| INMP441 | Pico |
|---|---|
| VDD | 3V3(OUT)，物理脚 36 |
| GND | 任意 GND |
| SCK/BCLK | GP18，物理脚 24 |
| WS/LRCLK | GP19，物理脚 25 |
| SD/DOUT | GP20，物理脚 26 |
| L/R | GND，选择左声道槽 |

单个 INMP441 是单麦克风，不会同时产生两个独立声道。`L/R` 是槽位选择：接 GND 放左槽，接 3V3 放右槽。真正双声道需要两只麦克风共用 SCK、WS、SD，一只接 GND、一只接 3V3。

INMP441 的 SD 在未选中的另一半 WS 帧会三态，所以右槽接近静音是正常的。不要把单个麦克风误判为“必须读取左右两个槽”。

### 按键与扩展

| 功能 | GPIO |
|---|---:|
| PTT | GP2 |
| Enter | GP3 |
| Backspace | GP4 |
| Ctrl+A | GP5 |
| Ctrl+C | GP6 |
| Ctrl+V | GP7 |
| Ctrl+L | GP8 |
| Esc | GP9 |
| 长时语音 Win+\\ | GP10 |
| 状态 LED 预留 | GP11、GP12、GP13 |
| 蜂鸣器预留 | GP14 |
| 备用 | GP15 |
| OLED SDA | GP16 |
| OLED SCL | GP17 |
| 板载 LED | GP25 |

所有按键一端接 GPIO，另一端接 GND，固件使用内部上拉。外接 LED 应串联 330 Ω～1 kΩ 电阻；蜂鸣器不要直接由 GPIO 驱动较大负载。

## 4. 本机开发环境

本机已验证的工具链：

- Pico SDK：`D:\Program Files\Raspberry Pi\Pico SDK v1.5.1`
- SDK 根目录：`D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\pico-sdk`
- CMake：`D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\cmake\bin\cmake.exe`
- Ninja：`D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\ninja\ninja.exe`
- picotool：`D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\picotool\picotool.exe`
- 音频分析 FFmpeg：`M:\facefusion\ffmpeg\bin\ffmpeg.exe`
- Pico 官方手册目录：`raspberry_pi_pico/`

SDK 本体不提交到 GitHub；接手者应安装 Pico SDK 1.5.1，并在 CMake 配置时使用该版本。官方安装包来源：<https://github.com/raspberrypi/pico-setup-windows/releases/tag/v1.5.1>。

## 5. 构建与刷机

在项目根目录执行：

```powershell
$cmake='D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\cmake\bin\cmake.exe'
$ninja='D:\Program Files\Raspberry Pi\Pico SDK v1.5.1\ninja\ninja.exe'
& $cmake -S firmware -B firmware\build -G Ninja
& $ninja -C firmware\build vibecoding_mate_hid
```

生成的 UF2 在 `firmware/build/`。Pico 进入 BOOTSEL 后会出现 `RPI-RP2` 磁盘；把目标 UF2 复制进去即可。

当前已验证的目标：

| 目标 | 作用 | 结论 |
|---|---|---|
| `vibecoding_mate_hid` | 稳定 HID 键盘 | 当前安全回退版本 |
| `vibecoding_mate_uac1_hid` | UAC1 + HID + PTT | USB 架构可用，麦克风数据仍待修复 |
| `vibecoding_mate_uac1_tone` | 连续输出 1 kHz 方波 | 已证明 Windows USB 音频链路正常 |
| `vibecoding_mate_uac1_mic_test` | 连续发送 I2S 环形缓冲 | 当前 INMP441 诊断版本 |
| `vibecoding_mate_uac1_mic_shift8` | 使用另一段 16 位数据切片 | 位移诊断，未解决问题 |
| `vibecoding_mate_i2s_test` | 不枚举 USB Audio 的独立 I2S 测试 | 早期电气/时钟诊断 |
| `vibecoding_mate_audio_baseline` | TinyUSB UAC2 基线 | Windows 兼容性失败 |
| `vibecoding_mate_audio_hid` | 早期 UAC2 + HID 原型 | 不作为生产版本 |

## 6. 研究过程、实验和踩坑记录

### UAC2 失败

TinyUSB 内置 UAC2 方案在 Windows 上出现过：

- 设备管理器 Code 10；
- `STATUS_RANGE_NOT_FOUND`；
- 设置页面严重卡顿；
- 音频设备消失；
- HID 按键释放异常。

因此项目转为手写 UAC1 描述符和自定义音频类驱动。后续不要未经验证地切回 UAC2。

### UAC1 成功

参考 WeebLabs USBrx 的自定义 UAC1 驱动，关闭 TinyUSB 内置 Audio 类，通过 `usbd_app_driver_get_cb()` 注册自定义驱动。当前 UAC1 使用 48 kHz、16 bit、单声道、等时 IN 端点，Windows 枚举稳定，不再出现早期设置卡顿。

### 1 kHz 方波测试

`vibecoding_mate_uac1_tone.uf2` 连续发送 1 kHz 方波。Windows 录音统计曾得到：

- RMS 约 `-0.248 dB`；
- 零交叉率约 `0.0415`；
- 与 1 kHz 方波相符。

这证明 USB UAC1 接收链路不是当前故障来源。

### INMP441 连续采集测试

连续采集固件的录音结果反复出现：

- 满幅 `-32768/+32767`；
- RMS 约 `-1～-2 dB`；
- 宽带噪声或固定条纹频谱；
- 没有正常语音波形。

已经做过以下软件实验：

1. 使用 GP18/19/20 生成 48 kHz × 64 BCLK 的 PIO I2S；
2. 使用 DMA + 环形缓冲；
3. 采用与 Arduino-Pico 已验证实现相同思路的 PIO 输入移位；
4. 把 32 位帧的高 16 位改为另一段 16 位切片；
5. 反向 WS 左右槽位；
6. 使用原始 WAV 和频谱图进行离线分析。

反向 WS 后录音几乎只有 `-1/+1`，说明右槽无有效数据，符合 L/R 接地时麦克风只驱动左槽的行为。恢复正确相位后左槽仍满幅异常，因此当前最可能是 SD 线、模块焊点、面包板接触、模块方向、供电/地或 SD 受到时钟串扰，而不是 USB 描述符。

### 常见硬件坑

- L/R 悬空会造成槽位不确定；本项目应保持 L/R→GND。
- INMP441 的 VDD 必须接 3.3 V，不能接 5 V/VBUS。
- SCK、WS、SD 容易在面包板上插错或电源轨断开。
- 模块排针虚焊、插反、焊接残留或背面半环金属线短接都会导致异常噪声。
- VDD 与 GND 附近应放置 0.1 µF 去耦电容。
- SD 线应短，避免与 SCK 长距离并行。
- 非选中声道的 SD 是高阻态，不能用它判断麦克风损坏。

## 7. 当前代码状态与提交策略

本存档提交包含当前 I2S 诊断修改，但它们仍未被标记为生产修复，原因是还没有得到有效语音数据。稳定 HID 提交仍是安全回退点。

当前正式提交历史的关键节点：

| 提交 | 内容 |
|---|---|
| `f8c3fab` | 初始 UAC1 + HID 复合实验 |
| `e456492` | 音频缩放与 PTT HID 时序改进 |
| `2a0a08d` | 48 kHz I2S 时钟与 USB reset 接口 |
| `b2d4522` | 1 kHz USB 音频方波目标 |
| `a21b014` | 连续 INMP441 USB 采集目标 |

只有在重新录音确认“环境声能改变波形、没有满幅削顶、能听到或识别语音”后，才应提交 I2S 修复，并继续测试最终 PTT 固件。

## 8. 后续推荐顺序

1. 断电后重新插拔 INMP441 的 `SD`、`SCK`、`WS`、`VDD`、`GND`，确认面包板电源轨没有断点。
2. 优先用短杜邦线直连 Pico 与模块，暂时绕开面包板。
3. 确认模块方向和排针焊点；不要只看模块丝印，逐针核对实际连接。
4. 保持 `L/R→GND`，不要为了“双声道”读取右槽。
5. 重新刷 `vibecoding_mate_uac1_mic_test.uf2`，录 5 秒环境声并分析。
6. 麦克风数据正确后，重新构建并测试 `vibecoding_mate_uac1_hid`。
7. 最后才恢复 GP3～GP10 功能键、状态 LED、蜂鸣器和 OLED。

## 9. 未来功能安排

- PTT：采用稳定的按下/释放去抖，按下发送同时的 Win+`，释放发送全零 HID 报告。
- 长时语音：GP10 使用 Win+\\，由微信输入法配置为按一次开始、再按一次停止。
- 功能键：Enter、Backspace、Ctrl+A、Ctrl+C、Ctrl+V、Ctrl+L、Esc、长时语音。
- 音频：先保持 UAC1 单声道 48 kHz 16 bit，稳定后再评估 16 kHz 语音路径和采样率声明。
- LED：GP11 表示 USB 就绪，GP12 表示 PTT/音频活动，GP13 表示长时语音锁存。
- 蜂鸣器：GP14 仅输出短提示音，必要时增加三极管驱动。
- OLED：GP16/GP17 使用 I²C0，显示 USB、PTT、音频、长时语音和错误状态。
- 双核：后续可将核心 0 用于 USB/HID/UAC1，核心 1 用于 OLED、LED、蜂鸣器和状态机，但在当前单核稳定前不应提前并行化。

## 10. 接手者第一步

```powershell
cd G:\Desktop4xtremcer\vibecoding-mate
git status
git log --oneline --decorate -12
```

先确认没有覆盖用户的未提交 I2S 诊断改动，再阅读本文、`docs/hardware-wiring.md`、`docs/audio-usb-research.md` 和 `firmware/README.md`。遇到不确定的固件，优先刷回 `vibecoding_mate_hid.uf2`，不要直接继续使用未验证的复合音频固件。
