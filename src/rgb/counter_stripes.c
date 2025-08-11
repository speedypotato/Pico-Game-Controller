/** Counter-rotating stripes per zone with HID tints **/
#include <math.h>
void ws_counter_stripes(uint32_t counter, bool hid_mode)
{
    int stripes = 6;
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        int zone = (i < (WS2812B_LED_SIZE / 2)) ? 0 : 1;
        int localIdx = zone == 0 ? i : i - (WS2812B_LED_SIZE / 2);
        int zoneLen = WS2812B_LED_SIZE / 2;
        int dir = (zone == 0) ? 1 : -1;
        int idx = (localIdx + dir * ((counter / 3) % zoneLen)) % zoneLen;
        float band = (idx * stripes / (float)zoneLen);
        float w = (fmodf(band, 1.0f) < 0.5f) ? 1.0f : 0.2f;

        set_color_palette(zone == 0 ? PALETTE_FIRE : PALETTE_OCEAN);
        uint32_t pc = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 12) % 768);
        uint8_t pr = ((pc >> 8) & 0xFF) * 0.4f;
        uint8_t pg = ((pc >> 16) & 0xFF) * 0.4f;
        uint8_t pb = (pc & 0xFF) * 0.4f;

        float s = hid_mode ? 0.6f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, pr + s * w * hid_rgb[zone].r);
        leds[i].g = (uint8_t)fminf(255.0f, pg + s * w * hid_rgb[zone].g);
        leds[i].b = (uint8_t)fminf(255.0f, pb + s * w * hid_rgb[zone].b);
    }
}
