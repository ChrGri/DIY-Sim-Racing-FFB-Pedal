#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadRudderState {
  uint16_t pedal_position;
  float pedal_position_ratio;
};