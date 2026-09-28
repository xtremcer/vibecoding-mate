#include "tusb.h"
#include <string.h>

#define USB_VID 0xCafe
#define USB_PID 0x4010
#define USB_BCD 0x0100

enum { ITF_NUM_HID, ITF_NUM_TOTAL };
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_HID 0x81

static const uint8_t hid_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return hid_report_desc;
}

static const uint8_t desc_device[] = {
    TUSB_DESC_DEVICE(0x0200, TUSB_CLASS_MISC, MISC_SUBCLASS_COMMON,
                     MISC_PROTOCOL_IAD, 64, USB_VID, USB_PID, USB_BCD,
                     0x01, 0x02, 0x03, 0x01)
};

uint8_t const* tud_descriptor_device_cb(void) {
    return desc_device;
}

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_desc), EPNUM_HID, 16, 10)
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

static char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "Vibecoding Mate",
    "Vibecoding Mate HID",
    "0001",
    "Keyboard"
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        _desc_str[1] = 0x0409;
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = (uint8_t) strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) _desc_str[1 + i] = str[i];
    }

    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
