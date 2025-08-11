/** Rotating spokes with strobe, colored by HID C0/C1 **/
#include <math.h>
void ws_spokes(uint32_t counter, bool hid_mode)
{
    static uint32_t prev_enc = 0;
    int d = (int)(enc_val[0] - prev_enc);
    prev_enc = enc_val[0];
    float vel = (float)d / ENC_PULSE;

    int N = 8 + ((g_buttons != 0) ? 8 : 0); // double when any button pressed
    float phase = counter * (0.02f + fabsf(vel) * 0.3f);

    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        float angle = (float)i / WS2812B_LED_SIZE * N + phase;
        float spoke = fabsf(sinf(angle * 3.14159f)); // spoke profile
        float stro = (counter / 10) % 2 == 0 ? 1.0f : 0.6f;
        RGB_t c = (i % 2 == 0) ? hid_rgb[0] : hid_rgb[1];
        float s = hid_mode ? 0.7f : 1.0f;
        leds[i].r = (uint8_t)fminf(255.0f, s * stro * spoke * c.r);
        leds[i].g = (uint8_t)fminf(255.0f, s * stro * spoke * c.g);
        leds[i].b = (uint8_t)fminf(255.0f, s * stro * spoke * c.b);
    }
}
