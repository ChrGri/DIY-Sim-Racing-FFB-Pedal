#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadPedalAction {
  uint8_t triggerAbs_u8;
  //uint8_t resetPedalPos_u8; //1=reset position, 2=restart ESP
  uint8_t system_action_u8; //1=reset position, 2=restart ESP, 3=OTA Enable, 4=enable pairing
  uint8_t startSystemIdentification_u8;
  uint8_t returnPedalConfig_u8;
  uint8_t RPM_u8;
  uint8_t G_value;
  uint8_t WS_u8;
  uint8_t impact_value_u8;
  uint8_t Trigger_CV_1;
  uint8_t Trigger_CV_2;
  uint8_t Rudder_action;
  uint8_t Rudder_brake_action;
};