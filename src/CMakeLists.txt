add_executable(pico_sdvx
        pico_sdvx.c
        usb_descriptors.c)

target_include_directories(pico_sdvx PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(pico_sdvx ${CMAKE_CURRENT_LIST_DIR}/encoders.pio)
target_sources(pico_sdvx PRIVATE pico_sdvx.c)

target_link_libraries(pico_sdvx PRIVATE
        pico_stdlib
        tinyusb_device
        tinyusb_board
        hardware_dma
        hardware_irq)

pico_add_extra_outputs(pico_sdvx)