/** Palette gradient around ring with HID tint scaled by button activity **/
#include <math.h>
void ws_palette_tint_gradient(uint32_t counter, bool hid_mode)
{
    set_color_palette(PALETTE_SUNSET);
    int active = __builtin_popcount(g_buttons);
    float tint = fminf(0.3f, active * 0.03f);
    RGB_t tintColor = hid_rgb[0];

    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        uint32_t pc = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 12) % 768);
        float r = (pc >> 8) & 0xFF;
        float g = (pc >> 16) & 0xFF;
        float b = pc & 0xFF;
        float s = hid_mode ? 0.7f : 1.0f;
        r = (1.0f - tint) * r + tint * tintColor.r;
        g = (1.0f - tint) * g + tint * tintColor.g;
        b = (1.0f - tint) * b + tint * tintColor.b;
        leds[i].r = (uint8_t)fminf(255.0f, s * r);
        leds[i].g = (uint8_t)fminf(255.0f, s * g);
        leds[i].b = (uint8_t)fminf(255.0f, s * b);
    }
}
