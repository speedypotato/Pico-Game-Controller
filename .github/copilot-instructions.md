# Pico Game Controller — AI Coding Instructions

Goal: Make fast, correct edits to this RP2040 firmware by following the repo’s patterns and workflows.

## Architecture (what runs where)

- Core 0: USB HID device task + input scan + mode/LED logic. See `src/pico_game_controller.c` (main, `joy_mode()`, `key_mode()`, `update_lights()`).
- Core 1: WS2812B renderer (`core1_entry()` ~5 ms). Launched only if RGB isn’t disabled at boot.
- PIO/DMA: `encoders.pio` → DMA into `enc_val[]`; `ws2812.pio` at 800 kHz for LED output. Headers autogen in `build/src/` as `encoders.pio.h`, `ws2812.pio.h`.

## Boot-time behavior (GPIO pull-ups; pressed = low)

- Hold `SW_GPIO[0]` → NKRO keyboard/mouse; default is joystick.
- Hold `SW_GPIO[1]` → start with Turbocharger effect; default is Color Cycle.
- Hold `SW_GPIO[8]` → disable RGB (don’t launch core 1).
  Pins and sizes are defined in `src/controller_config.h` (keep arrays aligned to `*_SIZE`).

## HID and runtime config

- Report IDs (`src/usb_descriptors.h`): 1=Joystick, 2=Lights, 3=NKRO, 4=Mouse, 5=Config (Feature report). Descriptor lengths depend on `SW_GPIO_SIZE`, `LED_GPIO_SIZE`, `WS2812B_LED_ZONES`.
- Lights: OUT reports fill `lights_report`; if idle for `REACTIVE_TIMEOUT_MAX` (1s), `update_lights()` reverts to button-reactive LEDs.
- Encoders → joystick: value wraps by PPR×4, scaled to 0–255.
- Config Feature report (RID 5), 8-byte `[cmd, arg0..arg6]`:
  - GET basic (0x00): `[status, effect_id, brightness, …]`
  - GET extended (0x20): `[status, enc_ppr(lo,hi), mouse_sens, enc_debounce, ws_led_size(lo,hi), ws_led_zones]`
  - SETs: `0x01=EFFECT`, `0x02=BRIGHTNESS`, `0x10=ENC_PPR`, `0x11=MOUSE_SENS`, `0x12=ENC_DEBOUNCE`, `0x13=WS_PARAMS(size,zones)`, `0x03=REBOOT_TO_BOOTSEL`.
- Persistent settings (`load_settings()/save_settings()`): stored in last flash sector via `settings_t` (effect, brightness, enc/mouse params, WS2812B params). Some apply immediately; others (debounce, WS zones) take effect after reboot.
- Runtime variables: `g_enc_ppr/g_enc_pulse`, `g_mouse_sens`, `g_enc_debounce`, `g_brightness`, `g_ws_led_size_cfg/g_ws_led_zones_cfg`. `show()` caps output count to `g_ws_led_size_cfg` (≤ compiled `WS2812B_LED_SIZE`).

## Project conventions

- Debounce algos: add `uint16_t my_algo()` in `src/debounce/`, include in `debounce_include.h`, select with `debounce_mode = &my_algo;` in `init()`.
- RGB effects: add `void my_effect(uint32_t counter, bool hid_mode)` in `src/rgb/`, include in `rgb_include.h`, map in `set_effect_by_id()`; write colors to global `leds[]`, then `show()` flushes.
- Single-writer globals across cores: core 1 is the only renderer; core 0 updates mode/state and publishes `g_buttons`/`hid_rgb`.
- Switches use pull-ups: pressed is `!gpio_get(SW_GPIO[i])`.

## Build, flash, debug (Windows)

- VS Code tasks: “Compile Project” (ninja → `build/`), “Run Project” (picotool RAM load), “Flash” (OpenOCD SWD). Rescue/reset tasks available.
- Artifacts: UF2 at `build/src/Pico_Game_Controller.uf2`, also copied to `build_uf2/Pico_Game_Controller.uf2`.
- PowerShell helper: `./flash.ps1 [-SkipBuild] [-Timeout <sec>]` builds, reboots to BOOTSEL, copies UF2 to `RPI-RP2`.
- Config GUI: `tools/effect_selector.py` (needs `hidapi`). Use `run_config_tool.ps1` to launch.

## Files you’ll touch most

- `src/pico_game_controller.c`: main loop, HID callbacks, DMA IRQ, settings, boot-time logic, effect selection.
- `src/controller_config.h`: pin maps and sizes; keep lengths in sync with `*_SIZE` and descriptors.
- `src/usb_descriptors.h`: HID report layouts; sizes depend on config constants.
- `src/rgb/*`: effects; `ws2812b_util.c` has palette/color helpers.
- `src/debounce/*`: eager/deferred examples.

## Gotchas

- Do not change USB descriptor structure/IDs or boot semantics lightly (affects EAC/Konami spoofing and host compatibility).
- Keep arrays and loops aligned to `*_SIZE`; mismatches cause descriptor/runtime bugs.
- PIO headers are generated in `build/src/`; include them as `encoders.pio.h` / `ws2812.pio.h`.
