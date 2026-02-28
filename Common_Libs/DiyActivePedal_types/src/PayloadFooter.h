#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadFooter
{
  // To check if structure is valid
  uint16_t checkSum_u16;

  // end of frame bytes
  uint8_t enfOfFrame0_u8;
  uint8_t enfOfFrame1_u8;
} PayloadFooter_t;