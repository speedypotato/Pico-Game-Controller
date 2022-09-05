typedef struct {
  uint8_t r, g, b;
} RGB_t;

typedef struct {
  uint16_t buttons;
  uint8_t joy0;
  uint8_t joy1;
} report_t;

typedef struct {
  uint8_t buttons[LED_GPIO_SIZE];
  RGB_t rgb[WS2812B_LED_ZONES];
} lights_t;

typedef union {
  lights_t lights;
  uint8_t raw[LED_GPIO_SIZE + WS2812B_LED_ZONES * 3];
} lights_report_t;