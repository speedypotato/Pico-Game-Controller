/**
 * Color cycle effect & reactive buttons for Pocket IIDX
 * @author SpeedyPotato
 **/
const uint64_t timeout_us = 600000000;  // 600000000us = 10min
const double WS2812B_BRIGHTNESS = 0.2;
const RGB_t COLOR_BLACK = {0, 0, 0};
const RGB_t SW_COLORS[] = {
    {255, 255, 255},  // Key 1
    {255, 255, 255},  // Key 2
    {255, 255, 255},  // Key 3
    {255, 255, 255},  // Key 4
    {255, 255, 255},  // Key 5
    {255, 255, 255},  // Key 6
    {255, 255, 255},  // Key 7
    {0, 0, 255},      // E1
    {0, 0, 255},      // E2
    {0, 0, 255},      // E3
    {0, 0, 255},      // E4
};

RGB_t ws2812b_data[WS2812B_LED_SIZE] = {0};

void ws2812b_color_cycle_iidx(uint32_t counter) {
  RGB_t default_glow = color_wheel_rgbt(counter);
  default_glow.r *= WS2812B_BRIGHTNESS;
  default_glow.g *= WS2812B_BRIGHTNESS;
  default_glow.b *= WS2812B_BRIGHTNESS;
  // rgb timeout
  uint64_t latest_switch_ts = sw_timestamp[0];
  for (int i = 1; i < SW_GPIO_SIZE; i++)
    if (latest_switch_ts < sw_timestamp[i]) latest_switch_ts = sw_timestamp[i];
  if (time_us_64() > latest_switch_ts + timeout_us) {  // timed out
    for (int i = 0; i < WS2812B_LED_SIZE; i++) ws2812b_data[i] = COLOR_BLACK;
  } else {
    /* Switches */
    for (int i = 0; i < 7; i++) {
      if (time_us_64() - reactive_timeout_timestamp >= REACTIVE_TIMEOUT_MAX) {
        if ((report.buttons >> i) % 2 == 1) {
          ws2812b_data[i + 4] = SW_COLORS[i];
        } else {
          ws2812b_data[i + 4] = default_glow;
        }
      } else {
        if (lights_report.lights.buttons[i] == 0) {
          ws2812b_data[i + 4] = COLOR_BLACK;
        } else {
          ws2812b_data[i + 4] = SW_COLORS[i];
        }
      }
    }
    for (int i = 8; i < 12; i++) {
      if (time_us_64() - reactive_timeout_timestamp >= REACTIVE_TIMEOUT_MAX) {
        if ((report.buttons >> i) % 2 == 1) {
          ws2812b_data[i - 8] = SW_COLORS[i - 1];
        } else {
          ws2812b_data[i - 8] = default_glow;
        }
      } else {
        if (lights_report.lights.buttons[i] == 0) {
          ws2812b_data[i - 8] = COLOR_BLACK;
        } else {
          ws2812b_data[i - 8] = SW_COLORS[i - 1];
        }
      }
    }
    /* Turntable RGB */
    for (int i = WS2812B_LED_SIZE - 8; i < WS2812B_LED_SIZE; i++) {
      int led_offset = (int)(((double)(i - WS2812B_LED_SIZE - 7) / 8) * 768);
      RGB_t base_color = color_wheel_rgbt(counter + led_offset);
      base_color.r *= WS2812B_BRIGHTNESS;
      base_color.g *= WS2812B_BRIGHTNESS;
      base_color.b *= WS2812B_BRIGHTNESS;
      if (time_us_64() - reactive_timeout_timestamp >= REACTIVE_TIMEOUT_MAX) {
        ws2812b_data[i] = base_color;
      } else {
        ws2812b_data[i].r = lights_report.lights.rgb[0].r;
        ws2812b_data[i].g = lights_report.lights.rgb[0].g;
        ws2812b_data[i].b = lights_report.lights.rgb[0].b;
      }
    }
  }
  for (int i = 0; i < WS2812B_LED_SIZE; ++i) {
    put_pixel(
        urgb_u32(ws2812b_data[i].r, ws2812b_data[i].g, ws2812b_data[i].b));
  }
}