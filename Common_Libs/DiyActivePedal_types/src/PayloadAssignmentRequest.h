#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadAssignmentRequest
{
  uint8_t assignmentAction_u8;
  uint8_t assignmentState_u8;
  uint8_t macAddress_au8[6];
} PayloadAssignmentRequest_t;