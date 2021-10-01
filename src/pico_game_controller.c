/*
 * Pico Game Controller
 * @author SpeedyPotato
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "encoders.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "ws2812.pio.h"

#define SW_GPIO_SIZE 11               // Number of switches
#define LED_GPIO_SIZE 10              // Number of switches
#define ENC_GPIO_SIZE 2               // Number of encoders
#define ENC_PPR 600                   // Encoder PPR
#define ENC_DEBOUNCE true             // Encoder Debouncing
#define ENC_PULSE (ENC_PPR * 4)       // 4 pulses per PPR
#define ENC_ROLLOVER (ENC_PULSE * 2)  // Delta Rollover threshold
#define REACTIVE_TIMEOUT_MAX 500000  // Cycles before HID falls back to reactive
#define WS2812B_LED_SIZE 10          // Number of WS2812B LEDs
#define WS2812B_LED_ZONES 2          // Number of WS2812B LED Zones
#define WS2812B_LEDS_PER_ZONE \
  WS2812B_LED_SIZE / WS2812B_LED_ZONES  // Number of LEDs per zone

// MODIFY KEYBINDS HERE, MAKE SURE LENGTHS MATCH SW_GPIO_SIZE
const uint8_t SW_KEYCODE[] = {HID_KEY_D, HID_KEY_F, HID_KEY_J, HID_KEY_K,
                              HID_KEY_C, HID_KEY_M, HID_KEY_A, HID_KEY_B,
                              HID_KEY_1, HID_KEY_E, HID_KEY_G};
const uint8_t SW_GPIO[] = {
    4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 27,
};
const uint8_t LED_GPIO[] = {
    5, 7, 9, 11, 13, 15, 17, 19, 21, 26,
};
const uint8_t ENC_GPIO[] = {0, 2};      // L_ENC(0, 1); R_ENC(2, 3)
const bool ENC_REV[] = {false, false};  // Reverse Encoders
const uint8_t WS2812B_GPIO = 28;

PIO pio, pio_1;
uint32_t enc_val[ENC_GPIO_SIZE];
uint32_t prev_enc_val[ENC_GPIO_SIZE];
uint16_t cur_enc_val[ENC_GPIO_SIZE];
bool enc_changed;

bool sw_val[SW_GPIO_SIZE];
bool prev_sw_val[SW_GPIO_SIZE];
bool sw_changed;

bool kbm_report_mode;

bool leds_changed;
unsigned long reactive_timeout_count = REACTIVE_TIMEOUT_MAX;

void (*loop_mode)();

typedef struct {
  uint8_t r, g, b;
} RGB_t;

union {
  struct {
    uint8_t buttons[LED_GPIO_SIZE];
    RGB_t rgb[WS2812B_LED_ZONES];
  } lights;
  uint8_t raw[LED_GPIO_SIZE + WS2812B_LED_ZONES * 3];
} lights_report;

/**
 * WS2812B RGB Assignment
 * @param pixel_grb The pixel color to set
 **/
static inline void put_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(pio1, ENC_GPIO_SIZE, pixel_grb << 8u);
}

/**
 * WS2812B RGB Format Helper
 **/
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

/**
 * 768 Color Wheel Picker
 * @param wheel_pos Color value, r->g->b->r...
 **/
uint32_t color_wheel(uint16_t wheel_pos) {
  wheel_pos %= 768;
  if (wheel_pos < 256) {
    return urgb_u32(wheel_pos, 255 - wheel_pos, 0);
  } else if (wheel_pos < 512) {
    wheel_pos -= 256;
    return urgb_u32(255 - wheel_pos, 0, wheel_pos);
  } else {
    wheel_pos -= 512;
    return urgb_u32(0, wheel_pos, 255 - wheel_pos);
  }
}

/**
 * Color cycle effect
 **/
void ws2812b_color_cycle(uint32_t counter) {
  for (int i = 0; i < WS2812B_LED_SIZE; ++i) {
    put_pixel(color_wheel((counter + i * (int)(768 / WS2812B_LED_SIZE)) % 768));
  }
}

/**
 * WS2812B Lighting
 * @param counter Current number of WS2812B cycles
 **/
void ws2812b_update(uint32_t counter) {
  if (reactive_timeout_count >= REACTIVE_TIMEOUT_MAX) {
    ws2812b_color_cycle(counter);
  } else {
    for (int i = 0; i < WS2812B_LED_ZONES; i++) {
      for (int j = 0; j < WS2812B_LEDS_PER_ZONE; j++) {
        put_pixel(urgb_u32(lights_report.lights.rgb[i].r,
                           lights_report.lights.rgb[i].g,
                           lights_report.lights.rgb[i].b));
      }
    }
  }
}

/**
 * HID/Reactive Lights
 **/
