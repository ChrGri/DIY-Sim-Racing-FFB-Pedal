#pragma once

#include "Arduino.h"
#include "Main.h"

static const uint16_t s_JOYSTICK_MIN_VALUE_U16 = 0U;
static const uint16_t s_JOYSTICK_MAX_VALUE_U16 = UINT16_MAX;
static const uint16_t s_JOYSTICK_RANGE_U16 = s_JOYSTICK_MAX_VALUE_U16 - s_JOYSTICK_MIN_VALUE_U16;

uint16_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput_u8);



#ifdef USB_JOYSTICK
  void SetupController_USB(uint8_t pedal_ID);
  bool IsControllerReady();
  void SetControllerOutputValue(uint16_t value);
  void SetupController();
#endif

#ifdef USE_VENDOR_HID
  #include "DapAttributeProtocol.h"
  // Vendor HID interface (UsagePage 0xFF00, INPUT ReportID=0x02, OUTPUT ReportID=0x03).
  // Used for bidirectional data exchange with the SimHub plugin on PCB_VERSION=14.
  void SetupVendorHID();
  void VendorHID_SendData(const uint8_t* data, size_t len);
  bool VendorHID_ReadPacket(uint8_t* buf, size_t* outLen);
  // Attribute protocol (HID marker 0xA1 packets)
  bool VendorHID_ReadAttrPacket(DapAttrPacket_t* out);
  void VendorHID_SendAttrReply(const DapAttrPacket_t* pkt);
#endif