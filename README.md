# Pico-Game-Controller

WIP code for a rhythm game controller using a Raspberry Pi Pico. Intended for SDVX or IIDX but is capable of handling 11 buttons, 10 LEDs, 1 WS2812B RGB strip, and 2 encoders.

Currently working/fixed:

- Gamepad mode - default boot mode
- NKRO Keyboard & Mouse Mode - hold first button to enter kb mode
- HID LEDs with Reactive LED fallback
- ws2812b rgb on second core
- 2 ws2812b hid descriptor zones
- sdvx/iidx spoof - Tested on EAC
- 1000hz polling
- Reversable Encoders with debouncing
- Switch and LED pins are now staggered for easier wiring
- Fix 0-~71% encoder rollover in gamepad mode, uint32 max val isn't divisible evenly by ppr\*4 for joystick - thanks friends

TODO:

- store configuration settings in a text file? consider implementing littlefs https://github.com/littlefs-project/littlefs https://www.raspberrypi.org/forums/viewtopic.php?t=313009 https://www.raspberrypi.org/forums/viewtopic.php?p=1894014#p1894014
- Store last mode in flash memory https://www.raspberrypi.org/forums/viewtopic.php?t=305570
- debounce on switches

How to Use:

- For basic flashing, see README in build_uf2
- Otherwise, setup the C++ environment for the Pi Pico as per https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf
- Build pico-examples directory once to ensure all the tinyusb and other libraries are there. You might have to move the pico-sdk folder into pico-examples/build for it to play nice.
- Move pico-sdk back outside to the same level directory as Pico-Game-Controller.
- Open Pico-Game-Controller in VSCode(assuming this is setup for the Pi Pico) and see if everything builds.
- Tweakable parameters: Pinout, bindings in pico_game_controller.c, USB device reporting as SDVX/IIDX(con_mode) in usb_descriptors.c

Thanks to:

- Everyone in the Cons & Stuff Discord for providing near instant support to questions.
- https://github.com/hathach/tinyusb/tree/master/examples/device/hid_composite
- https://github.com/mdxtinkernick/pico_encoders for encoders which performed better than both interrupts and polling.
- My SE buddies who helped come up with a solution for the encoder rollover edge case scenario.
- https://github.com/Drewol/rp2040-gamecon for usb gamepad descriptor info.
- https://github.com/veroxzik/arduino-konami-spoof for konami spoof usb descriptor info.
- https://github.com/veroxzik/roxy-firmware for nkro descriptor and logic info.
- KyubiFox for bringing clkdiv to my attention for encoder debouncing
