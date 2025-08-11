/*
 * ws2812b utility class with advanced palette system inspired by cpt-city
 * @author SpeedyPotato, Renard
 */
#include "ws2812.pio.h"

/**
 * WS2812B RGB Format Helper
 **/
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Palette definitions - simple integer constants for easier use
#define PALETTE_RAINBOW 0
#define PALETTE_OCEAN 1
#define PALETTE_FIRE 2
#define PALETTE_PLASMA 3
#define PALETTE_VIRIDIS 4
#define PALETTE_SUNSET 5
#define PALETTE_ARCTIC 6
#define PALETTE_EARTH 7
#define PALETTE_NEON 8
#define PALETTE_COUNT 9

// Current active palette (can be changed at runtime)
static int current_palette = PALETTE_SUNSET;

/**
 * Linear interpolation between two RGB values
 * @param color1 First color (RGB as single uint32_t)
 * @param color2 Second color (RGB as single uint32_t)
 * @param t Interpolation factor (0.0 to 1.0)
 */
static inline uint32_t lerp_color(uint32_t color1, uint32_t color2, float t)
{
  uint8_t r1 = (color1 >> 8) & 0xFF;
  uint8_t g1 = (color1 >> 16) & 0xFF;
  uint8_t b1 = color1 & 0xFF;

  uint8_t r2 = (color2 >> 8) & 0xFF;
  uint8_t g2 = (color2 >> 16) & 0xFF;
  uint8_t b2 = color2 & 0xFF;

  uint8_t r = (uint8_t)(r1 + t * (r2 - r1));
  uint8_t g = (uint8_t)(g1 + t * (g2 - g1));
  uint8_t b = (uint8_t)(b1 + t * (b2 - b1));

  return urgb_u32(r, g, b);
}

/**
 * Get color from specific palette
 * @param palette_type The palette to use (0-8)
 * @param position Position in palette (0.0 to 1.0, wraps around for circular)
 */
