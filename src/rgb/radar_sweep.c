/** Radar sweep with persistent decay **/
#include <math.h>
void ws_radar_sweep(uint32_t counter, bool hid_mode)
{
    static uint8_t buf[WS2812B_LED_SIZE] = {0};
    static float pos = 0.0f;
    // speed from encoder
    static uint32_t prev = 0;
    int d = (int)(enc_val[0] - prev);
    prev = enc_val[0];
    float v = (float)d / ENC_PULSE * 4.0f; // adjust speed
    pos += v;
    if (pos < 0)
        pos += WS2812B_LED_SIZE;
    if (pos >= WS2812B_LED_SIZE)
        pos -= WS2812B_LED_SIZE;

    int head = (int)pos;
    buf[head] = 255;

    // decay
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
        buf[i] = buf[i] > 2 ? buf[i] - 2 : 0;

    set_color_palette(PALETTE_RAINBOW);
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        uint8_t br = buf[i];
        RGB_t c = hid_rgb[0];
        uint32_t p = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 6) % 768);
        uint8_t pr = ((p >> 8) & 0xFF) / 4;
        uint8_t pg = ((p >> 16) & 0xFF) / 4;
        uint8_t pb = (p & 0xFF) / 4;
        float s = hid_mode ? 0.8f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, s * (pr + (c.r * br / 255)));
        leds[i].g = (uint8_t)fminf(255.0f, s * (pg + (c.g * br / 255)));
        leds[i].b = (uint8_t)fminf(255.0f, s * (pb + (c.b * br / 255)));
    }
}
