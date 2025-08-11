/**
 * Simple header file to include all files in the folder
 * @author SpeedyPotato
 *
 * To add a debounce mode, return a uint16_t representing the button states.
 * These are saved in report.buttons as truth. Create debounce mode as desired
 * and then add the #include here.
 **/
extern uint64_t sw_timestamp[SW_GPIO_SIZE];

#include "deferred.c"
#include "eager.c"