/*
 * Pico SDVX
 * @author SpeedyPotato
 *
 * Based off dev_hid_composite, mdxtinkernick/pico_encoders, and
 * Drewol/rp2040-gamecon
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "encoders.pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define SW_GPIO_SIZE 11               // Number of switches
#define ENC_GPIO_SIZE 2               // Number of encoders
#define ENC_PPR 600                   // Encoder PPR
#define ENC_PULSE (ENC_PPR * 4)       // 4 pulses per PPR
#define ENC_ROLLOVER (ENC_PULSE * 2)  // Delta Rollover threshold
#define REACTIVE_TIMEOUT_MAX 1000  // Cycles before HID falls back to reactive

// MODIFY KEYBINDS HERE, MAKE SURE LENGTHS MATCH SW_GPIO_SIZE
const uint8_t SW_KEYCODE[] = {HID_KEY_D, HID_KEY_F, HID_KEY_J, HID_KEY_K,
                              HID_KEY_C, HID_KEY_M, HID_KEY_A, HID_KEY_B,
                              HID_KEY_1, HID_KEY_C, HID_KEY_D};
const uint8_t SW_GPIO[] = {
    4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 27,
};
const uint8_t LED_GPIO[] = {
    5, 7, 9, 11, 13, 15, 17, 19, 21, 26, 28,
};
const uint8_t ENC_GPIO[] = {0, 2};  // L_ENC(0, 1); R_ENC(2, 3)

PIO pio;
uint32_t enc_val[ENC_GPIO_SIZE];
uint32_t prev_enc_val[ENC_GPIO_SIZE];
int cur_enc_val[ENC_GPIO_SIZE];
bool enc_changed;

bool sw_val[SW_GPIO_SIZE];
bool prev_sw_val[SW_GPIO_SIZE];
bool sw_changed;

bool leds_changed;
unsigned long reactive_timeout_count = REACTIVE_TIMEOUT_MAX;

void (*loop_mode)();

struct lights_report {
  uint8_t buttons[11];
} lights_report;

/**
 * HID/Reactive Lights
 **/
void update_lights() {
  if (leds_changed) {
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
      if (reactive_timeout_count >= REACTIVE_TIMEOUT_MAX) {
        if (sw_val[i]) {
          gpio_put(LED_GPIO[i], 1);
        } else {
          gpio_put(LED_GPIO[i], 0);
        }
      } else {
        if (lights_report.buttons[i] == 0) {
          gpio_put(LED_GPIO[i], 0);
        } else {
          gpio_put(LED_GPIO[i], 1);
        }
      }
    }
    leds_changed = false;
  }
}

struct report {
  uint16_t buttons;
  uint8_t joy0;
  uint8_t joy1;
} report;

/**
 * Gamepad Mode
 **/
void joy_mode() {
  if (tud_hid_ready()) {
    bool send_report = false;
    if (sw_changed) {
      send_report = true;
      uint16_t translate_buttons = 0;
      for (int i = SW_GPIO_SIZE - 1; i >= 0; i--) {
        translate_buttons = (translate_buttons << 1) | (sw_val[i] ? 1 : 0);
        prev_sw_val[i] = sw_val[i];
      }
      report.buttons = translate_buttons;
      sw_changed = false;
    }
    if (enc_changed) {
      send_report = true;

      // find the delta between previous and current enc_val
      for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        int delta;
        int changeType;                      // -1 for negative 1 for positive
        if (enc_val[i] > prev_enc_val[i]) {  // if the new value is bigger its
                                             // a positive change
          delta = enc_val[i] - prev_enc_val[i];
          changeType = 1;
        } else {  // otherwise its a negative change
          delta = prev_enc_val[i] - enc_val[i];
          changeType = -1;
        }
        // Overflow / Underflow
        if (delta > ENC_ROLLOVER) {
          // Reverse the change type due to overflow / underflow
          changeType *= -1;
          delta = UINT32_MAX - delta + 1;  // this should give us how much we
                                           // overflowed / underflowed by
        }

        cur_enc_val[i] = cur_enc_val[i] + (delta * changeType);
        while (cur_enc_val[i] < 0) {
          cur_enc_val[i] = ENC_PULSE - cur_enc_val[i];
        }

        prev_enc_val[i] = enc_val[i];
      }

      report.joy0 = ((double)cur_enc_val[0] / ENC_PULSE) * 256;
      report.joy1 = ((double)cur_enc_val[1] / ENC_PULSE) * 256;
      enc_changed = false;
    }

    if (send_report) {
      tud_hid_n_report(0x00, REPORT_ID_GAMEPAD, &report, sizeof(report));
    }
  }
}

/**
 * Keyboard Mode
 **/
