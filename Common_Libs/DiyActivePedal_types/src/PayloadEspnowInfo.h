#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadEspnowInfo
{
  uint8_t deviceId_u8;
  uint8_t occupy_u8;
  uint8_t occupy2_u8;

} PayloadEspnowInfo_t;