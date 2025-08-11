/**
 * Multi-point trail effect - creates 5 trailing light effects based on encoder movement
 * @author Renard
 **/

#define NUM_TRAIL_POINTS 5

static uint32_t trail_prev_enc_val = 0;
static float trail_positions[NUM_TRAIL_POINTS] = {0};
static uint8_t trail_brightness[WS2812B_LED_SIZE] = {0};
static bool trail_initialized = false;
static uint64_t last_encoder_change_time = 0;

void ws2812b_trail(uint32_t counter, bool hid_mode)
{
    int TRAIL_DECAY_RATE = 2;
    // Initialize trail positions if not done yet
    if (!trail_initialized)
    {
        for (int i = 0; i < NUM_TRAIL_POINTS; i++)
        {
            trail_positions[i] = (float)(i * WS2812B_LED_SIZE) / NUM_TRAIL_POINTS;
        }
        last_encoder_change_time = time_us_64(); // Initialize timestamp
        trail_initialized = true;
    }

    float position_delta = 0.0f;

    int enc_delta = (enc_val[0] - trail_prev_enc_val) * (ENC_REV[0] ? -1 : 1); // reverse the encoder value cuz i messed up the wiring lol

    // Check if encoder value has changed
    if (enc_delta != 0)
    {
        last_encoder_change_time = time_us_64();
    }

    trail_prev_enc_val = enc_val[0];

    // Update trail positions based on encoder movement or timeout
    uint64_t current_time = time_us_64();
    uint64_t time_since_last_change = current_time - last_encoder_change_time;
    const uint64_t TIMEOUT_3_SECONDS = 3000000; // 3 seconds in microseconds

    if (time_since_last_change >= TIMEOUT_3_SECONDS)
    {
        // No encoder movement for 3+ seconds, use slow automatic movement
        position_delta = 0.05f;
        TRAIL_DECAY_RATE = 4;
    }
    else
    {
        // Normal encoder-based movement
        position_delta = (float)enc_delta / ENC_PULSE * WS2812B_LED_SIZE;
        TRAIL_DECAY_RATE = 16;
    } // Update all trail point positions
    for (int i = 0; i < NUM_TRAIL_POINTS; i++)
    {
        trail_positions[i] += position_delta;

        // Keep positions within bounds (circular)
        while (trail_positions[i] < 0)
            trail_positions[i] += WS2812B_LED_SIZE;
        while (trail_positions[i] >= WS2812B_LED_SIZE)
            trail_positions[i] -= WS2812B_LED_SIZE;
    }

    // Decay all trail brightness values

    for (int i = 0; i < WS2812B_LED_SIZE; i++)
    {
        if (trail_brightness[i] > 0)
        {
            trail_brightness[i] = trail_brightness[i] > TRAIL_DECAY_RATE ? trail_brightness[i] - TRAIL_DECAY_RATE : 0;
        }
    }

    // Set brightness at current positions for all 5 points
    for (int point = 0; point < NUM_TRAIL_POINTS; point++)
    {
        int pos = (int)trail_positions[point];
        if (pos >= 0 && pos < WS2812B_LED_SIZE)
        {
            // Each point has maximum brightness, trails will fade naturally
            trail_brightness[pos] = 255;

            // Add some brightness to adjacent LEDs for smoother effect
            int prev_pos = (pos - 1 + WS2812B_LED_SIZE) % WS2812B_LED_SIZE;
            int next_pos = (pos + 1) % WS2812B_LED_SIZE;

            if (trail_brightness[prev_pos] < 180)
                trail_brightness[prev_pos] = 180;
            if (trail_brightness[next_pos] < 180)
                trail_brightness[next_pos] = 180;
        }
    }

    // Apply trail effect to LEDs
    for (int i = 0; i < WS2812B_LED_SIZE; i++)
    {
        uint8_t brightness = trail_brightness[i];

        // Change palette based on counter to demonstrate different palettes
        // This cycles through palettes every ~10 seconds at 200Hz
        // int demo_palette = (counter / 2000) % 9; // 9 palettes available (0-8)
        set_color_palette(PALETTE_NEON);

        // Create a color that shifts through the spectrum based on position
        // Each of the 5 trail points will have different colors
        uint32_t color = color_wheel((i * 768 / WS2812B_LED_SIZE + counter / 8) % 768);

        // Apply brightness scaling
        leds[i].r = (((color >> 8) & 0xFF) * brightness) / 255;
        leds[i].g = (((color >> 16) & 0xFF) * brightness) / 255;
        leds[i].b = ((color & 0xFF) * brightness) / 255;
    }
}
