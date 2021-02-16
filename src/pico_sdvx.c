/* 
 * Pico SDVX
 * @author SpeedyPotato
 * 
 * Based off dev_hid_composite and PaulStoffregen/Encoder
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "usb_descriptors.h"

const uint8_t ENC_SENS = 10;        // Encoder sensitivity multiplier
const uint8_t L_ENC_GPIO[] = {0, 1};
const uint8_t R_ENC_GPIO[] = {2, 3};
const uint8_t SW_KEYCODE[] = {HID_KEY_RETURN, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_Z, HID_KEY_X};    // MODIFY KEYBINDS HERE
const uint8_t SW_GPIO[] = {4, 5, 6, 7, 8, 9, 10};                               // MAKE SURE SW_KEYCODE and SW_GPIO LENGTHS MATCH
const uint8_t LED_GPIO[] = {28, 27, 26, 22, 21, 20, 19};                        // MAKE SURE SW_GPIO and LED_GPIO LENGTHS MATCH
const size_t ENC_GPIO_SIZE = sizeof(L_ENC_GPIO)/ sizeof(L_ENC_GPIO[0]);
const size_t SW_GPIO_SIZE = sizeof(SW_GPIO)/ sizeof(SW_GPIO[0]);
const size_t LED_GPIO_SIZE = sizeof(LED_GPIO)/ sizeof(LED_GPIO[0]);

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

int l_enc=0, r_enc=0, l_state, r_state;

/**
 * Encoder Callback
 **/
void gpio_enc_callback(uint gpio, uint32_t events) {
    if (gpio == L_ENC_GPIO[0] || gpio == L_ENC_GPIO[1]) {
        uint8_t s = l_state & 3;
        if (gpio_get(L_ENC_GPIO[0])) s |= 4;
        if (gpio_get(L_ENC_GPIO[1])) s |= 8;
        switch (s) {
            case 0: case 5: case 10: case 15:
                break;
            case 1: case 7: case 8: case 14:
                l_enc++; break;
            case 2: case 4: case 11: case 13:
                l_enc--; break;
            case 3: case 12:
                l_enc += 2; break;
            default:
                l_enc -= 2; break;
        }
        l_state = (s >> 2);
    } else {
        uint8_t s = r_state & 3;
        if (gpio_get(R_ENC_GPIO[0])) s |= 4;
        if (gpio_get(R_ENC_GPIO[1])) s |= 8;
        switch (s) {
            case 0: case 5: case 10: case 15:
                break;
            case 1: case 7: case 8: case 14:
                r_enc++; break;
            case 2: case 4: case 11: case 13:
                r_enc--; break;
            case 3: case 12:
                r_enc += 2; break;
            default:
                r_enc -= 2; break;
        }
        r_state = (s >> 2);
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

    // Setup L Encoder GPIO
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        gpio_init(L_ENC_GPIO[i]);
        gpio_set_function(L_ENC_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(L_ENC_GPIO[i], GPIO_IN);
        gpio_pull_up(L_ENC_GPIO[i]);
        gpio_set_irq_enabled_with_callback(L_ENC_GPIO[i], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_enc_callback);
    }
    l_state = gpio_get(L_ENC_GPIO[0]);

    // Setup R Encoder GPIO
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        gpio_init(R_ENC_GPIO[i]);
        gpio_set_function(R_ENC_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(R_ENC_GPIO[i], GPIO_IN);
        gpio_pull_up(R_ENC_GPIO[i]);
        gpio_set_irq_enabled_with_callback(R_ENC_GPIO[i], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_enc_callback);
    }
    r_state = gpio_get(R_ENC_GPIO[0]);

    // Setup Button GPIO
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
        gpio_init(SW_GPIO[i]);
        gpio_set_function(SW_GPIO[i], GPIO_FUNC_SIO);
        gpio_set_dir(SW_GPIO[i], GPIO_IN);
        gpio_pull_up(SW_GPIO[i]);
    }

    // Setup LED GPIO
    for (int i = 0; i < LED_GPIO_SIZE; i++) {
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
        bool isPressed = false;
        for (int i = 0; i < SW_GPIO_SIZE; i++) {
            if (!gpio_get(SW_GPIO[i])) {
                // use to avoid send multiple consecutive zero report for keyboard
                uint8_t keycode[6] = {0};
                keycode[0] = SW_KEYCODE[i];
                tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
                isPressed = true;

                // Reactive Lighting On
                gpio_put(LED_GPIO[i], 1);
            } else {
                // Reactive Lighting Off
                gpio_put(LED_GPIO[i], 0);
            }
        }
        // send empty key report if previously has key pressed
        if (!isPressed) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);

        // delay a bit before attempt to send keyboard report
        board_delay(10);

        /*------------- Mouse -------------*/
        tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, l_enc * ENC_SENS, r_enc * ENC_SENS, 0, 0);
        l_enc = 0;
        r_enc = 0;
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
