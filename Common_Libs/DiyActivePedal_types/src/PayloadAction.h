#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadPedalAction
{
  uint8_t triggerAbs_u8;
  //uint8_t resetPedalPos_u8; //1=reset position, 2=restart ESP
  uint8_t systemAction_u8; //1=reset position, 2=restart ESP, 3=OTA Enable, 4=enable pairing
  uint8_t startSystemIdentification_u8;
  uint8_t returnPedalConfig_u8;
  uint8_t rpm_u8;
  uint8_t gValue_u8;
  uint8_t wheelSlip_u8;
  uint8_t impactValue_u8;
  uint8_t triggerCv1_u8;
  uint8_t triggerCv2_u8;
  uint8_t triggerCv3_u8;
  uint8_t triggerCv4_u8;
  uint8_t rudderAction_u8;
  uint8_t rudderBrakeAction_u8;
} PayloadPedalAction_t;