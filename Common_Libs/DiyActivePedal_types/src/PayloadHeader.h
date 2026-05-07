#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadHeader
{
  // start of frame indicator
  uint8_t startOfFrame0_u8;
  uint8_t startOfFrame1_u8;

  // structure identification via payload
  uint8_t payloadType_u8;

  // variable to check if structure at receiver matched version from transmitter
  uint8_t version_u8;

  // store to EEPROM flag
  uint8_t storeToEeprom_u8;

  // pedal tag
  uint8_t pedalTag_u8;
} PayloadHeader_t;