#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadHeader{
  // start of frame indicator
  uint8_t startOfFrame0_u8;
  uint8_t startOfFrame1_u8;

  // structure identification via payload
  uint8_t payloadType;

  // variable to check if structure at receiver matched version from transmitter
  uint8_t version;

  // store to EEPROM flag
  uint8_t storeToEeprom;

  // pedal tag
  uint8_t PedalTag;
};