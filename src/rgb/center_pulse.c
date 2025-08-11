/** Pulses that originate near two centers; button proximity triggers **/
#include <math.h>
#include "centered_helpers.h"
void ws_center_pulse(uint32_t counter, bool hid_mode)
{
    float centers[2] = {0.0f, WS2812B_LED_SIZE / 2.0f};
    static uint32_t born[2] = {0, 0};

    // trigger if any button mapped near a center is pressed
    static uint16_t last = 0;
    uint16_t now = g_buttons;
    uint16_t press = (~last) & now;
    last = now;
    if (press)
    {
        int bi = __builtin_ctz(press);
        float a = (bi * WS2812B_LED_SIZE) / (float)SW_GPIO_SIZE;
        int ci = (circular_distance(a, centers[0], WS2812B_LED_SIZE) < circular_distance(a, centers[1], WS2812B_LED_SIZE)) ? 0 : 1;
        born[ci] = counter; // retrigger
    }

    set_color_palette(PALETTE_ARCTIC);
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        float r = 0, g = 0, b = 0;
        uint32_t base = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 20) % 768);
        float br = ((base >> 8) & 0xFF) * 0.2f;
        float bg = ((base >> 16) & 0xFF) * 0.2f;
        float bb = (base & 0xFF) * 0.2f;

        for (int c = 0; c < 2; ++c)
        {
            float age = (counter - born[c]) * 0.4f;
            float d = circular_distance(i, centers[c], WS2812B_LED_SIZE);
            float w = expf(-fabsf(d - age) * 0.8f);
            r += w * hid_rgb[c].r;
            g += w * hid_rgb[c].g;
            b += w * hid_rgb[c].b;
        }
        float s = hid_mode ? 0.8f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, s * (br + r));
        leds[i].g = (uint8_t)fminf(255.0f, s * (bg + g));
        leds[i].b = (uint8_t)fminf(255.0f, s * (bb + b));
    }
}
