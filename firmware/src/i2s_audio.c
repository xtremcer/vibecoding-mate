#include "i2s_audio.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "i2s_rx.pio.h"

#define I2S_BCLK_GPIO 18
#define I2S_WS_GPIO 19
#define I2S_DATA_GPIO 20
#define AUDIO_SAMPLE_RATE 48000u

static PIO i2s_pio = pio0;
static uint i2s_sm = 0;

void i2s_audio_init(void) {
    uint offset = pio_add_program(i2s_pio, &i2s_rx_program);
    pio_sm_config c = i2s_rx_program_get_default_config(offset);

    sm_config_set_in_pins(&c, I2S_DATA_GPIO);
    sm_config_set_sideset_pins(&c, I2S_BCLK_GPIO);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    sm_config_set_clkdiv(&c,
        (float) clock_get_hz(clk_sys) / (AUDIO_SAMPLE_RATE * 128.0f));

    pio_gpio_init(i2s_pio, I2S_BCLK_GPIO);
    pio_gpio_init(i2s_pio, I2S_WS_GPIO);
    pio_gpio_init(i2s_pio, I2S_DATA_GPIO);
    pio_sm_set_consecutive_pindirs(i2s_pio, i2s_sm, I2S_BCLK_GPIO, 2, true);
    pio_sm_set_consecutive_pindirs(i2s_pio, i2s_sm, I2S_DATA_GPIO, 1, false);
    pio_sm_init(i2s_pio, i2s_sm, offset, &c);
    pio_sm_set_enabled(i2s_pio, i2s_sm, true);
}

int16_t i2s_audio_read_left(void) {
    uint32_t raw = pio_sm_get_blocking(i2s_pio, i2s_sm);
    return (int16_t) ((int32_t) raw >> 16);
}
