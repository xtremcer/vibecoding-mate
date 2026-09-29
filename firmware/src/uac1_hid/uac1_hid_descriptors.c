#include "tusb.h"
#include "class/audio/audio.h"
#include "tusb_config.h"
#include <string.h>

#define U16LE(x) (uint8_t)((x) & 0xff), (uint8_t)(((x) >> 8) & 0xff)
#define U24LE(x) (uint8_t)((x) & 0xff), (uint8_t)(((x) >> 8) & 0xff), (uint8_t)(((x) >> 16) & 0xff)

#define ITF_AUDIO_CONTROL 0
#define ITF_AUDIO_STREAMING 1
#define ITF_HID 2
#define ITF_TOTAL 3
#define EP_AUDIO_IN 0x81
#define EP_HID_IN 0x82
#define AUDIO_DESC_LEN 91
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_AUDIO_DESC_IAD_LEN + AUDIO_DESC_LEN + TUD_HID_DESC_LEN)
#define AC_TOTAL_LEN (9 + 12 + 9)

#define INPUT_TERMINAL_ID 1
#define OUTPUT_TERMINAL_ID 2

static const uint8_t hid_report_desc[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0xCafe,
    .idProduct = 0x4012,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&desc_device; }

static const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),

    /* UAC1 audio function IAD */
    8, TUSB_DESC_INTERFACE_ASSOCIATION, ITF_AUDIO_CONTROL, 2,
    TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, 0, 0,

    /* AudioControl interface and class-specific topology. */
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_CONTROL, 0, 0,
    TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_CONTROL, AUDIO_INT_PROTOCOL_CODE_UNDEF, 0,
    9, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_HEADER,
    U16LE(0x0100), U16LE(AC_TOTAL_LEN), 1, ITF_AUDIO_STREAMING,
    12, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_INPUT_TERMINAL,
    INPUT_TERMINAL_ID, U16LE(AUDIO_TERM_TYPE_IN_GENERIC_MIC), 0, 1, U16LE(0x0000), 0, 0,
    9, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AC_INTERFACE_OUTPUT_TERMINAL,
    OUTPUT_TERMINAL_ID, U16LE(AUDIO_TERM_TYPE_USB_STREAMING), 0, INPUT_TERMINAL_ID, 0,

    /* AudioStreaming alt 0 and alt 1. */
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_STREAMING, 0, 0,
    TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, AUDIO_INT_PROTOCOL_CODE_UNDEF, 0,
    9, TUSB_DESC_INTERFACE, ITF_AUDIO_STREAMING, 1, 1,
    TUSB_CLASS_AUDIO, AUDIO_SUBCLASS_STREAMING, AUDIO_INT_PROTOCOL_CODE_UNDEF, 0,
    7, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AS_INTERFACE_AS_GENERAL,
    OUTPUT_TERMINAL_ID, 1, U16LE(AUDIO_FORMAT_TYPE_I),
    11, TUSB_DESC_CS_INTERFACE, AUDIO_CS_AS_INTERFACE_FORMAT_TYPE,
    AUDIO_FORMAT_TYPE_I, 1, 2, 16, 1, U24LE(48000),
    9, TUSB_DESC_ENDPOINT, EP_AUDIO_IN, 0x05, U16LE(96), 1, 0, 0,
    7, TUSB_DESC_CS_ENDPOINT, AUDIO_CS_EP_SUBTYPE_GENERAL, 0x01, 0, U16LE(0),

    TUD_HID_DESCRIPTOR(ITF_HID, 4, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_desc), EP_HID_IN, 16, 10)
};

TU_VERIFY_STATIC(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "UAC1 descriptor length mismatch");

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_desc;
}

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, "Vibecoding Mate", "Vibecoding Mate UAC1", "0002", "Microphone", "Keyboard"
};
static uint16_t desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t count;
    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
        const char *s = string_desc_arr[index];
        count = (uint8_t)strlen(s);
        if (count > 31) count = 31;
        for (uint8_t i = 0; i < count; i++) desc_str[1 + i] = s[i];
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return desc_str;
}
