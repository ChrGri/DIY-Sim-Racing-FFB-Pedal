#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadAssignmentRequest{
  uint8_t assignmentAction;
  uint8_t assignmentState;
  uint8_t macAddress[6];
};