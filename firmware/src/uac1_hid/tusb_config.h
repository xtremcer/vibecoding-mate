#ifndef VIBECODING_MATE_UAC1_HID_TUSB_CONFIG_H
#define VIBECODING_MATE_UAC1_HID_TUSB_CONFIG_H

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#define CFG_TUSB_OS OPT_OS_PICO
#define CFG_TUSB_DEBUG 0
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

/* The audio function is a custom UAC1 class driver. TinyUSB's built-in
 * audio driver is intentionally disabled because this SDK version's driver
 * only accepts UAC2. HID remains handled by TinyUSB. */
#define CFG_TUD_AUDIO 0
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 1
#define CFG_TUD_HID_EP_BUFSIZE 16
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

#endif
