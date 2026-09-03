#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadRudderState
{
  uint16_t pedalPosition_u16;
  float pedalPositionRatio_fl32;
  float pedalForce_N_fl32;
  uint32_t sendTimestamp_ms;
  uint32_t echoTimestamp_ms;
} PayloadRudderState_t;
