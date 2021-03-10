add_executable(pico_sdvx
        pico_sdvx.c
        usb_descriptors.c
        )

target_include_directories(pico_sdvx PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico_sdvx PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(pico_sdvx)
