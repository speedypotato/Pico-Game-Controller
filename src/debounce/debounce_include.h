/**
 * Simple header file to include all files in the folder
 * @author SpeedyPotato
 *
 * A debounce mode function modifies sw_cooked_val to update button states.
 * These are saved in report.buttons as truth. Create debounce mode as desired
 * and then add the #include here.
 *
 * At the start of the debounce function, sw_cooked_val is the state of the
 * buttons from the previous cycle. You should change it to be the new state
 * by the end of the function. sw_prev_raw_val contains the state of the GPIO
 * pins on the previous cycle. sw_timestamp is for you to use.
 **/
extern bool sw_prev_raw_val[SW_GPIO_SIZE];
extern bool sw_cooked_val[SW_GPIO_SIZE];
extern uint64_t sw_timestamp[SW_GPIO_SIZE];

#include "deferred.c"
#include "eager.c"
