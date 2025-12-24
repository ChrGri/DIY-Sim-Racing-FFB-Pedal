#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadPedalState_Basic {
  uint16_t pedalPosition_u16;
  uint16_t pedalForce_u16;
  uint16_t joystickOutput_u16;
  uint8_t error_code_u8;
  uint8_t pedalFirmwareVersion_u8[3];
  uint8_t servoStatus;
  uint8_t pedalStatus;
  uint8_t pedalContrlBoardType;
};