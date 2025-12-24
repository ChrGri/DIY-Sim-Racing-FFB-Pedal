#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadBridgeState{
  uint8_t unassignedPedalCount;
  uint8_t Pedal_availability[3];
  uint8_t Bridge_action; // 0=none, 1=enable pairing 2=Restart 3=download mode
  uint8_t Bridge_firmware_version_u8[3];
  int32_t Pedal_RSSI_Realtime[3];
  uint8_t macAddressDetected[18];
};