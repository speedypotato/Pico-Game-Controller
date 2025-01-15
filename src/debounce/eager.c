/**
 * Debouncing algorithm which sends a report immediately when a switch is
 * triggered and holds it for n amount of time.
 * @author SpeedyPotato
 **/

void debounce_eager() {
  for (int i = 0; i < SW_GPIO_SIZE; i++) {
    bool sw_raw_val = !gpio_get(SW_GPIO[i]);

    if (time_us_64() - sw_timestamp[i] >= SW_DEBOUNCE_TIME_US &&
        sw_cooked_val[i] != sw_raw_val) {
      sw_cooked_val[i] = sw_raw_val;
      sw_timestamp[i] = time_us_64();
    }
  }
}
