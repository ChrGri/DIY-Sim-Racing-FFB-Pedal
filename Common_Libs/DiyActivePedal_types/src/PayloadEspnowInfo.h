#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadESPNowInfo{
  uint8_t _deviceID;
  uint8_t occupy;
  uint8_t occupy2;

};