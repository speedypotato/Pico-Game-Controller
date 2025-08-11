/**
 * Demo: cycle through all available effects with smooth crossfades
 * Runs each effect for PHASE_TICKS (~seconds = PHASE_TICKS/200),
 * then crossfades to the next over FADE_TICKS.
 */

#include <stdint.h>
#include <stdbool.h>

// Forward declarations of effects we will showcase
void ws_dual_orbit(uint32_t counter, bool hid_mode);
void ws_velocity_comet(uint32_t counter, bool hid_mode);
void ws_button_ripples(uint32_t counter, bool hid_mode);
void ws_spokes(uint32_t counter, bool hid_mode);
void ws_counter_stripes(uint32_t counter, bool hid_mode);
void ws_palette_tint_gradient(uint32_t counter, bool hid_mode);
void ws_multipoint_snap(uint32_t counter, bool hid_mode);
void ws_center_pulse(uint32_t counter, bool hid_mode);
void ws_sector_equalizer(uint32_t counter, bool hid_mode);
void ws_radar_sweep(uint32_t counter, bool hid_mode);
void ws2812b_color_cycle(uint32_t counter, bool hid_mode);
void turbocharger_color_cycle(uint32_t counter, bool hid_mode);
void ws2812b_trail(uint32_t counter, bool hid_mode);

// Provided by ws2812b_util.c and main
extern RGB_t leds[WS2812B_LED_SIZE];

// Local buffer for crossfade
static RGB_t demo_tmp[WS2812B_LED_SIZE];

// Timing tuned for 200 Hz (core1 loop sleeps ~5 ms per frame)
#define PHASE_TICKS 1600u // ~8 seconds per effect
#define FADE_TICKS 300u   // ~1.5 seconds crossfade

static inline void copy_leds(RGB_t *dst, const RGB_t *src)
{
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
        dst[i] = src[i];
}

static inline void crossfade(RGB_t *out, const RGB_t *from, const RGB_t *to, uint16_t alpha /*0-255*/)
{
    uint16_t inv = 255u - alpha;
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        out[i].r = (uint8_t)((from[i].r * inv + to[i].r * alpha) / 255u);
        out[i].g = (uint8_t)((from[i].g * inv + to[i].g * alpha) / 255u);
        out[i].b = (uint8_t)((from[i].b * inv + to[i].b * alpha) / 255u);
    }
}

void ws_demo_all(uint32_t counter, bool hid_mode)
{
    // List of effects to showcase
    static void (*effects[])(uint32_t, bool) = {
        ws_dual_orbit,
        ws_velocity_comet,
        ws_button_ripples,
        ws_spokes,
        ws_counter_stripes,
        ws_palette_tint_gradient,
        ws_multipoint_snap,
        ws_center_pulse,
        ws_sector_equalizer,
        ws_radar_sweep,
        ws2812b_color_cycle,
        turbocharger_color_cycle,
        ws2812b_trail,
    };

    const uint32_t effects_count = sizeof(effects) / sizeof(effects[0]);
    const uint32_t phase = counter / PHASE_TICKS;
    const uint32_t idx = phase % effects_count;
    const uint32_t next = (idx + 1) % effects_count;
    const uint32_t phase_pos = counter % PHASE_TICKS;

    if (phase_pos < (PHASE_TICKS - FADE_TICKS))
    {
        // Show current effect only
        uint32_t local = phase_pos; // per-effect time base
        effects[idx](local, hid_mode);
    }
    else
    {
        // Crossfade current -> next
        uint32_t local_curr = PHASE_TICKS - FADE_TICKS - 1;           // freeze tail of current
        uint32_t local_next = phase_pos - (PHASE_TICKS - FADE_TICKS); // 0..FADE_TICKS-1
        uint16_t alpha = (uint16_t)((local_next * 255u) / (FADE_TICKS - 1));

        // Render current into demo_tmp
        effects[idx](local_curr, hid_mode);
        copy_leds(demo_tmp, leds);

        // Render next into leds, then blend into leds
        effects[next](local_next, hid_mode);
        crossfade(leds, demo_tmp, leds, alpha);
    }
}
