/** Velocity comet with deceleration sparks **/
#include <math.h>
void ws_velocity_comet(uint32_t counter, bool hid_mode)
{
    static uint32_t prev_enc = 0;
    int d = (int)(enc_val[0] - prev_enc);
    prev_enc = enc_val[0];
    static float vel = 0.0f;
    float target = (float)d / ENC_PULSE * WS2812B_LED_SIZE;
    vel = 0.85f * vel + 0.15f * target;
    static float pos = 0.0f;
    pos = fmodf(pos + vel, (float)WS2812B_LED_SIZE);
    if (pos < 0)
        pos += WS2812B_LED_SIZE;

    set_color_palette(PALETTE_PLASMA);

    static uint8_t trail[WS2812B_LED_SIZE] = {0};
    // decay trail proportional to speed
    int decay = (int)fminf(16.0f, fabsf(vel) * 6.0f + 2.0f);
    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
        trail[i] = trail[i] > decay ? trail[i] - decay : 0;

    int p = (int)pos;
    trail[p] = 255;
    if (trail[(p + 1) % WS2812B_LED_SIZE] < 200)
        trail[(p + 1) % WS2812B_LED_SIZE] = 200;
    if (trail[(p - 1 + WS2812B_LED_SIZE) % WS2812B_LED_SIZE] < 200)
        trail[(p - 1 + WS2812B_LED_SIZE) % WS2812B_LED_SIZE] = 200;

    // decel sparks
    static float last_vel = 0.0f;
    if (vel < last_vel - 0.02f)
    {
        int s = (p + (vel > 0 ? -2 : 2) + WS2812B_LED_SIZE) % WS2812B_LED_SIZE;
        trail[s] = 255;
    }
    last_vel = vel;

    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        uint8_t br = trail[i];
        uint32_t c = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 4) % 768);
        uint8_t r = ((c >> 8) & 0xFF) * br / 255;
        uint8_t g = ((c >> 16) & 0xFF) * br / 255;
        uint8_t b = (c & 0xFF) * br / 255;
        leds[i].r = r;
        leds[i].g = g;
        leds[i].b = b;
    }
}
