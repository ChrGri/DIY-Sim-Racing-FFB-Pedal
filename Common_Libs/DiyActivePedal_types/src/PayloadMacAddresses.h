#pragma once
#include <stdint.h>
#include "Arduino.h"

#define DAP_NODE_CLUTCH_IDX    0U
#define DAP_NODE_BRAKE_IDX     1U
#define DAP_NODE_THROTTLE_IDX  2U
#define DAP_NODE_BRIDGE_IDX    3U

typedef struct __attribute__((packed)) PayloadMacAddresses
{
  uint8_t assignmentState_au8[4];
  uint8_t macAddress_aau8[4][6];
  uint8_t ownMacAddress_au8[6];
  uint8_t ownNodeType_u8;          // 0=Clutch, 1=Brake, 2=Throttle, 3=Bridge
  uint8_t wifiChannel_u8;
  uint8_t reserved_au8[2];
} PayloadMacAddresses_t;
