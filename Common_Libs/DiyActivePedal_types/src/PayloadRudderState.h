#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadRudderState
{
  uint16_t pedalPosition_u16;
  float pedalPositionRatio_fl32;
} PayloadRudderState_t;