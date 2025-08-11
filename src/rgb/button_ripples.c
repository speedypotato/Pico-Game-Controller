/** Button-triggered ripples expanding on the ring **/
#include <math.h>
static inline float circ_dist(float a, float b, float m)
{
    float d = fabsf(a - b);
    return d < (m - d) ? d : (m - d);
}

void ws_button_ripples(uint32_t counter, bool hid_mode)
{
    set_color_palette(PALETTE_OCEAN);
    // map buttons to angles evenly
    float m = (float)WS2812B_LED_SIZE;

    static uint16_t prev_btn = 0;
    uint16_t now = g_buttons;
    uint16_t pressed = (~prev_btn) & now;
    prev_btn = now;

    typedef struct
    {
        float center;
        uint32_t born;
        uint8_t alive;
        uint8_t zone;
    } ripple_t;
#define MAX_R 6
    static ripple_t rip[MAX_R] = {0};

    // Spawn ripples on new presses
    for (int bi = 0; bi < SW_GPIO_SIZE; ++bi)
    {
        if (pressed & (1u << bi))
        {
            for (int k = 0; k < MAX_R; ++k)
                if (!rip[k].alive)
                {
                    rip[k].alive = 1;
                    rip[k].born = counter;
                    rip[k].center = (bi * m) / SW_GPIO_SIZE;
                    rip[k].zone = (bi < (SW_GPIO_SIZE / 2)) ? 0 : 1;
                    break;
                }
        }
    }

    for (int i = 0; i < WS2812B_LED_SIZE; ++i)
    {
        // base dim palette
        uint32_t base = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 16) % 768);
        uint8_t r = ((base >> 8) & 0xFF) / 10;
        uint8_t g = ((base >> 16) & 0xFF) / 10;
        uint8_t b = (base & 0xFF) / 10;

        for (int k = 0; k < MAX_R; ++k)
            if (rip[k].alive)
            {
                float age = (counter - rip[k].born) * 0.25f; // expands ~4px per 5ms
                float d = circ_dist(i, rip[k].center, m);
                float w = expf(-fabsf(d - age) * 0.9f);
                RGB_t c = hid_rgb[rip[k].zone];
                float s = hid_mode ? 0.6f : 1.0f;
                r = (uint8_t)fminf(255.0f, r + s * w * c.r);
                g = (uint8_t)fminf(255.0f, g + s * w * c.g);
                b = (uint8_t)fminf(255.0f, b + s * w * c.b);
                if (age > m)
                    rip[k].alive = 0;
            }

        leds[i].r = r;
        leds[i].g = g;
        leds[i].b = b;
    }
}
