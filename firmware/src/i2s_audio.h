#ifndef VIBECODING_MATE_I2S_AUDIO_H
#define VIBECODING_MATE_I2S_AUDIO_H

#include <stdint.h>

void i2s_audio_init(void);
int16_t i2s_audio_read_left(void);

#endif
