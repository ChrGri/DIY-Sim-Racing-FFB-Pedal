#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadFooter {
  // To check if structure is valid
  uint16_t checkSum;

  // end of frame bytes
  uint8_t enfOfFrame0_u8;
  uint8_t enfOfFrame1_u8;
};