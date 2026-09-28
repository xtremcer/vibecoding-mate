#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#define PTT_GPIO 2
#define LED_GPIO 25
#define FUNCTION_KEY_COUNT 8

typedef struct {
    uint gpio;
    uint8_t modifier;
    uint8_t keycode;
} function_key_t;

// All function keys use the internal pull-up and connect to GND when pressed.
// The last key is a toggle for WeChat Input Method long voice input.
static const function_key_t function_keys[FUNCTION_KEY_COUNT] = {
    { 3, 0, HID_KEY_ENTER },
    { 4, 0, HID_KEY_BACKSPACE },
    { 5, KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_A },
    { 6, KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_C },
    { 7, KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_V },
    { 8, KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_L },
    { 9, 0, HID_KEY_ESCAPE },
    { 10, KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_LEFTGUI |
          KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_F9 },
};

static bool ptt_active = false;
static bool function_raw_state[FUNCTION_KEY_COUNT] = { false };
static bool function_stable_state[FUNCTION_KEY_COUNT] = { false };
static uint32_t function_changed_at[FUNCTION_KEY_COUNT] = { 0 };
static bool long_voice_active = false;
static bool one_shot_report_active = false;
static uint32_t one_shot_release_at = 0;

static void release_all_keys(void) {
    if (tud_hid_ready()) {
        uint8_t keycode[6] = { 0 };
        tud_hid_keyboard_report(1, 0, keycode);
    }
    one_shot_report_active = false;
}

static void send_one_shot(uint8_t modifier, uint8_t keycode) {
    if (!tud_hid_ready() || one_shot_report_active) return;

    uint8_t keycodes[6] = { keycode, 0 };
    tud_hid_keyboard_report(1, modifier, keycodes);
    one_shot_report_active = true;
    one_shot_release_at = to_ms_since_boot(get_absolute_time()) + 35;
}

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

static void update_function_keys(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (one_shot_report_active && (int32_t)(now - one_shot_release_at) >= 0) {
        release_all_keys();
    }

    // Do not inject unrelated shortcuts while the PTT audio session is active.
    if (ptt_active) return;

    for (uint i = 0; i < FUNCTION_KEY_COUNT; ++i) {
        bool raw_pressed = !gpio_get(function_keys[i].gpio);
        if (raw_pressed != function_raw_state[i]) {
            function_raw_state[i] = raw_pressed;
            function_changed_at[i] = now;
        }

        bool pressed = function_stable_state[i];
        bool stable_transition = false;
        if (function_raw_state[i] != function_stable_state[i] &&
            (uint32_t)(now - function_changed_at[i]) >= 25) {
            function_stable_state[i] = function_raw_state[i];
            pressed = function_stable_state[i];
            stable_transition = true;
        }

        if (stable_transition && pressed) {
            if (i == 7) {
                long_voice_active = !long_voice_active;
                // Configure WeChat Input Method long voice shortcut as
                // Ctrl+Win+Shift+F9. One press starts, the next stops.
            }
            send_one_shot(function_keys[i].modifier, function_keys[i].keycode);
        }

        if (!function_stable_state[i]) {
            function_changed_at[i] = 0;
        }
    }
}

int main(void) {
    board_init();

    gpio_init(PTT_GPIO);
    gpio_set_dir(PTT_GPIO, GPIO_IN);
    gpio_pull_up(PTT_GPIO);

    for (uint i = 0; i < FUNCTION_KEY_COUNT; ++i) {
        gpio_init(function_keys[i].gpio);
        gpio_set_dir(function_keys[i].gpio, GPIO_IN);
        gpio_pull_up(function_keys[i].gpio);
    }

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, false);

    tusb_init();

    while (true) {
        tud_task();
        update_ptt();
        update_function_keys();
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
