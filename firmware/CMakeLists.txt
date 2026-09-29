cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(vibecoding_mate_hid C CXX ASM)

pico_sdk_init()

add_executable(vibecoding_mate_hid
    src/main.c
    src/usb_descriptors.c
)

target_include_directories(vibecoding_mate_hid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(vibecoding_mate_hid
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(vibecoding_mate_hid)

# Standalone INMP441 electrical/data-path test. This target is intentionally
# separate from the stable HID firmware and does not expose USB Audio.
add_executable(vibecoding_mate_i2s_test
    src/audio_test_main.c
)

pico_generate_pio_header(vibecoding_mate_i2s_test
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)

target_link_libraries(vibecoding_mate_i2s_test
    pico_stdlib
    hardware_pio
    hardware_dma
)

pico_add_extra_outputs(vibecoding_mate_i2s_test)

# Isolated TinyUSB UAC2 baseline. This target is not the production firmware;
# it is used to validate Windows audio enumeration before adding HID and DMA.
add_executable(vibecoding_mate_audio_baseline
    src/audio_hid/audio_hid_main.c
    src/audio_hid/audio_hid_descriptors.c
)

target_include_directories(vibecoding_mate_audio_baseline PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/audio_hid
)

target_link_libraries(vibecoding_mate_audio_baseline
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(vibecoding_mate_audio_baseline)

# Audio + HID composite prototype. Audio is still the official dummy stream;
# this target validates composite enumeration before wiring in PIO + DMA.
add_executable(vibecoding_mate_audio_hid
    src/audio_hid_prod/audio_hid_prod_main.c
    src/audio_hid_prod/audio_hid_prod_descriptors.c
    src/i2s_audio.c
)

pico_generate_pio_header(vibecoding_mate_audio_hid
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)

target_include_directories(vibecoding_mate_audio_hid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/audio_hid_prod
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(vibecoding_mate_audio_hid
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_pio
    hardware_dma
    pico_usb_reset_interface
)

pico_add_extra_outputs(vibecoding_mate_audio_hid)

# Experimental but standards-compatible UAC1 + HID composite target.  This
# intentionally remains separate from the stable HID target until Windows
# enumeration and audio streaming have both been verified.
add_executable(vibecoding_mate_uac1_hid
    src/uac1_hid/uac1_hid_main.c
    src/uac1_hid/uac1_hid_descriptors.c
    src/i2s_audio.c
)

pico_generate_pio_header(vibecoding_mate_uac1_hid
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)

target_include_directories(vibecoding_mate_uac1_hid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/uac1_hid
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PICO_SDK_PATH}/src/common/pico_usb_reset_interface/include
)

target_link_libraries(vibecoding_mate_uac1_hid
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_pio
    hardware_dma
)

pico_add_extra_outputs(vibecoding_mate_uac1_hid)

# USB-link diagnostic: emits a deterministic 1 kHz PCM test signal so the
# host-side capture path can be verified independently of the INMP441.
add_executable(vibecoding_mate_uac1_tone
    src/uac1_hid/uac1_hid_main.c
    src/uac1_hid/uac1_hid_descriptors.c
    src/i2s_audio.c
)
pico_generate_pio_header(vibecoding_mate_uac1_tone
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)
target_include_directories(vibecoding_mate_uac1_tone PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/uac1_hid
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PICO_SDK_PATH}/src/common/pico_usb_reset_interface/include
)
target_compile_definitions(vibecoding_mate_uac1_tone PRIVATE VIBECODING_UAC1_TONE=1)
target_link_libraries(vibecoding_mate_uac1_tone
    pico_stdlib tinyusb_device tinyusb_board hardware_pio hardware_dma pico_usb_reset_interface
)
pico_add_extra_outputs(vibecoding_mate_uac1_tone)

# INMP441 diagnostic: continuously streams the I2S ring without PTT gating.
add_executable(vibecoding_mate_uac1_mic_test
    src/uac1_hid/uac1_hid_main.c
    src/uac1_hid/uac1_hid_descriptors.c
    src/i2s_audio.c
)
pico_generate_pio_header(vibecoding_mate_uac1_mic_test
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)
target_include_directories(vibecoding_mate_uac1_mic_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/uac1_hid
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PICO_SDK_PATH}/src/common/pico_usb_reset_interface/include
)
target_compile_definitions(vibecoding_mate_uac1_mic_test PRIVATE VIBECODING_UAC1_MIC_TEST=1)
target_link_libraries(vibecoding_mate_uac1_mic_test
    pico_stdlib tinyusb_device tinyusb_board hardware_pio hardware_dma pico_usb_reset_interface
)
pico_add_extra_outputs(vibecoding_mate_uac1_mic_test)

# Same continuous microphone test, but keeps a different 16-bit slice of the
# captured 32-bit I2S word to diagnose 24-bit alignment.
add_executable(vibecoding_mate_uac1_mic_shift8
    src/uac1_hid/uac1_hid_main.c
    src/uac1_hid/uac1_hid_descriptors.c
    src/i2s_audio.c
)
pico_generate_pio_header(vibecoding_mate_uac1_mic_shift8
    ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
)
target_include_directories(vibecoding_mate_uac1_mic_shift8 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/uac1_hid
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${PICO_SDK_PATH}/src/common/pico_usb_reset_interface/include
)
target_compile_definitions(vibecoding_mate_uac1_mic_shift8 PRIVATE
    VIBECODING_UAC1_MIC_TEST=1 VIBECODING_I2S_SHIFT8=1
)
target_link_libraries(vibecoding_mate_uac1_mic_shift8
    pico_stdlib tinyusb_device tinyusb_board hardware_pio hardware_dma pico_usb_reset_interface
)
pico_add_extra_outputs(vibecoding_mate_uac1_mic_shift8)

function(add_i2s_diagnostic_target target gain_shift)
    add_executable(${target}
        src/uac1_hid/uac1_hid_main.c
        src/uac1_hid/uac1_hid_descriptors.c
        src/i2s_audio.c
    )
    pico_generate_pio_header(${target}
        ${CMAKE_CURRENT_LIST_DIR}/src/i2s_rx.pio
    )
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/uac1_hid
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${PICO_SDK_PATH}/src/common/pico_usb_reset_interface/include
    )
    target_compile_definitions(${target} PRIVATE
        VIBECODING_UAC1_MIC_TEST=1
        VIBECODING_AUDIO_DIAGNOSTIC=1
        VIBECODING_AUDIO_GAIN_SHIFT=${gain_shift}
    )
    target_link_libraries(${target}
        pico_stdlib tinyusb_device tinyusb_board hardware_pio hardware_dma pico_usb_reset_interface
    )
    pico_add_extra_outputs(${target})
endfunction()

add_i2s_diagnostic_target(vibecoding_mate_uac1_mic_raw 0)
add_i2s_diagnostic_target(vibecoding_mate_uac1_mic_gain6 1)
add_i2s_diagnostic_target(vibecoding_mate_uac1_mic_gain12 2)
