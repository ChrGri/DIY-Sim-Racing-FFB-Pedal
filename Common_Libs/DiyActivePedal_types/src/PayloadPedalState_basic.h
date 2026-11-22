#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadPedalStateBasic
{
  uint16_t pedalPosition_u16;
  uint16_t pedalForce_u16;
  uint16_t joystickOutput_u16;
  uint8_t errorCode_u8;
  uint8_t pedalFirmwareVersion_au8[3];
  uint8_t servoStatus_u8;
  uint8_t pedalStatus_u8;
  uint8_t pedalControlBoardType_u8;
  uint8_t rudderSyncDelay_ms;
  uint16_t servoAlarmCode_u16; // active iSV57 alarm (lower 12 bits, 0 = none)
} PayloadPedalStateBasic_t;
