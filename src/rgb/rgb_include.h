/**
 * Simple header file to include all files in the folder
 * @author SpeedyPotato
 * 
 * To add a lighting mode, create a function which accepts a uint32_t as a parameter.
 * Create lighting mode as desired and then add the #include here.
 **/
extern uint32_t enc_val[ENC_GPIO_SIZE];

#include "ws2812b_util.c"
#include "color_cycle.c"
#include "turbocharger.c"