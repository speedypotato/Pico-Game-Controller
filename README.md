# Pico-Game-Controller

WIP code for a rhythm game controller using a Raspberry Pi Pico. Intended for SDVX but is capable of handling 11 buttons, 11 LEDs, and 2 encoders.

Currently working/fixed:

- Gamepad mode - untested on EAC - default boot mode
- Keyboard & Mouse Mode - limited to 6KRO - hold btn-a to enter kb mode
- HID LEDs with Reactive LED fallback
- sdvx/iidx spoof - untested on EAC
- 1000hz polling
- Switch and LED pins are now staggered for easier wiring
- Fix 0-~71% encoder rollover in gamepad mode, uint32 max val isn't divisible evenly by ppr\*4 for joystick - thanks friends

TODO:

- Store last mode in flash memory https://www.raspberrypi.org/forums/viewtopic.php?t=305570
- nkro
- debounce
- ws2812b rgb on second core?
- split input updating into pi pico's second core?

How to Use:

- For basic flashing, see README in build_uf2
- Otherwise, setup the C++ environment for the Pi Pico as per https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf
- Build pico-examples directory once to ensure all the tinyusb and other libraries are there. You might have to move the pico-sdk folder into pico-examples/build for it to play nice.
- Move pico-sdk back outside to the same level directory as Pico-Game-Controller.
- Open Pico-Game-Controller in VSCode(assuming this is setup for the Pi Pico) and see if everything builds.
- Tweakable parameters: Pinout, bindings in pico_game_controller.c, USB device reporting as SDVX/IIDX(con_mode) in usb_descriptors.c
