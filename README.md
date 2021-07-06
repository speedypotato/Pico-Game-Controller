# Pico-Game-Controller

WIP code for a rhythm game controller using a Raspberry Pi Pico.  Intended for SDVX but is capable of handling 11 buttons, 11 LEDs, and 2 encoders.

Currently working/fixed:
- Gamepad mode - untested on EAC - default boot mode
- Keyboard & Mouse Mode - limited to 6KRO - hold btn-a to enter kb mode
- Reactive LEDs (KB/Mouse Mode)
- sdvx/iidx spoof - untested on EAC
- 1000hz polling
- Fix 0-~71% encoder rollover in gamepad mode, uint32 max val isn't divisible evenly by ppr*4 for joystick - thanks friends

TODO:
- HID leds - create led func to use incoming light data
- Reactive LED fallback if no HID info
- Store last mode in flash memory https://www.raspberrypi.org/forums/viewtopic.php?t=305570
- debounce
- nkro

Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
