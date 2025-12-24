#pragma once
#include "Arduino.h"
struct __attribute__((packed)) Dap_hidmessage_st
{
  uint8_t payloadType;
  uint8_t magicKey1;
  uint8_t magicKey2;
  uint8_t length;
  char text[235];
};