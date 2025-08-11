/** Sector equalizer: per-button wedges **/
void ws_sector_equalizer(uint32_t counter, bool hid_mode)
{
    int ledsPerBtn = WS2812B_LED_SIZE / SW_GPIO_SIZE;
    set_color_palette(PALETTE_EARTH);

    for (int b = 0; b < SW_GPIO_SIZE; ++b)
    {
        int start = b * ledsPerBtn;
        int end = (b == SW_GPIO_SIZE - 1) ? WS2812B_LED_SIZE : start + ledsPerBtn;
        int active = (g_buttons >> b) & 1;
        for (int i = start; i < end; ++i)
        {
            uint32_t pc = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 16) % 768);
            uint8_t pr = ((pc >> 8) & 0xFF) / 8;
            uint8_t pg = ((pc >> 16) & 0xFF) / 8;
            uint8_t pb = (pc & 0xFF) / 8;
            RGB_t c = (b < SW_GPIO_SIZE / 2) ? hid_rgb[0] : hid_rgb[1];
            float s = hid_mode ? 0.7f : 1.0f;
            if (active)
            {
                leds[i].r = (uint8_t)fminf(255.0f, s * (c.r + pr));
                leds[i].g = (uint8_t)fminf(255.0f, s * (c.g + pg));
                leds[i].b = (uint8_t)fminf(255.0f, s * (c.b + pb));
            }
            else
            {
                leds[i].r = pr;
                leds[i].g = pg;
                leds[i].b = pb;
            }
        }
    }
}
