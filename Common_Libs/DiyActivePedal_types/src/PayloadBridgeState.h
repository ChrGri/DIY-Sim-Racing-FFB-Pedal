#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadBridgeState
{
  uint8_t unassignedPedalCount_u8;
  uint8_t pedalAvailability_au8[3];
  uint8_t bridgeAction_u8; // 0=none, 1=enable pairing 2=Restart 3=download mode
  uint8_t bridgeFirmwareVersion_au8[3];
  int32_t pedalRssiRealtime_ai32[3];
  uint8_t macAddressDetected_au8[18];
} PayloadBridgeState_t;