void update_lights() {
  if (reactive_timeout_count < REACTIVE_TIMEOUT_MAX) {
    reactive_timeout_count++;
  }
  if (leds_changed) {
    for (int i = 0; i < LED_GPIO_SIZE; i++) {
      if (reactive_timeout_count >= REACTIVE_TIMEOUT_MAX) {
        if (sw_val[i]) {
          gpio_put(LED_GPIO[i], 1);
        } else {
          gpio_put(LED_GPIO[i], 0);
        }
      } else {
        if (lights_report.lights.buttons[i] == 0) {
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
  uint16_t joy0;
  uint16_t joy1;
} report;

/**
 * Gamepad Mode
 **/
void joy_mode() {
  if (tud_hid_ready()) {
    uint16_t translate_buttons = 0;
    for (int i = SW_GPIO_SIZE - 1; i >= 0; i--) {
      translate_buttons = (translate_buttons << 1) | (sw_val[i] ? 1 : 0);
      prev_sw_val[i] = sw_val[i];
    }
    report.buttons = translate_buttons;
    sw_changed = false;
    
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
      //Joystick takes aboslute unsigned values
      //1 full turn of knob == ENC_PULSE
      //Force an overflow first, then overflow down to 16bit
      cur_enc_val[i] = enc_val[i] + ENC_PULSE;
      //Modulo to range
      cur_enc_val[i] %= ENC_PULSE;
      //Check reverse
      if (!ENC_REV[i]) cur_enc_val[i] = ENC_PULSE - cur_enc_val[i];

      prev_enc_val[i] = enc_val[i];
    }

    report.joy0 = cur_enc_val[0];
    report.joy1 = cur_enc_val[1];
    enc_changed = false;
    
    tud_hid_n_report(0x00, REPORT_ID_JOYSTICK, &report, sizeof(report));
  }
}

/**
 * Keyboard Mode
 **/
void key_mode() {
  // Wait for ready, reporting mouse too fast hampers movement
  if (tud_hid_ready()) {    
    // Alternate report to send
    if (kbm_report_mode) {
      /*------------- Keyboard -------------*/
      uint8_t nkro_report[32] = {0};
      for (int i = 0; i < SW_GPIO_SIZE; i++) {
        if (sw_val[i]) {
          uint8_t bit = SW_KEYCODE[i] % 8;
          uint8_t byte = (SW_KEYCODE[i] / 8) + 1;
          if (SW_KEYCODE[i] >= 240 && SW_KEYCODE[i] <= 247) {
            nkro_report[0] |= (1 << bit);
          } else if (byte > 0 && byte <= 31) {
            nkro_report[byte] |= (1 << bit);
          }

          prev_sw_val[i] = sw_val[i];
        }
      }

      tud_hid_n_report(0x00, REPORT_ID_KEYBOARD, &nkro_report, sizeof(nkro_report));
      sw_changed = false;
    } else {
      /*------------- Mouse -------------*/
      // find the delta between previous and current enc_val
      int delta[ENC_GPIO_SIZE] = {0};
      for (int i = 0; i < ENC_GPIO_SIZE; i++) {
        delta[i] = (enc_val[i] - prev_enc_val[i]) * (ENC_REV[i] ? 1 : -1);
        prev_enc_val[i] = enc_val[i];
      }

      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta[0], delta[1], 0, 0);
      enc_changed = false;
    }

    kbm_report_mode = !kbm_report_mode;
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
  // Update LEDs if input changed while in reactive mode
  if (sw_changed && reactive_timeout_count >= REACTIVE_TIMEOUT_MAX)
    leds_changed = true;
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
    encoders_program_init(pio, i, offset, ENC_GPIO[i], ENC_DEBOUNCE);

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

  // Set up WS2812B
  pio_1 = pio1;
  uint offset2 = pio_add_program(pio_1, &ws2812_program);
  ws2812_program_init(pio_1, ENC_GPIO_SIZE, offset2, WS2812B_GPIO, 800000,
                      false);

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
  for (int i = 0; i < LED_GPIO_SIZE; i++) {
    gpio_init(LED_GPIO[i]);
    gpio_set_dir(LED_GPIO[i], GPIO_OUT);
  }

  // Set listener bools
  enc_changed = false;
  sw_changed = false;
  leds_changed = false;
  kbm_report_mode = false;

  // Joy/KB Mode Switching
  if (gpio_get(SW_GPIO[0])) {
    loop_mode = &joy_mode;
  } else {
    loop_mode = &key_mode;
  }
}

/**
 * Second Core Runnable
 **/
void core1_entry() {
  uint32_t counter = 0;
  while (1) {
    ws2812b_update(++counter);
    sleep_ms(5);
  }
}

/**
 * Main Loop Function
 **/
int main(void) {
  board_init();
  tusb_init();
  init();

  multicore_launch_core1(core1_entry);

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
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
  // TODO not Implemented
  (void)itf;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
  (void)itf;
  if (report_id == 2 && report_type == HID_REPORT_TYPE_OUTPUT &&
      buffer[0] == 2 && bufsize >= sizeof(lights_report))  // light data
  {
    size_t i = 0;
    for (i; i < sizeof(lights_report); i++) {
      lights_report.raw[i] = buffer[i + 1];
    }
    reactive_timeout_count = 0;
    leds_changed = true;
  }
}
