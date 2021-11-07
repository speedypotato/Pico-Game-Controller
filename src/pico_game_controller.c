/*
 * Pico Game Controller
 * @author SpeedyPotato
 */
#define PICO_GAME_CONTROLLER_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "controller_config.h"
#include "encoders.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "switches.pio.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "ws2812.pio.h"

PIO pio_0, pio_1;
//DMA writes to volatile, make local copy to use
volatile uint32_t enc_val[ENC_GPIO_SIZE];
uint32_t prev_enc_val[ENC_GPIO_SIZE];
int cur_enc_val[ENC_GPIO_SIZE];
int enc_dma_chan[ENC_GPIO_SIZE];

dma_channel_config sw_rx_c;
dma_channel_config sw_tx_c;
int sw_sm;
int sw_dma_chan;
uint32_t sw_bitmask;
//Must make local copy of sw_data. DMA will change value faster than CPU can process
volatile uint32_t sw_data;

uint64_t time_now;
uint64_t sw_debounce_timestamp[SW_GPIO_SIZE];
uint32_t prev_sw_data;
int delta[ENC_GPIO_SIZE];

bool kbm_report;

unsigned long reactive_timeout_count = REACTIVE_TIMEOUT_MAX;

void (*loop_mode)();
bool joy_mode_check = true;

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
  pio_sm_put_blocking(pio_1, 0, pixel_grb << 8u);
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
  //Use prev_sw_data
  for (int i = 0; i < LED_GPIO_SIZE; i++) {
    if (reactive_timeout_count >= REACTIVE_TIMEOUT_MAX) {
      gpio_put(LED_GPIO[i], (prev_sw_data >> i) & 0x1);
    } else {
      gpio_put(LED_GPIO[i], lights_report.lights.buttons[i]);
    }
  }
}

struct report {
  uint16_t buttons;
  uint8_t joy[ENC_GPIO_SIZE];
} report;

/**
 * Gamepad Mode
 **/
void joy_mode() {
  if (tud_hid_ready()) {
    //Update time
    time_now = time_us_64();

    //Cache sw_data
    int temp_data = sw_data;
    //Zero out buttons
    report.buttons = 0;

    for (int i = 0; i < SW_GPIO_SIZE; i++) {
      int bt_val = (temp_data >> i) & 0x1;
      int prev_bt_val = (prev_sw_data >> i) & 0x1;
      //If debounce and under time, use old value
      if (SW_DEBOUNCE && (time_now - sw_debounce_timestamp[i] < SW_DEBOUNCE_TIME_US)) {
        report.buttons |= prev_bt_val << (BT_MAP[i] - 1);
      } else {
        //Use new value
        report.buttons |= bt_val << (BT_MAP[i] - 1);
        //Update prev_sw_data
        prev_sw_data |= bt_val << (BT_MAP[i] - 1);
      }
    }

    //Cache enc_val
    uint32_t temp_val[ENC_GPIO_SIZE];
    for (int i = 0; i < ENC_GPIO_SIZE; i++) temp_val[i] = enc_val[i];

    // find the delta between previous and current enc_val
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
      int temp_enc_val = cur_enc_val[i];
      temp_enc_val += ((ENC_REV[i] ? 1 : -1) * (temp_enc_val - prev_enc_val[i]));
      while (temp_enc_val < 0) temp_enc_val = ENC_PULSE + temp_enc_val;
      temp_enc_val %= ENC_PULSE;

      cur_enc_val[i] = temp_enc_val;
      prev_enc_val[i] = temp_val[i];

      report.joy[i] = ((double)cur_enc_val[i] / ENC_PULSE) * (UINT8_MAX + 1);
    }

    tud_hid_n_report(0x00, REPORT_ID_JOYSTICK, &report, sizeof(report));
  }
}

/**
 * Keyboard Mode
 **/
void key_mode(uint64_t time_now) {
  if (tud_hid_ready()) {  // Wait for ready, updating mouse too fast hampers
                          // movement

    //Update time
    time_now = time_us_64();

    /*------------- Keyboard -------------*/
    uint8_t nkro_report[32] = {0};
    //Cache a local copy of sw_data, dma can change it in middle of processing
    uint16_t temp_data = sw_data;
    
    for (int i = 0; i < SW_GPIO_SIZE; i++) {
      int temp_key = temp_data >> i & 0x1;
      //If debounce
      if (SW_DEBOUNCE) {
        //Under debounce time, use old key
        if ((time_now - sw_debounce_timestamp[i]) < SW_DEBOUNCE_TIME_US) {
          temp_key = prev_sw_data >> i & 0x1;
        } else {
          //Over debounce time, update prev key
          prev_sw_data |= temp_key << i;
        }
      }

      if (temp_key) {
        uint8_t bit = SW_KEYCODE[i] % 8;
        uint8_t byte = (SW_KEYCODE[i] / 8) + 1;
        if (SW_KEYCODE[i] >= 240 && SW_KEYCODE[i] <= 247) {
          nkro_report[0] |= (1 << bit);
        } else if (byte > 0 && byte <= 31) {
          nkro_report[byte] |= (1 << bit);
        }
      }
    }

    /*------------- Mouse -------------*/
    //Cache enc_val
    uint32_t temp_val[ENC_GPIO_SIZE];
    for (int i = 0; i < ENC_GPIO_SIZE; i++) temp_val[i] = enc_val[i];
    // find the delta between previous and current enc_val
    for (int i = 0; i < ENC_GPIO_SIZE; i++) {
      delta[i] = (temp_val[i] - prev_enc_val[i]) * (ENC_REV[i] ? 1 : -1);
      prev_enc_val[i] = temp_val[i];
    }

    if (kbm_report) {
      tud_hid_n_report(0x00, REPORT_ID_KEYBOARD, &nkro_report,
                       sizeof(nkro_report));
    } else {
      tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta[0], delta[1], 0, 0);
    }
    // Alternate reports
    kbm_report = !kbm_report;
  }
}

