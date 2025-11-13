#pragma once
#include "Arduino.h"
#include <string>
#include "Adafruit_TinyUSB.h"
//extern Adafruit_USBD_HID usb_hid;
// HID Report Descriptor for Bridge, 6 axis
uint8_t const desc_hid_report[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04, // Usage (Joystick)
    0xA1, 0x01, // Collection (Application)

    // Define six 16-bit axes (X, Y, Z, Rx, Ry, Rz)
    0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x33,       //   Usage (Rx)
    0x09, 0x34,       //   Usage (Ry)
    0x09, 0x35,       //   Usage (Rz)
    0x16, 0x00, 0x80, //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F, //   Logical Maximum (32767)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x06,       //   Report Count (6) 
    0x81, 0x02,       //   Input (Data,Var,Abs)

    0xC0, // End Collection
};

class TinyusbJoystick {
private:
    static const uint16_t JOYSTICK_MIN_VALUE = INT16_MIN;
    static const uint16_t JOYSTICK_MAX_VALUE = INT16_MAX;
    static const uint16_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;
    // USB HID object
    // Report payload for the two axes
    typedef struct
    {
        int16_t x;
        int16_t y;
        int16_t z;
        int16_t rx;
        int16_t ry;
        int16_t rz;
    } hid_report_t;
    Adafruit_USBD_HID usb_hid;
    hid_report_t hid_report = {0};

public:
  TinyusbJoystick();
  bool IsReady();
  void Begin();
  void SetXAxis(int16_t value);
  void SetYAxis(int16_t value);
  void SetZAxis(int16_t value);
  void SetRx(int16_t value);
  void SetRy(int16_t value);
  void SetRz(int16_t value);
  void SendState();
};