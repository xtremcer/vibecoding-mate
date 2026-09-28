#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#define PTT_GPIO 2
#define LED_GPIO 25

static bool ptt_active = false;

static void update_ptt(void) {
    bool pressed = !gpio_get(PTT_GPIO);
    if (pressed == ptt_active) return;

    ptt_active = pressed;
    gpio_put(LED_GPIO, ptt_active);

    if (tud_hid_ready()) {
        uint8_t keycode[6] = { 0 };
        uint8_t modifier = 0;
        if (ptt_active) {
            modifier = KEYBOARD_MODIFIER_LEFTGUI;
            keycode[0] = HID_KEY_GRAVE;
        }
        tud_hid_keyboard_report(1, modifier, keycode);
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

    while (true) {
        tud_task();
        update_ptt();
        sleep_ms(10);
    }
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
