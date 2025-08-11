/**
 * @author 4yn, SpeedyPotato
 * Turbocharger chasing laser effect
 *
 * Move 2 lighting areas around the controller depending on knob input.
 *
 * For each knob, calculate every 5 ms:
 * - Add any knob delta to a counter
 * - Clamp counter to some "maximum speed"
 * - If counter is far enough from 0, knob is moving
 * - If counter is too near 0 and has been there for a while, fade out
 * - Move the lighting area in the correct direction at a constant speed
 * - Decay the counter
 *
 * Curent values are tuned for
 * - Lights sampled at 200 Hz
 * - 0.1 rotations for lights to activate
 * - Lighting areas take 0.75s to make one full rotation
 * - Movement takes 0.5s to decay to stop
 * - Fade out takes another 0.2s samples to disappear
 *
 * LEDs are positioned as:
 * - 0, 1 as dummy leds on top of controller
 * - 2-6 LEDs on right edge, top to bottom
 * - 7-9 as dummy leds on bottom of controller
 * - 10-14 LEDs on left edge, bottom to top
 * - 15 as dummy led on top of controller
 *
 * Lighting areas start at position 0 and will light up the 3 nearest LEDs.
 * By strategically positioning led 0 at the top, this avoids lighting areas
 * from suddenly appeaering.
 **/

#define TURBO_LIGHTS_CLAMP 0.1f
#define TURBO_LIGHTS_THRESHOLD 0.05f
#define TURBO_LIGHTS_DECAY 0.0005f
#define TURBO_LIGHTS_VEL 0.12f
#define TURBO_LIGHTS_MAX (WS2812B_LED_SIZE + 6.0f)
#define TURBO_LIGHTS_FADE 40
#define TURBO_LIGHTS_FADE_VEL 0.025f

int i_clamp(int d, int min, int max)
{
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

float f_clamp(float d, float min, float max)
{
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

float f_one_mod(float d, float mod)
{
  const float t = d < 0 ? d + mod : d;
  return t > mod ? t - mod : t;
}

float f_abs(float d)
{
  return d < 0 ? -d : d;
}

uint32_t turbo_prev_enc_val[ENC_GPIO_SIZE];
float turbo_cur_enc_val[ENC_GPIO_SIZE];
float turbo_lights_pos[ENC_GPIO_SIZE];
float turbo_lights_brightness[ENC_GPIO_SIZE];
int turbo_lights_idle[ENC_GPIO_SIZE];

void turbocharger_color_cycle(uint32_t unused, bool hid_mode)
{
  for (int i = 0; i < ENC_GPIO_SIZE; i++)
  {
    int enc_delta = (enc_val[i] - turbo_prev_enc_val[i]) * (ENC_REV[i] ? 1 : -1);
    turbo_prev_enc_val[i] = enc_val[i];
    turbo_cur_enc_val[i] = f_clamp(turbo_cur_enc_val[i] + (float)(enc_delta) / ENC_PULSE, -TURBO_LIGHTS_CLAMP, TURBO_LIGHTS_CLAMP);

    if (turbo_cur_enc_val[i] < -TURBO_LIGHTS_THRESHOLD)
    {
      turbo_lights_idle[i] = 0;
      turbo_lights_pos[i] += TURBO_LIGHTS_VEL;
      turbo_lights_brightness[i] = 1.0f;
    }
    else if (turbo_cur_enc_val[i] > TURBO_LIGHTS_THRESHOLD)
    {
      turbo_lights_idle[i] = 0;
      turbo_lights_pos[i] -= TURBO_LIGHTS_VEL;
      turbo_lights_brightness[i] = 1.0f;
    }
    else
    {
      turbo_lights_idle[i]++;
      if (turbo_lights_idle[i] > TURBO_LIGHTS_FADE)
      {
        turbo_lights_pos[i] = 0;
      }
      else
      {
        turbo_lights_brightness[i] = f_clamp(turbo_lights_brightness[i] - TURBO_LIGHTS_FADE_VEL, 0.0f, 1.0f);
      }
    }

    turbo_lights_pos[i] = f_one_mod(turbo_lights_pos[i], TURBO_LIGHTS_MAX);

    if (turbo_cur_enc_val[i] < -TURBO_LIGHTS_DECAY)
    {
      turbo_cur_enc_val[i] += TURBO_LIGHTS_DECAY;
    }
    else if (turbo_cur_enc_val[i] > TURBO_LIGHTS_DECAY)
    {
      turbo_cur_enc_val[i] -= TURBO_LIGHTS_DECAY;
    }
  }

  for (int i = 0; i < WS2812B_LED_SIZE; i++)
  {
    float pos = 2.0f + i + (i >= WS2812B_LED_SIZE / 2 ? 3.0f : 0.0f);
    float l_strength = (1.0f - f_clamp(f_abs(turbo_lights_pos[0] - pos), 0.0f, 2.0f) / 2) * turbo_lights_brightness[0];
    float r_strength = (1.0f - f_clamp(f_abs(turbo_lights_pos[1] - pos), 0.0f, 2.0f) / 2) * turbo_lights_brightness[1];

    // Set colors in leds array instead of calling put_pixel directly
    leds[i].r = i_clamp(l_strength * 70 + r_strength * 250, 0, 255);
    leds[i].g = i_clamp(l_strength * 230 + r_strength * 60, 0, 255);
    leds[i].b = i_clamp(l_strength * 250 + r_strength * 200, 0, 255);
  }
}
