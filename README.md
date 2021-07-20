# Pico-Game-Controller

WIP code for a rhythm game controller using a Raspberry Pi Pico.  Intended for SDVX but is capable of handling 11 buttons, 11 LEDs, and 2 encoders.

Currently working/fixed:
- Gamepad mode - untested on EAC - default boot mode
- Keyboard & Mouse Mode - limited to 6KRO - hold btn-a to enter kb mode
- HID LEDs with Reactive LED fallback - untested
- sdvx/iidx spoof - untested on EAC
- 1000hz polling
- Fix 0-~71% encoder rollover in gamepad mode, uint32 max val isn't divisible evenly by ppr*4 for joystick - thanks friends
- Switch and LED pins are now staggered for easier wiring

TODO:
- Store last mode in flash memory https://www.raspberrypi.org/forums/viewtopic.php?t=305570
- nkro
- debounce
- split input updating into pi pico's second core?
