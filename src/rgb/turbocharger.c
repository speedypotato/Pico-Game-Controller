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
 * - Lights sampled at 31.25 Hz (every 32ms)
 * - 0.1 rotations for lights to activate
 * - Lighting areas take 1s to make one full rotation
 * - Movement takes 1s to decay to stop
 * - Fade out takes another 0.5s to disappear
 * 
 * See turbo_led_base_pos for led positions
 * 
 * Lighting areas start at position 0 and will light up the 3 nearest LEDs.
 * By strategically positioning led 0 at the top, this avoids lighting areas
 * from suddenly appeaering.
 **/

#define TURBO_LIGHTS_CLAMP 0.1f
#define TURBO_LIGHTS_THRESHOLD 0.05f
#define TURBO_LIGHTS_DECAY 0.003f
#define TURBO_LIGHTS_VEL 0.5f
#define TURBO_LIGHTS_MAX 16.0f
#define TURBO_LIGHTS_FADE 16
#define TURBO_LIGHTS_FADE_VEL 0.0625f
#define EPS 0.0001f

#define TURBO_LIGHTS_OFFSET ((SW_GPIO_SIZE - 2) * 2)
#define TURBO_LIGHTS_N (WS2812B_LED_SIZE - TURBO_LIGHTS_OFFSET)

int i_clamp(int d, int min, int max) {
  const int t = d < min ? min : d;
  return t > max ? max : t;
}

float f_clamp(float d, float min, float max) {
  const float t = d < min ? min : d;
  return t > max ? max : t;
}

float f_one_mod(float d, float mod) {
  const float t = d < 0 ? d + mod : d;
  return t > mod ? t - mod : t;
}

float f_abs(float d) {
  return d < 0 ? -d : d;
}

uint32_t turbo_prev_enc_val[ENC_GPIO_SIZE];
float turbo_cur_enc_val[ENC_GPIO_SIZE];
float turbo_lights_pos[ENC_GPIO_SIZE];
float turbo_lights_brightness[ENC_GPIO_SIZE];

float turbo_led_base_pos[TURBO_LIGHTS_N] = {
  // https://cdn.discordapp.com/attachments/870171955175759882/1046955855448375406/Pocket-SDVX-Pico-v5c.png
  // 4x bottom right, top to bottom
  4, 4.66f, 5.33f, 6,
  // 1x right knob
  3,
  // 3x top right, top to bottom
  2, 2, 2,
  // 3x top left, top to bottom
  14, 14, 14,
  // 1x left knob
  13,
  // 4x bottom left, top to bottom
  12, 11.33f, 10.66f, 10,
  // 2x bottom logo
  8, 8
};
float turbo_start_pos[ENC_GPIO_SIZE] = {13, 3};

