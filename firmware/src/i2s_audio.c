#include "i2s_audio.h"

#include "hardware/dma.h"
#include "hardware/pio.h"
#include "i2s_rx.pio.h"
#include "pico/stdlib.h"

#define I2S_SCK_GPIO 18
#define I2S_WS_GPIO 19
#define I2S_DATA_GPIO 20
/* 125 MHz / 20.345 = 6.144 MHz PIO instruction rate. Each I2S bit uses
 * two instructions, giving 3.072 MHz BCLK = 48 kHz x 64 bits. */
#define I2S_PIO_CLOCK_DIV 20.345f
#define DMA_WORDS 96
#define AUDIO_RING_SAMPLES 1024

static PIO audio_pio = pio0;
static uint audio_sm;
static int audio_dma_chan;
static uint32_t dma_buffer[DMA_WORDS];
static int16_t audio_ring[AUDIO_RING_SAMPLES];
static volatile uint32_t ring_read = 0;
static volatile uint32_t ring_write = 0;

static void start_dma(void) {
    dma_channel_config cfg = dma_channel_get_default_config((uint) audio_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, pio_get_dreq(audio_pio, audio_sm, false));
    dma_channel_configure((uint) audio_dma_chan, &cfg,
                          dma_buffer, &audio_pio->rxf[audio_sm],
                          DMA_WORDS, true);
}

void i2s_audio_init(void) {
    uint offset = pio_add_program(audio_pio, &i2s_rx_program);
    audio_sm = (uint) pio_claim_unused_sm(audio_pio, true);
    audio_dma_chan = dma_claim_unused_channel(true);

    pio_gpio_init(audio_pio, I2S_SCK_GPIO);
    pio_gpio_init(audio_pio, I2S_WS_GPIO);
    pio_gpio_init(audio_pio, I2S_DATA_GPIO);

    pio_sm_config config = i2s_rx_program_get_default_config(offset);
    sm_config_set_sideset_pins(&config, I2S_SCK_GPIO);
    sm_config_set_in_pins(&config, I2S_DATA_GPIO);
    // Left-shift input so the first I2S bit becomes the MSB of the 32-bit
    // word, as required for 32-bit I2S frames.
    sm_config_set_in_shift(&config, false, true, 32);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_RX);
    sm_config_set_clkdiv(&config, I2S_PIO_CLOCK_DIV);
    pio_sm_set_consecutive_pindirs(audio_pio, audio_sm, I2S_SCK_GPIO, 2, true);
    pio_sm_set_consecutive_pindirs(audio_pio, audio_sm, I2S_DATA_GPIO, 1, false);
    pio_sm_init(audio_pio, audio_sm, offset, &config);
    pio_sm_set_enabled(audio_pio, audio_sm, true);
    start_dma();
}

void i2s_audio_task(void) {
    if (dma_channel_is_busy((uint) audio_dma_chan)) return;

    for (size_t i = 0; i < DMA_WORDS; i += 2) {
        // INMP441 supplies a 24-bit left-justified sample in the left slot.
        // Keep the most significant 16 bits for the USB PCM stream.  Using
        // a narrower shift here made the signal unnecessarily small and could
        // leave Windows' input meter looking flat.
#ifdef VIBECODING_I2S_SHIFT8
        int16_t sample = (int16_t) ((int32_t) dma_buffer[i] >> 8);
#else
        int16_t sample = (int16_t) ((int32_t) dma_buffer[i] >> 16);
#endif
        uint32_t next_write = (ring_write + 1) % AUDIO_RING_SAMPLES;
        if (next_write != ring_read) {
            audio_ring[ring_write] = sample;
            ring_write = next_write;
        }
    }

    start_dma();
}

void i2s_audio_flush(void) {
    ring_read = ring_write;
}

size_t i2s_audio_read(int16_t* samples, size_t count) {
    size_t read_count = 0;
    while (read_count < count && ring_read != ring_write) {
        samples[read_count++] = audio_ring[ring_read];
        ring_read = (ring_read + 1) % AUDIO_RING_SAMPLES;
    }
    return read_count;
}
