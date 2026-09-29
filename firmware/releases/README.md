# 固件存档

这些 UF2 是本项目实际构建并用于硬件测试过的版本。开发时应优先重新构建；这里的文件用于接手者快速复现实验和回退。

| 文件 | 用途 |
|---|---|
| `vibecoding_mate_hid.uf2` | 稳定 HID 回退版本 |
| `vibecoding_mate_uac1_hid.uf2` | UAC1 + HID + PTT 实验版本 |
| `vibecoding_mate_uac1_tone.uf2` | USB 音频 1 kHz 方波验证 |
| `vibecoding_mate_uac1_mic_test.uf2` | 连续 INMP441 采集诊断 |
| `vibecoding_mate_uac1_mic_shift8.uf2` | I2S 位移诊断 |
| `vibecoding_mate_i2s_test.uf2` | 独立 I2S/PIO/DMA 诊断 |
| `vibecoding_mate_audio_baseline.uf2` | TinyUSB UAC2 基线 |
| `vibecoding_mate_audio_hid.uf2` | 早期 UAC2 + HID 原型 |

对应的源代码和构建目标在 `firmware/`；实验原因和结果在 `docs/handoff.md`。