void turbocharger_color_cycle(uint32_t unused) {
  // calculate turbocharger vars
  for (int i = 0; i < ENC_GPIO_SIZE; i++) {
    int enc_delta = (enc_val[i] - turbo_prev_enc_val[i]) * (ENC_REV[i] ? -1 : 1);
    turbo_prev_enc_val[i] = enc_val[i];
    turbo_cur_enc_val[i] = f_clamp(turbo_cur_enc_val[i] + (float)(enc_delta) / ENC_PULSE, -TURBO_LIGHTS_CLAMP, TURBO_LIGHTS_CLAMP);

    if (turbo_cur_enc_val[i] < -TURBO_LIGHTS_THRESHOLD) {
      turbo_lights_pos[i] += TURBO_LIGHTS_VEL;
      turbo_lights_brightness[i] = 1.0f;
    } else if (turbo_cur_enc_val[i] > TURBO_LIGHTS_THRESHOLD) {
      turbo_lights_pos[i] -= TURBO_LIGHTS_VEL;
      turbo_lights_brightness[i] = 1.0f;
    } else {
      turbo_lights_brightness[i] = f_clamp(turbo_lights_brightness[i] - TURBO_LIGHTS_FADE_VEL, 0.0f, 1.0f);
      if (turbo_lights_brightness[i] < EPS) {
        turbo_lights_pos[i] = turbo_start_pos[i];
      }

      if (turbo_cur_enc_val[i] < -EPS) {
        turbo_lights_pos[i] += TURBO_LIGHTS_VEL * turbo_lights_brightness[i];
      } else if (turbo_cur_enc_val[i] > EPS) {
        turbo_lights_pos[i] -= TURBO_LIGHTS_VEL * turbo_lights_brightness[i];
      }
    }

    turbo_lights_pos[i] = f_one_mod(turbo_lights_pos[i], TURBO_LIGHTS_MAX);

    if (turbo_cur_enc_val[i] < -TURBO_LIGHTS_DECAY) {
      turbo_cur_enc_val[i] += TURBO_LIGHTS_DECAY;
    } else if (turbo_cur_enc_val[i] > TURBO_LIGHTS_DECAY) {
      turbo_cur_enc_val[i] -= TURBO_LIGHTS_DECAY;
    }
  }

  // base code from color cycle v5
  uint64_t latest_switch_ts = sw_timestamp[0];
  for (int i = 1; i < SW_GPIO_SIZE; i++)
    if (latest_switch_ts < sw_timestamp[i]) latest_switch_ts = sw_timestamp[i];
  if (time_us_64() > latest_switch_ts + timeout_us) {  // timed out
    for (int i = 0; i < WS2812B_LED_SIZE; i++) ws2812b_data[i] = COLOR_BLACK;
  } else {
    /* Switches */
    for (int i = 0; i < SW_GPIO_SIZE - 3; i++) {
      if (time_us_64() - reactive_timeout_timestamp >= REACTIVE_TIMEOUT_MAX) {
        if ((report.buttons >> i) % 2 == 1) {
          ws2812b_data[2 * i + 2] = SW_COLORS[i + 1];
        } else {
          ws2812b_data[2 * i + 2] = COLOR_BLACK;
        }
      } else {
        if (lights_report.lights.buttons[i] == 0) {
          ws2812b_data[2 * i + 2] = COLOR_BLACK;
        } else {
          ws2812b_data[2 * i + 2] = SW_COLORS[i + 1];
        }
      }
    }

    /* start button sw_val index is offset by two with respect to LED_GPIO */
    if (time_us_64() - reactive_timeout_timestamp >= REACTIVE_TIMEOUT_MAX) {
      if ((report.buttons >> (SW_GPIO_SIZE - 1)) % 2 == 1) {
        ws2812b_data[0] = SW_COLORS[0];
      } else {
        ws2812b_data[0] = COLOR_BLACK;
      }
    } else {
      if (lights_report.lights.buttons[SW_GPIO_SIZE - 3] == 0) {
        ws2812b_data[0] = COLOR_BLACK;
      } else {
        ws2812b_data[0] = SW_COLORS[0];
      }
    }
    /* Switch Labels */
    for (int i = 0; i < SW_GPIO_SIZE - 2; i++) {
      ws2812b_data[2 * i + 1] = SW_LABEL_COLORS[i];
    }

    /* peripheral turbocharger leds */
    for (int i = 0; i < TURBO_LIGHTS_N; i++) {
      float pos = turbo_led_base_pos[i];
      float l_strength = (1.0f - f_clamp(f_abs(turbo_lights_pos[0] - pos), 0.0f, 2.0f) / 2) * turbo_lights_brightness[0];
      float r_strength = (1.0f - f_clamp(f_abs(turbo_lights_pos[1] - pos), 0.0f, 2.0f) / 2) * turbo_lights_brightness[1];

      ws2812b_data[TURBO_LIGHTS_OFFSET + i].r = i_clamp(l_strength * 70 + r_strength * 250, 0, 255);
      ws2812b_data[TURBO_LIGHTS_OFFSET + i].g = i_clamp(l_strength * 230 + r_strength * 60, 0, 255);
      ws2812b_data[TURBO_LIGHTS_OFFSET + i].b = i_clamp(l_strength * 250 + r_strength * 200, 0, 255);
    }
  }

  // put pixels
  for (int i = 0; i < WS2812B_LED_SIZE; ++i) {
    put_pixel(
        urgb_u32(ws2812b_data[i].r, ws2812b_data[i].g, ws2812b_data[i].b));
  }
}
