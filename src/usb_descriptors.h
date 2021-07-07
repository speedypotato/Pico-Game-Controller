#include "common/tusb_common.h"
#include "device/usbd.h"

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

enum {
  REPORT_ID_GAMEPAD = 1,
  REPORT_ID_LIGHTS,
  REPORT_ID_KEYBOARD,
  REPORT_ID_MOUSE,
};

#endif /* USB_DESCRIPTORS_H_ */

// Gamepad Report Descriptor Template - Based off Drewol/rp2040-gamecon
// 11 Button Map | X | Y
#define GAMECON_REPORT_DESC_GAMEPAD(...)                                       \
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),                                      \
      HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),                                    \
      HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
      __VA_ARGS__ HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON), HID_USAGE_MIN(1),     \
      HID_USAGE_MAX(11), /*11 buttons*/                                        \
      HID_LOGICAL_MIN(0), HID_LOGICAL_MAX(1), HID_REPORT_COUNT(11),            \
      HID_REPORT_SIZE(1), HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
      HID_REPORT_COUNT(1), HID_REPORT_SIZE(5), /*Padding*/                     \
      HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
      HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), HID_LOGICAL_MIN(0x00),           \
      HID_LOGICAL_MAX_N(0x00ff, 2),                                            \
      HID_USAGE(HID_USAGE_DESKTOP_X), /*Joystick*/                             \
      HID_USAGE(HID_USAGE_DESKTOP_Y), HID_REPORT_COUNT(2), HID_REPORT_SIZE(8), \
      HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), HID_COLLECTION_END

// 11 Light Map
#define GAMECON_REPORT_DESC_LIGHTS(...)                                        \
  HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), HID_USAGE(0x00),                     \
      HID_COLLECTION(HID_COLLECTION_APPLICATION),                              \
      __VA_ARGS__ HID_REPORT_COUNT(11), /*11 button lights*/                   \
      HID_REPORT_SIZE(8), HID_LOGICAL_MIN(0x00), HID_LOGICAL_MAX_N(0x00ff, 2), \
      HID_USAGE_PAGE(HID_USAGE_PAGE_ORDINAL), HID_USAGE_MIN(1),                \
      HID_USAGE_MAX(11), HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),   \
      HID_REPORT_COUNT(1), HID_REPORT_SIZE(40), /*Padding*/                    \
      HID_INPUT(HID_CONSTANT | HID_VARIABLE | HID_ABSOLUTE),                   \
      HID_COLLECTION_END
