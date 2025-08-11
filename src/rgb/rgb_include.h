/**
 * Simple header file to include all files in the folder
 * @author SpeedyPotato
 *
 * To add a lighting mode, create a function which accepts a uint32_t counter and bool hid_mode as parameters.
 * Create lighting mode as desired and then add the #include here.
 **/

// Forward declaration for RGB_t type (defined in main file)
#ifndef RGB_T_DEFINED
#define RGB_T_DEFINED
typedef struct
{
  uint8_t r, g, b;
} RGB_t;
#endif

extern uint32_t enc_val[ENC_GPIO_SIZE];
extern RGB_t leds[WS2812B_LED_SIZE];      // Reference to FastLED-style LED array
extern const bool ENC_REV[ENC_GPIO_SIZE]; // External reference to encoder reverse array
extern volatile uint16_t g_buttons;       // Button bitmask published by core 0
extern RGB_t hid_rgb[WS2812B_LED_ZONES];  // Two HID-provided RGB colors

#include "ws2812b_util.c"
#include "color_cycle.c"
#include "turbocharger.c"
#include "trail.c"
// New effects
#include "demo_all.c"
#include "dual_orbit.c"
#include "velocity_comet.c"
#include "button_ripples.c"
#include "spokes.c"
#include "counter_stripes.c"
#include "palette_tint_gradient.c"
#include "multipoint_snap.c"
#include "center_pulse.c"
#include "sector_equalizer.c"
#include "radar_sweep.c"
