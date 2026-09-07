#pragma once
#include "Arduino.h"
#include "PayloadHeader.h"
#include "PayloadFooter.h"

typedef struct __attribute__((packed)) PayloadWifiChannel
{
  uint8_t command_u8;            // 1=ScanReq, 2=ScanRes, 3=SetReq, 4=SetAck
  uint8_t currentChannel_u8;     // Current active Wi-Fi channel (1-13)
  uint8_t recommendedChannel_u8; // Recommended clean channel (1, 6, 11)
  int8_t  channel1Rssi_i8;       // Strongest AP RSSI on Ch 1
  int8_t  channel6Rssi_i8;       // Strongest AP RSSI on Ch 6
  int8_t  channel11Rssi_i8;      // Strongest AP RSSI on Ch 11
  uint8_t channel1ApCount_u8;    // AP count on Ch 1
  uint8_t channel6ApCount_u8;    // AP count on Ch 6
  uint8_t channel11ApCount_u8;   // AP count on Ch 11
  uint8_t channel1ApScore_u8;    // Congestion score (0-100) on Ch 1
  uint8_t channel6ApScore_u8;    // Congestion score (0-100) on Ch 6
  uint8_t channel11ApScore_u8;   // Congestion score (0-100) on Ch 11
} PayloadWifiChannel_t;

typedef struct __attribute__((packed)) DapWifiChannel
{
  PayloadHeader_t payloadHeader_st;
  PayloadWifiChannel_t payloadWifiChannel_st;
  PayloadFooter_t payloadFooter_st;
} DapWifiChannel_t;