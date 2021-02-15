/* 
 * Pico SDVX
 * dev_hid_composite.c
 * @Author SpeedyPotato
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "usb_descriptors.h"

const uint8_t lEncGPIO[] = {0, 1};
const uint8_t rEncGPIO[] = {2, 3};
const uint8_t swKeycode[] = {HID_KEY_RETURN, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_Z, HID_KEY_X};
const uint8_t swGPIO[] = {4, 5, 6, 7, 8, 9, 10};
const uint8_t ledGPIO[] = {28, 27, 26, 22, 21, 20, 19};
const size_t swGPIOsize = sizeof(swGPIO)/ sizeof(swGPIO[0]);
const size_t ledGPIOsize = sizeof(ledGPIO)/ sizeof(ledGPIO[0]);

/**
 * Pin Init
 **/
void init() {
    // Setup Button Inputs
    for (int i = 0; i < swGPIOsize; i++) {
        gpio_init(swGPIO[i]);
        gpio_set_function(swGPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(swGPIO[i], GPIO_IN);
        gpio_pull_up(swGPIO[i]);
    }
}

/**
 * Keyboard Mode
 **/
void keyMode() {
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    // uint32_t const btn = 1;

    // Remote wakeup
    // if (tud_suspended() && btn) {
    //     // Wake up host if we are in suspend mode
    //     // and REMOTE_WAKEUP feature is enabled by host
    //     tud_remote_wakeup();
    // }

    /*------------- Mouse -------------*/
    // if (tud_hid_ready()) {
    //     if (!btn) {
    //         int8_t const delta = 5;

    //         // no button, right + down, no scroll pan
    //         tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta, delta, 0, 0);

    //         // delay a bit before attempt to send keyboard report
    //         board_delay(10);
    //     }
    // }

    if (tud_hid_ready()) {
        /*------------- Keyboard -------------*/
        bool isPressed = false;
        for (int i = 0; i < swGPIOsize; i++) {
            if (!gpio_get(swGPIO[i])) {
                // use to avoid send multiple consecutive zero report for keyboard
                uint8_t keycode[6] = {0};
                keycode[0] = swKeycode[i];

                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);

                isPressed = true;
            }
        }
        // send empty key report if previously has key pressed
        if (!isPressed) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    }
}

/**
 * Main Loop
 **/
int main(void) {
    board_init();
    tusb_init();
    init();

    while (1) {
        tud_task(); // tinyusb device task
        keyMode();
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
