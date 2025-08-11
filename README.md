# Pico Game Controller (RP2040)

Raspberry Pi Pico firmware that can act as a joystick (gamepad) or NKRO keyboard/mouse, designed for rhythm controllers (SDVX/IIDX). It supports:

- 11 switches (with debouncing)
- 10 discrete LEDs for the switches
- 1 WS2812B addressable LED strip
- 2 rotary encoders

Pre-configured builds for Pocket SDVX Pico / Pocket IIDX are in branches `release/pocket-sdvx-pico` and `release/pocket-iidx`.

![Pocket SDVX Pico](demo.gif)

## Features

- Gamepad mode (default) with 1000 Hz polling
- NKRO Keyboard + Mouse mode (hold first button at boot)
- HID-controlled switch LEDs with reactive fallback
- WS2812B RGB effects rendered on core 1 (optional)
- Two encoder inputs with direction reversal and debouncing
- Tunable behavior via a simple HID Config Tool (Python GUI)

## Boot-time options (hold a button while plugging in)

- Hold SW_GPIO[0] (first switch) → start in NKRO Keyboard/Mouse mode
- Hold SW_GPIO[1] (second switch) → start with Turbocharger RGB effect
- Hold SW_GPIO[8] (ninth switch) → disable RGB (don’t launch core 1)

Pin maps and sizes live in `src/controller_config.h`.

## Runtime configuration (over HID Feature report)

Use the Python GUI at `tools/effect_selector.py` to view and set:

- RGB Effect (multiple effects available) and Brightness (0–255)
- Encoder PPR (1–4000)
- Mouse sensitivity (1–50)
- Encoder debouncing (on/off; applied on next boot)
- WS2812B LED count and zones (persisted; applied on reboot)

Notes:

- LED count is enforced at output time up to the compiled buffer size.
- Zones are part of the USB descriptor and take effect after reboot; they are persisted for the next session/firmware.

## Build and flash (VS Code tasks on Windows)

Prereqs: Raspberry Pi Pico SDK installed and VS Code Pico extension. This repo already contains tasks.

- Build: run the “Compile Project” task. Artifacts are generated in `build/src/` and a UF2 is copied to `build_uf2/`.
- Run (picotool RAM load): use the “Run Project” task.
- Flash (OpenOCD, SWD probe): use the “Flash” task.
- Rescue reset tasks available for recovery.

You can also use `flash.ps1` which builds and copies the UF2 to the `RPI-RP2` drive automatically when the Pico is in BOOTSEL mode.

## HID Config Tool (Python)

The GUI is in `tools/effect_selector.py`.

- Requirements: `pip install -r tools/requirements.txt` (needs `hidapi`).
- Run with `python tools/effect_selector.py` (or use `run_config_tool.ps1`).
- Use “Refresh” to read current settings; “Apply” writes changes to the device.
- “Reboot to BOOTSEL” tells the device to jump into UF2 bootloader (for flashing).

Troubleshooting:

- If it shows “Not connected”, check the device is enumerated (use “Debug HID”).
- If RGB zones/size don’t appear to change instantly, reboot the device—those are applied on startup.

## HID Config Tool (compiled EXE)

You can build a standalone Windows executable using PyInstaller.

- Build with:
  - PowerShell: `./tools/build_config_tool.ps1`
  - Output: `dist/PicoGameConfig.exe` (single file). The launcher `run_config_tool.ps1` will prefer this EXE if present.
- First run will create a local `.venv` and install `hidapi` and `pyinstaller` automatically.

Note: The compiled EXE uses the same VID/PID and HID feature reports as the Python script; no functional differences.

## Firmware architecture overview

- Core 0: USB HID + input scanning + mode/LED logic. See `src/pico_game_controller.c`.
- Core 1: WS2812B RGB rendering (every ~5 ms), launched only if RGB isn’t disabled at boot.
- PIO/DMA:
  - `encoders.pio` via DMA updates encoder values.
  - `ws2812.pio` drives the LED strip.

HID Report IDs (see `src/usb_descriptors.h`):

- 1: Joystick (gamepad)
- 2: Lights (switch LEDs + HID RGB color zones)
- 3: NKRO Keyboard
- 4: Mouse
- 5: Config (vendor-specific Feature report)

Config Feature Report (Report ID 5): 8-byte payload `[cmd, arg0..arg6]`

- 0x00 (GET basic): returns `[status, effect_id, brightness, ...]`
- 0x20 (GET extended): returns `[status, enc_ppr(lo,hi), mouse_sens, enc_debounce, ws_size(lo,hi), ws_zones]`
- 0x01 (SET_EFFECT), 0x02 (SET_BRIGHTNESS)
- 0x10 (SET_ENCODER_PPR), 0x11 (SET_MOUSE_SENS), 0x12 (SET_ENC_DEBOUNCE)
- 0x13 (SET_WS_PARAMS: size LE16, zones)
- 0x03 (REBOOT_TO_BOOTSEL)

## Pins and sizes (defaults)

All sizes and GPIOs are defined in `src/controller_config.h`. Defaults include:

- SW_GPIO_SIZE = 11, LED_GPIO_SIZE = 10, ENC_GPIO_SIZE = 2
- ENC_PPR = 600, MOUSE_SENS = 1, ENC_DEBOUNCE = false
- WS2812B_LED_SIZE = 10, WS2812B_LED_ZONES = 2

At runtime, the device uses persisted values stored in flash (effect, brightness, encoder/mouse, debounce flag, and WS2812B parameters). Some settings apply immediately; others require reboot (see Runtime configuration).

## Thanks

- Cons & Stuff Discord for constant help
- TinyUSB examples: https://github.com/hathach/tinyusb/tree/master/examples/device/hid_composite
- Encoders: https://github.com/mdxtinkernick/pico_encoders
- USB descriptors: https://github.com/Drewol/rp2040-gamecon
- Konami spoof info: https://github.com/veroxzik/arduino-konami-spoof
- NKRO details: https://github.com/veroxzik/roxy-firmware
- KyubiFox for clkdiv/debounce insights
- 4yn for Turbocharger lighting
- SushiRemover for alternate debounce mode
