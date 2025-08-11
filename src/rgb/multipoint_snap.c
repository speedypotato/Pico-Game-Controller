/** Multi-point chase that snaps to button angles and recolors **/
#include <math.h>
void ws_multipoint_snap(uint32_t counter, bool hid_mode)
{
#define PTS 4
    static float pts[PTS] = {0};
    static uint8_t hueShift = 0;

    float pos = ((enc_val[0] % ENC_PULSE) / (float)ENC_PULSE) * WS2812B_LED_SIZE;
    for (int k = 0; k < PTS; ++k)
    {
        float target = fmodf(pos + (k * WS2812B_LED_SIZE / PTS), WS2812B_LED_SIZE);
        float diff = target - pts[k];
        if (diff > WS2812B_LED_SIZE / 2)
            diff -= WS2812B_LED_SIZE;
        if (diff < -WS2812B_LED_SIZE / 2)
            diff += WS2812B_LED_SIZE;
        pts[k] = fmodf(pts[k] + diff * 0.2f, WS2812B_LED_SIZE);
        if (pts[k] < 0)
            pts[k] += WS2812B_LED_SIZE;
    }

    // snap on button events
    static uint16_t last = 0;
    uint16_t now = g_buttons;
    uint16_t press = (~last) & now;
    last = now;
    if (press)
    {
        int bi = __builtin_ctz(press);
        float ang = (bi * WS2812B_LED_SIZE) / (float)SW_GPIO_SIZE;
        pts[bi % PTS] = ang;
        hueShift += 16;
    }

    set_color_palette(PALETTE_VIRIDIS);
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        uint32_t base = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 8 + hueShift) % 768);
        float r = ((base >> 8) & 0xFF) * 0.15f;
        float g = ((base >> 16) & 0xFF) * 0.15f;
        float b = (base & 0xFF) * 0.15f;
        for (int k = 0; k < PTS; ++k)
        {
            float d = fabsf(pts[k] - i);
            if (d > WS2812B_LED_SIZE - d)
                d = WS2812B_LED_SIZE - d;
            float w = expf(-d * 0.9f);
            r += w * hid_rgb[k & 1].r;
            g += w * hid_rgb[k & 1].g;
            b += w * hid_rgb[k & 1].b;
        }
        float s = hid_mode ? 0.75f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, s * r);
        leds[i].g = (uint8_t)fminf(255.0f, s * g);
        leds[i].b = (uint8_t)fminf(255.0f, s * b);
    }
}
