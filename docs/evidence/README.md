# 实验证据

这里保存本轮硬件测试产生的 WAV 录音和频谱图：

- `uac1_tone_capture.wav`：1 kHz 方波，证明 Windows UAC1 接收链路正常；
- `inmp441_capture_01.wav`、`inmp441_capture_03_piofix.wav`：原始 I2S 采集测试；
- `inmp441_capture_shift8.wav`：改变 16 位数据切片后的诊断；
- `inmp441_capture_wsinvert.wav`：反向 WS 槽位后的诊断，几乎静音；
- `inmp441_capture_after_research.wav`：恢复正确左槽后的最新录音，仍然满幅异常；
- `inmp441_spectrum*.png`：对应频谱图。
- `diag_raw_idle.wav`、`diag_raw_11s.wav`：0 dB 原始诊断；
- `diag_gain6_current.wav`：6 dB 诊断，仍有削顶；
- `diag_gain12_current.wav`、`diag_gain12_spectrum.png`：12 dB 诊断，削顶改善但噪声频谱异常。

这些文件是诊断证据，不是生产音频素材。最新结论见 `docs/handoff.md`。
