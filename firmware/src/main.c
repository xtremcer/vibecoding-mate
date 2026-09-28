#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "i2s_audio.h"

#define PTT_GPIO 2
#define LED_GPIO 25

static bool ptt_active = false;
static uint32_t sample_frequency = 48000;
static uint8_t clock_valid = 1;
static bool audio_mute[2] = { false, false };
static int16_t audio_volume[2] = { 0, 0 };
static audio_control_range_4_n_t(2) sample_frequency_range;

static void update_ptt(void) {
    bool pressed = !gpio_get(PTT_GPIO);
    if (pressed == ptt_active) return;

    ptt_active = pressed;
    gpio_put(LED_GPIO, ptt_active);

    if (tud_hid_ready()) {
        uint8_t modifier = ptt_active
            ? (KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_LEFTCTRL)
            : 0;
        tud_hid_keyboard_report(1, modifier, NULL);
    }
}

int main(void) {
    board_init();

    gpio_init(PTT_GPIO);
    gpio_set_dir(PTT_GPIO, GPIO_IN);
    gpio_pull_up(PTT_GPIO);

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    tusb_init();
    i2s_audio_init();

    sample_frequency_range.wNumSubRanges = 2;
    sample_frequency_range.subrange[0].bMin = 16000;
    sample_frequency_range.subrange[0].bMax = 16000;
    sample_frequency_range.subrange[0].bRes = 0;
    sample_frequency_range.subrange[1].bMin = 48000;
    sample_frequency_range.subrange[1].bMax = 48000;
    sample_frequency_range.subrange[1].bRes = 0;

    while (true) {
        tud_task();
        update_ptt();
        sleep_ms(10);
    }
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf,
                                   uint8_t ep_in, uint8_t cur_alt_setting) {
    (void) rhport;
    (void) itf;
    (void) ep_in;
    (void) cur_alt_setting;

    int16_t samples[48];
    for (size_t i = 0; i < 48; i++) {
        samples[i] = ptt_active ? i2s_audio_read_left() : 0;
    }
    tud_audio_write((uint8_t*) samples, sizeof(samples));
    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const* request) {
    (void) rhport;
    (void) request;
    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport,
                                   tusb_control_request_t const* request) {
    (void) rhport;
    (void) request;
    return true;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport,
                                 tusb_control_request_t const* request,
                                 uint8_t* buffer) {
    (void) rhport;
    uint8_t channel = TU_U16_LOW(request->wValue);
    uint8_t control = TU_U16_HIGH(request->wValue);
    uint8_t entity = TU_U16_HIGH(request->wIndex);

    if (entity != 2 || channel > 1 || request->bRequest != AUDIO_CS_REQ_CUR) {
        return false;
    }

    if (control == AUDIO_FU_CTRL_MUTE && request->wLength == 1) {
        audio_mute[channel] = ((audio_control_cur_1_t*) buffer)->bCur;
        return true;
    }

    if (control == AUDIO_FU_CTRL_VOLUME &&
        request->wLength == sizeof(audio_control_cur_2_t)) {
        audio_volume[channel] = ((audio_control_cur_2_t*) buffer)->bCur;
        return true;
    }

    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport,
                                 tusb_control_request_t const* request) {
    uint8_t channel = TU_U16_LOW(request->wValue);
    uint8_t control = TU_U16_HIGH(request->wValue);
    uint8_t entity = TU_U16_HIGH(request->wIndex);

    if (entity == 1 && control == AUDIO_TE_CTRL_CONNECTOR) {
        audio_desc_channel_cluster_t cluster = { 1, 0, 0 };
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, request, &cluster, sizeof(cluster));
    }

    if (entity == 2 && channel <= 1) {
        if (control == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR) {
            return tud_control_xfer(rhport, request, &audio_mute[channel], 1);
        }
        if (control == AUDIO_FU_CTRL_VOLUME) {
            if (request->bRequest == AUDIO_CS_REQ_CUR) {
                return tud_control_xfer(rhport, request,
                                        &audio_volume[channel], sizeof(int16_t));
            }
            if (request->bRequest == AUDIO_CS_REQ_RANGE) {
                audio_control_range_2_n_t(1) range;
                range.wNumSubRanges = 1;
                range.subrange[0].bMin = -90;
                range.subrange[0].bMax = 0;
                range.subrange[0].bRes = 1;
                return tud_audio_buffer_and_schedule_control_xfer(
                    rhport, request, &range, sizeof(range));
            }
        }
    }

    if (entity == 4 && control == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            return tud_control_xfer(rhport, request,
                                    &sample_frequency, sizeof(sample_frequency));
        }
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            return tud_control_xfer(rhport, request,
                                    &sample_frequency_range,
                                    sizeof(sample_frequency_range));
        }
    }

    if (entity == 4 && control == AUDIO_CS_CTRL_CLK_VALID &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        return tud_control_xfer(rhport, request, &clock_valid, sizeof(clock_valid));
    }

    return false;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}
