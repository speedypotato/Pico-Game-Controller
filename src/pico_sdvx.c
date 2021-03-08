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
#include "encoders.pio.h"

const uint8_t SW_KEYCODE[] = {HID_KEY_RETURN, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_Z, HID_KEY_X};    // MODIFY KEYBINDS HERE
const uint8_t SW_GPIO[] = {4, 5, 6, 7, 8, 9, 10};                               // MAKE SURE SW_KEYCODE and SW_GPIO LENGTHS MATCH
const uint8_t LED_GPIO[] = {28, 27, 26, 22, 21, 20, 19};                        // MAKE SURE SW_GPIO and LED_GPIO LENGTHS MATCH
const uint8_t ENC_GPIO[] = {0, 2};       // ENC_L(0, 1) ENC_R(2,3)

const uint8_t ENC_SENS = 10;        // Encoder sensitivity multiplier
const size_t ENC_GPIO_SIZE = sizeof(ENC_GPIO) / sizeof(ENC_GPIO[0]);    // Encoder always uses 2GPIO
const size_t SW_GPIO_SIZE = sizeof(SW_GPIO) / sizeof(SW_GPIO[0]);       // Used for SW_KEYCODE, SW_GPIO, LED_GPIO

PIO pio;
uint32_t enc_val[] = {0, 0};    // Must match number of encoders
uint state_machine[] = {0, 1};    // Must match number of encoders
uint offset;

/**
 * Initialize board pins
 **/
void init() {
    // LED Pin on when connected
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    // Setup Encoder PIO
    pio = pio0;
    offset = pio_add_program(pio, &encoders_program);

    // Setup Encoder GPIO
    for (uint i = 0; i < ENC_GPIO_SIZE; i++) {
        encoders_program_init(pio, state_machine[i], offset, ENC_GPIO[i]);
        enc_val[i] = pio_sm_get(pio, i);
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
        bool changed_state = pio_sm_get_rx_fifo_level(pio, state_machine[0]) > 0 ||
                            pio_sm_get_rx_fifo_level(pio, state_machine[1]) > 0;
        if (changed_state) {
            // delay if needed before attempt to send mouse report
            while (!tud_hid_ready()) {
                board_delay(1);
            }
            // 0-4294967295
            tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, (pio_sm_get(pio, 0) - enc_val[0]) * ENC_SENS,
                                    (pio_sm_get(pio, 1) - enc_val[1]) * ENC_SENS, 0, 0);                    
            for (uint i = 0; i < ENC_GPIO_SIZE; i++) {
                enc_val[i] = pio_sm_get(pio, i);
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