void key_mode() {
  if (tud_hid_ready()) {
    /*------------- Keyboard -------------*/
    if (sw_changed) {
      bool is_pressed = false;
      int keycode_idx = 0;
      uint8_t keycode[6] = {0};  // looks like we are limited to 6kro?
      for (int i = 0; i < SW_GPIO_SIZE; i++) {
        if (sw_val[i]) {
          // use to avoid send multiple consecutive zero report for keyboard
          keycode[keycode_idx] = SW_KEYCODE[i];
          keycode_idx = ++keycode_idx % SW_GPIO_SIZE;
          is_pressed = true;

          prev_sw_val[i] = sw_val[i];
          // Reactive Lighting On
          gpio_put(LED_GPIO[i], 1);
        } else {
          // Reactive Lighting Off
          gpio_put(LED_GPIO[i], 0);
        }
      }
      if (is_pressed) {
        // Send key report
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
      } else {
        // Send empty key report if previously has key pressed
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
      }
      sw_changed = false;
    }

    /*------------- Mouse -------------*/
    if (enc_changed) {
      // Delay if needed before attempt to send mouse report
      while (!tud_hid_ready()) {
        board_delay(1);
      }
      // find the delta between previous and current enc_val
      int delta[ENC_GPIO_SIZE] = {0};
      for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        int changeType;                      // -1 for negative 1 for positive
        if (enc_val[i] > prev_enc_val[i]) {  // if the new value is bigger its
                                             // a positive change
          delta[i] = enc_val[i] - prev_enc_val[i];
          changeType = 1;
        } else {  // otherwise its a negative change
          delta[i] = prev_enc_val[i] - enc_val[i];
          changeType = -1;
        }
        // Overflow / Underflow
        if (delta[i] > ENC_ROLLOVER) {
          // Reverse the change type due to overflow / underflow
          changeType *= -1;
          delta[i] =
              UINT32_MAX - delta[i] + 1;  // this should give us how much we
                                          // overflowed / underflowed by
        }
        delta[i] *= changeType;  // set direction
        prev_enc_val[i] = enc_val[i];
      }
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta[0], delta[1], 0, 0);
      enc_changed = false;
    }
  }
}

/**
 * Update Input States
 **/
void update_inputs() {
  // Encoder Flag
  for (int i = 0; i < ENC_GPIO_SIZE; i++) {
    if (enc_val[i] != prev_enc_val[i]) {
      enc_changed = true;
      break;
    }
  }
  // Switch Update & Flag
  for (int i = 0; i < SW_GPIO_SIZE; i++) {
    if (gpio_get(SW_GPIO[i])) {
      sw_val[i] = false;
    } else {
      sw_val[i] = true;
    }
    if (!sw_changed && sw_val[i] != prev_sw_val[i]) {
      sw_changed = true;
    }
  }
  // Update LEDs if in reactive mode
  if (reactive_timeout_count >= REACTIVE_TIMEOUT_MAX) leds_changed = true;
}

/**
 * DMA Encoder Logic For 2 Encoders
 **/
void dma_handler() {
  uint i = 1;
  int interrupt_channel = 0;
  while ((i & dma_hw->ints0) == 0) {
    i = i << 1;
    ++interrupt_channel;
  }
  dma_hw->ints0 = 1u << interrupt_channel;
  if (interrupt_channel < 4) {
    dma_channel_set_read_addr(interrupt_channel, &pio->rxf[interrupt_channel],
                              true);
  }
}

/**
 * Initialize Board Pins
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
  for (int i = 0; i < ENC_GPIO_SIZE; i++) {
    enc_val[i] = 0;
    prev_enc_val[i] = 0;
    cur_enc_val[i] = 0;
    encoders_program_init(pio, i, offset, ENC_GPIO[i]);

    dma_channel_config c = dma_channel_get_default_config(i);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, i, false));

    dma_channel_configure(i, &c,
                          &enc_val[i],   // Destinatinon pointer
                          &pio->rxf[i],  // Source pointer
                          0x10,          // Number of transfers
                          true           // Start immediately
    );
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_set_irq0_enabled(i, true);
  }

  // Setup Button GPIO
  for (int i = 0; i < SW_GPIO_SIZE; i++) {
    sw_val[i] = false;
    prev_sw_val[i] = false;
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

  // Set listener bools
  enc_changed = false;
  sw_changed = false;
  leds_changed = false;

  // Joy/KB Mode Switching
  if (gpio_get(SW_GPIO[0])) {
    loop_mode = &joy_mode;
  } else {
    loop_mode = &key_mode;
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
    tud_task();  // tinyusb device task
    update_inputs();
    loop_mode();
    update_lights();
  }

  return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
  // TODO not Implemented
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
  if (report_id == 2 && report_type == HID_REPORT_TYPE_OUTPUT &&
      buffer[0] == 2 && bufsize >= sizeof(lights_report))  // light data
  {
    size_t i = 0;
    for (i; i < sizeof(lights_report); i++) {
      lights_report.buttons[i] = buffer[i + 1];
    }
    reactive_timeout_count = 0;
    leds_changed = true;
  }
}
