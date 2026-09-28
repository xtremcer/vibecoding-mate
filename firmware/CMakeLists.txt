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
)

target_include_directories(vibecoding_mate_audio_hid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/audio_hid_prod
)

target_link_libraries(vibecoding_mate_audio_hid
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

pico_add_extra_outputs(vibecoding_mate_audio_hid)