static inline uint32_t get_palette_color(int palette_type, float position)
{
  // Ensure position is in valid range and handle circular wrapping
  while (position < 0.0f)
    position += 1.0f;
  while (position >= 1.0f)
    position -= 1.0f;

  switch (palette_type)
  {
  case PALETTE_OCEAN:
  {
    // Ocean palette: deep blue -> cyan -> light blue -> white (circular)
    uint32_t colors[] = {
        urgb_u32(0, 0, 80),      // Deep blue
        urgb_u32(0, 50, 150),    // Ocean blue
        urgb_u32(0, 150, 200),   // Light blue
        urgb_u32(50, 200, 255),  // Cyan
        urgb_u32(150, 230, 255), // Light cyan
        urgb_u32(200, 245, 255)  // Almost white
    };
    int segments = 6;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_FIRE:
  {
    // Fire palette: black -> red -> orange -> yellow -> white
    uint32_t colors[] = {
        urgb_u32(0, 0, 0),       // Black
        urgb_u32(80, 0, 0),      // Dark red
        urgb_u32(150, 0, 0),     // Red
        urgb_u32(255, 50, 0),    // Red-orange
        urgb_u32(255, 150, 0),   // Orange
        urgb_u32(255, 255, 100), // Yellow
        urgb_u32(255, 255, 255)  // White
    };
    int segments = 7;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_PLASMA:
  {
    // Plasma palette: purple -> magenta -> orange -> yellow
    uint32_t colors[] = {
        urgb_u32(80, 0, 80),    // Purple
        urgb_u32(120, 0, 120),  // Deep purple
        urgb_u32(180, 0, 150),  // Magenta
        urgb_u32(220, 50, 100), // Pink-magenta
        urgb_u32(255, 100, 50), // Orange-red
        urgb_u32(255, 180, 0),  // Orange
        urgb_u32(255, 255, 100) // Yellow
    };
    int segments = 7;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_VIRIDIS:
  {
    // Viridis palette: purple -> blue -> green -> yellow
    uint32_t colors[] = {
        urgb_u32(70, 0, 80),    // Purple
        urgb_u32(50, 50, 140),  // Purple-blue
        urgb_u32(30, 100, 180), // Blue
        urgb_u32(0, 130, 160),  // Blue-green
        urgb_u32(40, 160, 120), // Green
        urgb_u32(120, 200, 80), // Light green
        urgb_u32(200, 220, 50)  // Yellow-green
    };
    int segments = 7;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_SUNSET:
  {
    // Sunset palette: deep blue -> purple -> orange -> yellow
    uint32_t colors[] = {
        urgb_u32(0, 20, 80),    // Deep blue
        urgb_u32(20, 0, 100),   // Purple-blue
        urgb_u32(80, 0, 120),   // Purple
        urgb_u32(150, 50, 100), // Pink-purple
        urgb_u32(200, 100, 50), // Orange-pink
        urgb_u32(255, 150, 0),  // Orange
        urgb_u32(255, 200, 50)  // Yellow-orange
    };
    int segments = 7;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_ARCTIC:
  {
    // Arctic palette: deep blue -> ice blue -> white -> cyan
    uint32_t colors[] = {
        urgb_u32(0, 50, 100),    // Deep blue
        urgb_u32(50, 100, 150),  // Ice blue
        urgb_u32(100, 150, 200), // Light blue
        urgb_u32(180, 220, 255), // Very light blue
        urgb_u32(230, 250, 255), // Almost white
        urgb_u32(200, 255, 255)  // Ice cyan
    };
    int segments = 6;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_EARTH:
  {
    // Earth palette: brown -> green -> blue (topographic)
    uint32_t colors[] = {
        urgb_u32(100, 50, 0),   // Brown
        urgb_u32(150, 100, 50), // Light brown
        urgb_u32(100, 150, 50), // Brown-green
        urgb_u32(50, 200, 100), // Green
        urgb_u32(0, 150, 150),  // Blue-green
        urgb_u32(0, 100, 200),  // Blue
        urgb_u32(50, 50, 255)   // Deep blue
    };
    int segments = 7;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_NEON:
  {
    // Neon cyberpunk palette: electric colors
    uint32_t colors[] = {
        urgb_u32(255, 0, 255), // Magenta
        urgb_u32(255, 0, 128), // Pink
        urgb_u32(255, 50, 0),  // Red-orange
        urgb_u32(255, 255, 0), // Yellow
        urgb_u32(0, 255, 0),   // Green
        urgb_u32(0, 255, 255), // Cyan
        urgb_u32(0, 0, 255),   // Blue
        urgb_u32(128, 0, 255)  // Purple
    };
    int segments = 8;
    float seg_pos = position * segments;
    int seg = (int)seg_pos;
    float t = seg_pos - seg;
    return lerp_color(colors[seg % segments], colors[(seg + 1) % segments], t);
  }

  case PALETTE_RAINBOW:
  default:
  {
    // Original rainbow palette (modified for smooth circular transition)
    if (position < 0.333f)
    {
      float t = position * 3.0f;
      return lerp_color(urgb_u32(255, 0, 0), urgb_u32(0, 255, 0), t); // Red to Green
    }
    else if (position < 0.666f)
    {
      float t = (position - 0.333f) * 3.0f;
      return lerp_color(urgb_u32(0, 255, 0), urgb_u32(0, 0, 255), t); // Green to Blue
    }
    else
    {
      float t = (position - 0.666f) * 3.0f;
      return lerp_color(urgb_u32(0, 0, 255), urgb_u32(255, 0, 0), t); // Blue to Red
    }
  }
  }
}

/**
 * Set the current color palette
 * @param palette The palette to use (0-8)
 */
static inline void set_color_palette(int palette)
{
  if (palette >= 0 && palette < PALETTE_COUNT)
  {
    current_palette = palette;
  }
}

/**
 * 768 Color Wheel Picker with Palette System
 * @param wheel_pos Color value (0-767, cyclical)
 **/
static inline uint32_t color_wheel(uint16_t wheel_pos)
{
  // Convert wheel position to 0.0-1.0 range for palette lookup
  float position = (float)(wheel_pos % 768) / 768.0f;
  return get_palette_color(current_palette, position);
}

/**
 * WS2812B RGB Assignment
 * @param pixel_grb The pixel color to set
 **/
static inline void put_pixel(uint32_t pixel_grb)
{
  pio_sm_put_blocking(pio1, ENC_GPIO_SIZE, pixel_grb << 8u);
}