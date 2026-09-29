#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "device/usbd_pvt.h"
#include "class/audio/audio.h"
#include "i2s_audio.h"

#define PTT_GPIO 2
#define LED_GPIO 25
#define AUDIO_IN_EP 0x81
#define AUDIO_PACKET_BYTES 96
#define UAC1_REQ_SET_CUR 0x01
#define UAC1_REQ_GET_CUR 0x81

static bool ptt_active;
static bool hid_release_pending;
static uint32_t ptt_changed_at;
static volatile bool audio_streaming;
static uint8_t audio_packet[AUDIO_PACKET_BYTES] __attribute__((aligned(4)));

static void hid_release_task(void) {
    if (hid_release_pending && tud_hid_ready()) {
        uint8_t keys[6] = {0};
        tud_hid_keyboard_report(1, 0, keys);
        hid_release_pending = false;
    }
}

static void ptt_task(void) {
    bool pressed = !gpio_get(PTT_GPIO);
    if (pressed == ptt_active) return;
    ptt_active = pressed;
    ptt_changed_at = to_ms_since_boot(get_absolute_time());
    gpio_put(LED_GPIO, pressed);
    if (pressed) {
        i2s_audio_flush();
        if (tud_hid_ready()) {
            uint8_t keys[6] = {HID_KEY_GRAVE, 0, 0, 0, 0, 0};
            tud_hid_keyboard_report(1, KEYBOARD_MODIFIER_LEFTGUI, keys);
        }
    } else {
        hid_release_pending = true;
    }
}

static uint16_t audio_packet_fill(void) {
    memset(audio_packet, 0, sizeof(audio_packet));
    if (ptt_active) {
        i2s_audio_read((int16_t *)audio_packet, AUDIO_PACKET_BYTES / 2);
    }
    return AUDIO_PACKET_BYTES;
}

static struct {
    uint8_t ac_itf;
    uint8_t as_itf;
    uint8_t cur_alt;
    bool ep_open;
    const uint8_t *ep_desc;
    uint8_t pending_cs;
} uac1;
static uint8_t ctrl_buf[4];
static uint32_t sample_rate = 48000;

static void uac1_arm(uint8_t rhport) {
    usbd_edpt_xfer(rhport, AUDIO_IN_EP, audio_packet, audio_packet_fill());
}

static bool uac1_set_alt(uint8_t rhport, uint8_t alt) {
    if (alt > 1) return false;
    if (alt == uac1.cur_alt) return true;
    uac1.cur_alt = alt;
    if (alt == 1) {
        if (!usbd_edpt_open(rhport, (tusb_desc_endpoint_t const *)uac1.ep_desc)) return false;
        uac1.ep_open = true;
        audio_streaming = true;
        uac1_arm(rhport);
    } else {
        if (uac1.ep_open) usbd_edpt_close(rhport, AUDIO_IN_EP);
        uac1.ep_open = false;
        audio_streaming = false;
    }
    return true;
}

static void uac1_init(void) { memset(&uac1, 0, sizeof(uac1)); }
static bool uac1_deinit(void) { return true; }
static void uac1_reset(uint8_t rhport) {
    (void)rhport;
    uac1.cur_alt = 0;
    uac1.ep_open = false;
    audio_streaming = false;
}

static uint16_t uac1_open(uint8_t rhport, tusb_desc_interface_t const *itf, uint16_t max_len) {
    TU_VERIFY(itf->bInterfaceClass == TUSB_CLASS_AUDIO &&
              itf->bInterfaceSubClass == AUDIO_SUBCLASS_CONTROL &&
              itf->bAlternateSetting == 0);
    uac1.ac_itf = itf->bInterfaceNumber;
    const uint8_t *p = (const uint8_t *)itf;
    const uint8_t *end = p + max_len;
    uint16_t used = 0;
    used += tu_desc_len(p); p += tu_desc_len(p);
    while (p < end && tu_desc_type(p) == TUSB_DESC_CS_INTERFACE) { used += tu_desc_len(p); p += tu_desc_len(p); }
    while (p < end && tu_desc_type(p) == TUSB_DESC_INTERFACE) {
        tusb_desc_interface_t const *as = (tusb_desc_interface_t const *)p;
        if (as->bInterfaceClass != TUSB_CLASS_AUDIO || as->bInterfaceSubClass != AUDIO_SUBCLASS_STREAMING) break;
        uac1.as_itf = as->bInterfaceNumber;
        used += tu_desc_len(p); p += tu_desc_len(p);
        while (p < end && tu_desc_type(p) != TUSB_DESC_INTERFACE) {
            if (tu_desc_type(p) == TUSB_DESC_ENDPOINT) {
                tusb_desc_endpoint_t const *ep = (tusb_desc_endpoint_t const *)p;
                if (ep->bmAttributes.xfer == TUSB_XFER_ISOCHRONOUS && tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
                    uac1.ep_desc = p;
                    /* RP2040 TinyUSB uses the regular endpoint-open path. */
                }
            }
            used += tu_desc_len(p); p += tu_desc_len(p);
        }
    }
    return used;
}

