#pragma once
#include <stdint.h>
#include "PayloadHeader.h"
#include "PayloadFooter.h"

#pragma pack(push, 1)

typedef struct payloadServoConfig {
  uint8_t readWriteFlag;           // 0 = Read from Servo, 1 = Write to Servo
  uint8_t numValidFields;          // Anzahl der zu verarbeitenden Array-Eintraege, 1 bis 10
  uint16_t registerAddresses[10];  // Die Modbus-Registeradressen
  uint16_t registerValues[10];     // Die zu schreibenden oder gelesenen Werte
} payloadServoConfig_t;

#pragma pack(pop)
