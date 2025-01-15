/**
 * Debouncing algorithm which waits for switch to output a constant n amount of
 * time before outputting
 * @author SpeedyPotato
 **/

void debounce_deferred() {
  for (int i = SW_GPIO_SIZE - 1; i >= 0; i--) {
    bool sw_raw_val = !gpio_get(SW_GPIO[i]);

    if (sw_raw_val != sw_prev_raw_val[i]) {
      sw_timestamp[i] = time_us_64();
    } else if (sw_timestamp[i] != 0 && 
        time_us_64() - sw_timestamp[i] >= SW_DEBOUNCE_TIME_US) {
      sw_cooked_val[i] = sw_raw_val;
      sw_timestamp[i] = 0;
    }
  }
}
