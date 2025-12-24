#pragma once
#include "Arduino.h"
struct __attribute__((packed)) payloadOtaInfo{
    uint8_t device_ID;
    uint8_t ota_action;
    uint8_t mode_select;
    uint8_t SSID_Length;
    uint8_t PASS_Length;
    uint8_t WIFI_SSID[64];
    uint8_t WIFI_PASS[64];
};