/* 
 * Pico SDVX
 * @author SpeedyPotato
 * 
 * Based off dev_hid_composite
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "usb_descriptors.h"

const uint8_t ENC_SENS = 30;        // Encoder sensitivity multiplier
const uint8_t L_ENC_GPIO[] = {0, 1};
const uint8_t R_ENC_GPIO[] = {2, 3};
const uint8_t SW_KEYCODE[] = {HID_KEY_RETURN, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_Z, HID_KEY_X};    // MODIFY KEYBINDS HERE
const uint8_t SW_GPIO[] = {4, 5, 6, 7, 8, 9, 10};                                                                   // MAKE SURE SW_KEYCODE and SW_GPIO LENGTHS MATCH
const uint8_t LED_GPIO[] = {28, 27, 26, 22, 21, 20, 19};
const size_t ENC_GPIO_SIZE = sizeof(L_ENC_GPIO)/ sizeof(L_ENC_GPIO[0]);
const size_t SW_GPIO_SIZE = sizeof(SW_GPIO)/ sizeof(SW_GPIO[0]);
const size_t LED_GPIO_SIZE = sizeof(LED_GPIO)/ sizeof(LED_GPIO[0]);

int l_enc=0, r_enc=0;
int l_state, l_prev_state, r_state, r_prev_state;

/**
 * Initialize board pins
 **/
void init() {
    // Setup L Encoder GPIO
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        gpio_init(L_ENC_GPIO[i]);
        gpio_set_function(L_ENC_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(L_ENC_GPIO[i], GPIO_IN);
        gpio_pull_up(L_ENC_GPIO[i]);
    }
    l_prev_state = gpio_get(L_ENC_GPIO[0]);

    // Setup R Encoder GPIO
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        gpio_init(R_ENC_GPIO[i]);
        gpio_set_function(R_ENC_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(R_ENC_GPIO[i], GPIO_IN);
        gpio_pull_up(R_ENC_GPIO[i]);
    }
    r_prev_state = gpio_get(R_ENC_GPIO[0]);

    // Setup Button GPIO
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
        gpio_init(SW_GPIO[i]);
        gpio_set_function(SW_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(SW_GPIO[i], GPIO_IN);
        gpio_pull_up(SW_GPIO[i]);
    }
}

/**
 * Keyboard Mode
 **/
void key_mode() {
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    // Remote wakeup
    // if (tud_suspended() && btn) {
    //     // Wake up host if we are in suspend mode
    //     // and REMOTE_WAKEUP feature is enabled by host
    //     tud_remote_wakeup();
    // }

    if (tud_hid_ready()) {
        /*------------- Keyboard -------------*/
        bool isPressed = false;
        for (int i = 0; i < SW_GPIO_SIZE; i++) {
            if (!gpio_get(SW_GPIO[i])) {
                // use to avoid send multiple consecutive zero report for keyboard
                uint8_t keycode[6] = {0};
                keycode[0] = SW_KEYCODE[i];

                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);

                isPressed = true;
            }
        }
        // send empty key report if previously has key pressed
        if (!isPressed) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        
        // delay a bit before attempt to send keyboard report
        board_delay(10);

        /*------------- Mouse -------------*/
        //todo: merge these together into less hid mouse reports, move to interrupts
        l_state = gpio_get(L_ENC_GPIO[0]);
        if (l_state != l_prev_state) {
            if (gpio_get(L_ENC_GPIO[1]) != l_state) {
                tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, 1 * ENC_SENS, 0, 0, 0);
            } else {
                tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, -1 * ENC_SENS, 0, 0, 0);
            }
        }

        r_state = gpio_get(R_ENC_GPIO[0]);
        if (r_state != r_prev_state) {
            if (gpio_get(R_ENC_GPIO[1]) != r_state) {
                tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, 0, 1 * ENC_SENS, 0, 0);
            } else {
                tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, 0, -1 * ENC_SENS, 0, 0);
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
