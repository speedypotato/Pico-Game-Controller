/** Dual orbit effect using HID colors C0/C1 and palette glow **/
#include <math.h>
void ws_dual_orbit(uint32_t counter, bool hid_mode)
{
    // Derive position from encoder 0
    float pos = ((enc_val[0] % ENC_PULSE) / (float)ENC_PULSE) * WS2812B_LED_SIZE;
    // Two heads 180 degrees apart, directions by encoder sign approximation
    static uint32_t prev_enc = 0;
    int enc_delta = (int)(enc_val[0] - prev_enc);
    prev_enc = enc_val[0];
    int dir = (enc_delta >= 0) ? 1 : -1;

    float head0 = fmodf(pos + dir * (counter * 0.05f), WS2812B_LED_SIZE);
    float head1 = fmodf(head0 + WS2812B_LED_SIZE / 2.0f, WS2812B_LED_SIZE);

    set_color_palette(PALETTE_NEON);

    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        // background glow from palette
        uint32_t bg = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 8) % 768);
        uint8_t br = 10; // low base brightness
        uint8_t r = ((bg >> 8) & 0xFF) * br / 255;
        uint8_t g = ((bg >> 16) & 0xFF) * br / 255;
        uint8_t b = (bg & 0xFF) * br / 255;

        // distance helper
        float d0 = fabsf(head0 - i);
        if (d0 > WS2812B_LED_SIZE - d0)
            d0 = WS2812B_LED_SIZE - d0;
        float d1 = fabsf(head1 - i);
        if (d1 > WS2812B_LED_SIZE - d1)
            d1 = WS2812B_LED_SIZE - d1;

        // gaussian-ish falloff
        float w0 = expf(-d0 * 0.8f);
        float w1 = expf(-d1 * 0.8f);

        // mix HID colors
        uint8_t r0 = hid_rgb[0].r, g0 = hid_rgb[0].g, b0 = hid_rgb[0].b;
        uint8_t r1 = hid_rgb[1].r, g1 = hid_rgb[1].g, b1 = hid_rgb[1].b;

        float fr = r + w0 * r0 + w1 * r1;
        float fg = g + w0 * g0 + w1 * g1;
        float fb = b + w0 * b0 + w1 * b1;

        // hid_mode: when true, reduce HID dominance
        float hid_scale = hid_mode ? 0.5f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, br + hid_scale * fr);
        leds[i].g = (uint8_t)fminf(255.0f, br + hid_scale * fg);
        leds[i].b = (uint8_t)fminf(255.0f, br + hid_scale * fb);
    }
}
