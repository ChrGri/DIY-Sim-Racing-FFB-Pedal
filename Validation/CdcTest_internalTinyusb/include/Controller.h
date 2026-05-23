#pragma once

#include "Arduino.h"
// #include "Main.h"

static const uint16_t s_JOYSTICK_MIN_VALUE_U16 = 0U;
static const uint16_t s_JOYSTICK_MAX_VALUE_U16 = UINT16_MAX;
static const uint16_t s_JOYSTICK_RANGE_U16 = s_JOYSTICK_MAX_VALUE_U16 - s_JOYSTICK_MIN_VALUE_U16;

uint16_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput_u8);



#ifdef USB_JOYSTICK
  void SetupController_USB(uint8_t pedal_ID);
  bool IsControllerReady();
  void SetControllerOutputValue(uint16_t value);
  void SetupController();
#endif