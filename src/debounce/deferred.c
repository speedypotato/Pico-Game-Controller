/**
 * Debouncing algorithm which waits for switch to output a constant n amount of
 * time before outputting
 * @author SpeedyPotato
 **/

uint16_t debounce_deferred() {
  uint16_t translate_buttons = 0;
  for (int i = SW_GPIO_SIZE - 1; i >= 0; i--) {
    if (!gpio_get(SW_GPIO[i]) &&
        time_us_64() - sw_timestamp[i] >= SW_DEBOUNCE_TIME_US) {
      translate_buttons = (translate_buttons << 1) | 1;
    } else {
      translate_buttons <<= 1;
    }
  }
  return translate_buttons;
}