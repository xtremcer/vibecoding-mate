#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "i2s_rx.pio.h"

#define I2S_SCK_GPIO 18
#define I2S_WS_GPIO 19
#define I2S_DATA_GPIO 20
#define LED_GPIO 25

#define DMA_WORDS 256
#define I2S_PIO_CLOCK_DIV 61.035f

static uint32_t dma_buffer[DMA_WORDS];

static void start_i2s_capture(PIO pio, uint sm, int dma_chan) {
    dma_channel_config cfg = dma_channel_get_default_config((uint) dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, pio_get_dreq(pio, sm, false));
    dma_channel_configure(
        (uint) dma_chan, &cfg,
        dma_buffer,
        &pio->rxf[sm],
        DMA_WORDS,
        true
    );
}

int main(void) {
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    PIO pio = pio0;
    uint sm = (uint) pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &i2s_rx_program);

    pio_gpio_init(pio, I2S_SCK_GPIO);
    pio_gpio_init(pio, I2S_WS_GPIO);
    pio_gpio_init(pio, I2S_DATA_GPIO);

    pio_sm_config config = i2s_rx_program_get_default_config(offset);
    sm_config_set_sideset_pins(&config, I2S_SCK_GPIO);
    sm_config_set_in_pins(&config, I2S_DATA_GPIO);
    sm_config_set_in_shift(&config, true, true, 32);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_RX);
    sm_config_set_clkdiv(&config, I2S_PIO_CLOCK_DIV);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_SCK_GPIO, 2, true);
    pio_sm_set_consecutive_pindirs(pio, sm, I2S_DATA_GPIO, 1, false);
    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_enabled(pio, sm, true);

    int dma_chan = dma_claim_unused_channel(true);
    start_i2s_capture(pio, sm, dma_chan);

    while (true) {
        if (!dma_channel_is_busy((uint) dma_chan)) {
            bool data_seen = false;
            for (uint i = 0; i < DMA_WORDS; ++i) {
                if (dma_buffer[i] != 0) {
                    data_seen = true;
                    break;
                }
            }

            // A lit LED means the PIO + DMA path received nonzero data.
            gpio_put(LED_GPIO, data_seen);
            start_i2s_capture(pio, sm, dma_chan);
        }
        sleep_ms(10);
    }
}