static bool uac1_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const *req) {
    if (stage == CONTROL_STAGE_SETUP) {
        if (req->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD) {
            uint8_t itf = TU_U16_LOW(req->wIndex);
            if (req->bRequest == TUSB_REQ_SET_INTERFACE) {
                uint8_t alt = TU_U16_LOW(req->wValue);
                if (itf == uac1.ac_itf) return alt == 0 && tud_control_status(rhport, req);
                if (itf == uac1.as_itf && uac1_set_alt(rhport, alt)) return tud_control_status(rhport, req);
                return false;
            }
            if (req->bRequest == TUSB_REQ_GET_INTERFACE) {
                static uint8_t alt;
                alt = (itf == uac1.as_itf) ? uac1.cur_alt : 0;
                return tud_control_xfer(rhport, req, &alt, 1);
            }
            return false;
        }
        if (req->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
            req->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT &&
            TU_U16_LOW(req->wIndex) == AUDIO_IN_EP &&
            TU_U16_HIGH(req->wValue) == AUDIO_CS_CTRL_SAM_FREQ) {
            if (req->bRequest == UAC1_REQ_GET_CUR && req->bmRequestType_bit.direction == TUSB_DIR_IN) {
                static uint8_t freq[3];
                freq[0] = sample_rate & 0xff; freq[1] = (sample_rate >> 8) & 0xff; freq[2] = (sample_rate >> 16) & 0xff;
                return tud_control_xfer(rhport, req, freq, 3);
            }
            if (req->bRequest == UAC1_REQ_SET_CUR && req->bmRequestType_bit.direction != TUSB_DIR_IN && req->wLength == 3) {
                uac1.pending_cs = AUDIO_CS_CTRL_SAM_FREQ;
                return tud_control_xfer(rhport, req, ctrl_buf, 3);
            }
        }
        return false;
    }
    if (stage == CONTROL_STAGE_DATA && uac1.pending_cs == AUDIO_CS_CTRL_SAM_FREQ) {
        uint32_t f = ctrl_buf[0] | ((uint32_t)ctrl_buf[1] << 8) | ((uint32_t)ctrl_buf[2] << 16);
        if (f == 48000) sample_rate = f;
        uac1.pending_cs = 0;
    }
    return true;
}

static bool uac1_xfer(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t bytes) {
    (void)result; (void)bytes;
    if (ep_addr == AUDIO_IN_EP && uac1.ep_open) uac1_arm(rhport);
    return ep_addr == AUDIO_IN_EP;
}

static const usbd_class_driver_t uac1_driver = {
    .init = uac1_init,
    .reset = uac1_reset, .open = uac1_open, .control_xfer_cb = uac1_control,
    .xfer_cb = uac1_xfer, .sof = NULL
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *count) {
    *count = 1;
    return &uac1_driver;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)bufsize;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t *buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)type; (void)buffer; (void)reqlen; return 0;
}

int main(void) {
    stdio_init_all();
    gpio_init(PTT_GPIO); gpio_set_dir(PTT_GPIO, GPIO_IN); gpio_pull_up(PTT_GPIO);
    gpio_init(LED_GPIO); gpio_set_dir(LED_GPIO, GPIO_OUT); gpio_put(LED_GPIO, 0);
    i2s_audio_init();
    tud_init(BOARD_TUD_RHPORT);
    while (true) {
        tud_task();
        i2s_audio_task();
        ptt_task();
        hid_release_task();
        if (ptt_active && to_ms_since_boot(get_absolute_time()) - ptt_changed_at > 30000) {
            ptt_active = false; gpio_put(LED_GPIO, 0); hid_release_pending = true;
        }
    }
}
