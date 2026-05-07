#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadHidMessage
{
  uint8_t payloadType_u8;
  uint8_t magicKey1_u8;
  uint8_t magicKey2_u8;
  uint8_t length_u8;
  char text_ac[235];
} PayloadHidMessage_t;