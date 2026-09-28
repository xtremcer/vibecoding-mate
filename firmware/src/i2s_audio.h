#ifndef VIBECODING_MATE_I2S_AUDIO_H
#define VIBECODING_MATE_I2S_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void i2s_audio_init(void);
void i2s_audio_task(void);
void i2s_audio_flush(void);
size_t i2s_audio_read(int16_t* samples, size_t count);

#endif
