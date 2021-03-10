/* 
 * Pico SDVX
 * @author SpeedyPotato
 * 
 * Based off dev_hid_composite and mdxtinkernick/pico_encoders
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "usb_descriptors.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "encoders.pio.h"

const uint8_t SW_KEYCODE[] = {HID_KEY_RETURN, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_Z, HID_KEY_X};    // MODIFY KEYBINDS HERE
const uint8_t SW_GPIO[] = {4, 5, 6, 7, 8, 9, 10};                   // MAKE SURE SW_KEYCODE and SW_GPIO LENGTHS MATCH
const uint8_t LED_GPIO[] = {28, 27, 26, 22, 21, 20, 19};            // MAKE SURE SW_GPIO and LED_GPIO LENGTHS MATCH
const uint8_t ENC_GPIO[] = {0, 2};

const size_t SW_GPIO_SIZE = sizeof(SW_GPIO)/ sizeof(SW_GPIO[0]);    // Used for SW_KEYCODE, SW_GPIO, LED_GPIO
const size_t ENC_GPIO_SIZE = sizeof(ENC_GPIO)/ sizeof(ENC_GPIO[0]);
const uint8_t ENC_SENS = 1;        // Encoder sensitivity multiplier

PIO pio;
uint32_t capture_buf[2] = {0, 0};
uint32_t prev_capture_buf[2] = {0, 0};

/**
 * DMA Encoder Logic
 **/
void dma_handler() {
    uint i = 1;
    int interrupt_channel = 0; 
        while ((i & dma_hw->ints0) == 0) {  
            i = i << 1; 
            ++interrupt_channel; 
            }  
    dma_hw->ints0 = 1u << interrupt_channel;
    if (interrupt_channel < 4){
        dma_channel_set_read_addr(interrupt_channel, &pio->rxf[interrupt_channel], true);
    }
}

/**
 * Initialize board pins
 **/
void init() {
    // LED Pin on when connected
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    // Set up the state machine for encoders
    pio = pio0;
    uint offset = pio_add_program(pio, &encoders_program);
    // Setup Encoders
    for (int i = 0; i<2; i++){
        encoders_program_init(pio, i, offset, ENC_GPIO[i]);
        
        dma_channel_config c = dma_channel_get_default_config(i);
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, false);
        channel_config_set_dreq(&c, pio_get_dreq(pio, i, false));
        
        dma_channel_configure(i, &c,
            &capture_buf[i],        // Destinatinon pointer
            &pio->rxf[i],      // Source pointer
            0x10, // Number of transfers
            true                // Start immediately
        );
        irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
        irq_set_enabled(DMA_IRQ_0, true);
        dma_channel_set_irq0_enabled(i, true);
    }

    // Setup Button GPIO
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
        gpio_init(SW_GPIO[i]);
        gpio_set_function(SW_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(SW_GPIO[i], GPIO_IN);
        gpio_pull_up(SW_GPIO[i]);
    }

    // Setup LED GPIO
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
        gpio_init(LED_GPIO[i]);
        gpio_set_dir(LED_GPIO[i], GPIO_OUT);
    }
}

/**
 * Keyboard Mode
 **/
void key_mode() {
    if (tud_hid_ready()) {
        /*------------- Keyboard -------------*/
        bool is_pressed = false;
        int keycode_idx = 0;
        uint8_t keycode[6] = {0};   //looks like we are limited to 6kro?
        for (int i = 0; i < SW_GPIO_SIZE; i++) {
            if (!gpio_get(SW_GPIO[i])) {
                // use to avoid send multiple consecutive zero report for keyboard
                keycode[keycode_idx] = SW_KEYCODE[i];
                keycode_idx = ++keycode_idx % SW_GPIO_SIZE;
                is_pressed = true;

                // Reactive Lighting On
                gpio_put(LED_GPIO[i], 1);
            } else {
                // Reactive Lighting Off
                gpio_put(LED_GPIO[i], 0);
            }
        }
        if (is_pressed) {
            // send key report
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
        } else {
            // send empty key report if previously has key pressed
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        }

        /*------------- Mouse -------------*/
        // delay if needed before attempt to send mouse report
        bool encoder_state_changed = false;
        for (int i = 0; i < ENC_GPIO_SIZE; i++) {
            if (prev_capture_buf[i] != capture_buf[i]);
                encoder_state_changed = true;
        }
        if (encoder_state_changed) {
            while (!tud_hid_ready()) {
                board_delay(1);
            }
            tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, (capture_buf[0] - prev_capture_buf[0]) * ENC_SENS,
                                                        (capture_buf[1] - prev_capture_buf[1]) * ENC_SENS, 0, 0);
            for (int i = 0; i < ENC_GPIO_SIZE; i++) {
                prev_capture_buf[i] = capture_buf[i];
            }
        }
    }
}

/**
 * Main Loop Function
 **/
int main(void) {
    board_init();
    tusb_init();
    init();

    while (1) {
        tud_task(); // tinyusb device task
        key_mode();
    }

    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    // TODO not Implemented
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    // TODO set LED based on CAPLOCK, NUMLOCK etc...
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}
