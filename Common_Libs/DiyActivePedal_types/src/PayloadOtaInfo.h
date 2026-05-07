#pragma once
#include "Arduino.h"
typedef struct __attribute__((packed)) PayloadOtaInfo
{
    uint8_t deviceId_u8;
    uint8_t otaAction_u8;
    uint8_t modeSelect_u8;
    uint8_t ssidLength_u8;
    uint8_t passLength_u8;
    uint8_t wifiSsid_au8[64];
    uint8_t wifiPass_au8[64];
} PayloadOtaInfo_t;