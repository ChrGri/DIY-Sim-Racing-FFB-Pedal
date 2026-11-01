#pragma once
#include "Arduino.h"
#include "PayloadHeader.h"
#include "PayloadFooter.h"

#define WIFI_SCAN_CHANNEL_COUNT_U8 13

typedef struct __attribute__((packed)) PayloadWifiChannel
{
  uint8_t command_u8;            // 1=ScanReq, 2=ScanRes, 3=SetReq, 4=SetAck
  uint8_t currentChannel_u8;     // Current active Wi-Fi channel (1-13)
  uint8_t recommendedChannel_u8; // Recommended clean channel (any of 1-13)
  // Per-channel scan results, index 0 = channel 1 .. index 12 = channel 13.
  int8_t  channelRssi_ai8[WIFI_SCAN_CHANNEL_COUNT_U8];    // Strongest AP RSSI seen on this channel (0 = none seen)
  uint8_t channelApCount_au8[WIFI_SCAN_CHANNEL_COUNT_U8]; // AP count on this channel
  uint8_t channelApScore_au8[WIFI_SCAN_CHANNEL_COUNT_U8]; // Congestion score (0-100, lower = cleaner)
} PayloadWifiChannel_t;

typedef struct __attribute__((packed)) DapWifiChannel
{
  PayloadHeader_t payloadHeader_st;
  PayloadWifiChannel_t payloadWifiChannel_st;
  PayloadFooter_t payloadFooter_st;
} DapWifiChannel_t;
