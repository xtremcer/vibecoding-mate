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