/**
 * DMA Encoder Logic For 2 Encoders
 **/
void irq0_dma_handler() {
  for (int i = 0; i < ENC_GPIO_SIZE; i++) {
    if (dma_channel_get_irq0_status(enc_dma_chan[i])) {
      dma_channel_acknowledge_irq0(enc_dma_chan[i]);
      dma_channel_set_read_addr(enc_dma_chan[i], &pio_0->rxf[i], true);
    }
  }
}

//Switch pio on irq1
void irq1_dma_handler() {
  if (dma_channel_get_irq1_status(sw_dma_chan)) {  
    // Clear the interrupt request.
    dma_channel_acknowledge_irq1(sw_dma_chan);
    //Check rxf for data
    if (pio_sm_get_rx_fifo_level(pio_1, sw_sm) > 0){
      //Has data, copy RXF to variable
      dma_channel_configure(sw_dma_chan, &sw_rx_c, &sw_data, &pio_1->rxf[sw_sm], 1, true);
    } else {
      //RXF empty, put bitmask into TXF
      dma_channel_configure(sw_dma_chan, &sw_tx_c, &pio_1->txf[sw_sm], &sw_bitmask, 1, true);
    }
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

  //Enable DMA IRQ
  irq_set_exclusive_handler(DMA_IRQ_0, irq0_dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);
  irq_set_exclusive_handler(DMA_IRQ_1, irq1_dma_handler);
  irq_set_enabled(DMA_IRQ_1, true);

  //Name PIOs, dont need?
  pio_0 = pio0;         //SM 0-3 encoders
  pio_1 = pio1;         //SM 0 WS2812B, 1 Switches

  //PIOs have a limit of 32 instructions each
  //PIO 0  -  Encoders 23 instructions
  // Set up the state machine for encoders
  uint offset = pio_add_program(pio_0, &encoders_program);

  // Setup Encoders, i = sm
  for (int i = 0; i < ENC_GPIO_SIZE; i++) {
    enc_val[i] = 0;
    prev_enc_val[i] = 0;
    cur_enc_val[i] = 0;
    delta[i] = 0;
    encoders_program_init(pio_0, i, offset, ENC_GPIO[i], ENC_DEBOUNCE);

    enc_dma_chan[i] = dma_claim_unused_channel(true);     //Claim DMA channel
    dma_channel_config c = dma_channel_get_default_config(enc_dma_chan[i]);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio_0, i, false));

    dma_channel_configure(enc_dma_chan[i], &c,
                          &enc_val[i],   // Destinatinon pointer
                          &pio_0->rxf[i],  // Source pointer
                          0x10,          // Number of transfers
                          true           // Start immediately
    );
    dma_channel_set_irq0_enabled(enc_dma_chan[i], true);
  }

  //PIO 1  -  WS2812B 4 + Switches 17 = 21 instructions
  // Set up WS2812B
  uint offset2 = pio_add_program(pio_1, &ws2812_program);
  ws2812_program_init(pio_1, 0, offset2, WS2812B_GPIO, 800000,
                      false);

  //Switches
  //Prep bitmask and gpio pins
  sw_bitmask = 0;
  for (int i = 0; i < SW_GPIO_SIZE; i++) {
    sw_bitmask |= (0x1 << SW_GPIO[i]);
    gpio_pull_up(SW_GPIO[i]);
    gpio_init(SW_GPIO[i]);
    sw_debounce_timestamp[i] = 0;
  }
  prev_sw_data = 0;

  //State Machine
  sw_sm = 1;
  uint sw_offset = pio_add_program(pio_1, &switches_program);
  switches_program_init(pio_1, sw_sm, sw_offset, SW_DEBOUNCE);
  //Get dma channel
  sw_dma_chan = dma_claim_unused_channel(true);
  //Configure
  sw_rx_c = dma_channel_get_default_config(sw_dma_chan);
  sw_tx_c = dma_channel_get_default_config(sw_dma_chan);
  //Set DREQs
  channel_config_set_dreq(&sw_rx_c, pio_get_dreq(pio_1, sw_sm, false));
  channel_config_set_dreq(&sw_tx_c, pio_get_dreq(pio_1, sw_sm, true));
  //Disable increments
  channel_config_set_read_increment(&sw_rx_c, false);
  channel_config_set_read_increment(&sw_tx_c, false);
  // Tell the DMA to raise IRQ line 0 when the channel finishes a block
  dma_channel_set_irq1_enabled(sw_dma_chan, true);
  // Trigger first transfer
  dma_channel_configure(sw_dma_chan, &sw_tx_c, &pio_1->txf[sw_sm], &sw_bitmask, 1, true);

  // Setup LED GPIO
  for (int i = 0; i < LED_GPIO_SIZE; i++) {
    gpio_init(LED_GPIO[i]);
    gpio_set_dir(LED_GPIO[i], GPIO_OUT);
  }

  // Set listener bools
  kbm_report = false;

  // Joy/KB Mode Switching
  if (gpio_get(SW_GPIO[0])) {
    loop_mode = &joy_mode;
    joy_mode_check = true;
  } else {
    loop_mode = &key_mode;
    joy_mode_check = false;
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
  init();
  tusb_init();

  multicore_launch_core1(core1_entry);

  while (1) {
    tud_task();  // tinyusb device task
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
  }
}