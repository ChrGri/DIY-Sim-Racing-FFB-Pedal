#pragma once
#include "Arduino.h"
#include <string>
#include "Adafruit_TinyUSB.h"
#include <DiyActivePedal_types.h>
//extern Adafruit_USBD_HID usb_hid;
// HID Report Descriptor for Bridge, 6 axis
enum HidReportId
{
    NONE,
    JOYSTICK_STRUCT,
    HID_PAYLOAD_INPUT,
    HID_PAYLOAD_OUTPUT
};
#define PACKET_SIZE 63
#define PACKET_SIZE_GET 64
#define HEADER_SIZE 3  // ID + Type + total length+ chunk length
#define HEADER_SIZE_GET 4  // ID + Type + total length+ chunk length
#define PAYLOAD_SIZE (PACKET_SIZE - HEADER_SIZE)
#define PKT_TYPE_START 0x01
#define PKT_TYPE_CONT 0x02
#define PKT_TYPE_END 0x03

uint8_t const desc_hid_report_old[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04, // Usage (Joystick)
    0xA1, 0x01, // Collection (Application)

    // Define six 16-bit axes (X, Y, Z, Rx, Ry, Rz)
    0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x33,       //   Usage (Rx)
    0x09, 0x34,       //   Usage (Ry)
    0x09, 0x35,       //   Usage (Rz)
    0x16, 0x00, 0x80, //   Logical Minimum (-32768)
    0x26, 0xFF, 0x7F, //   Logical Maximum (32767)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x06,       //   Report Count (6) 
    0x81, 0x02,       //   Input (Data,Var,Abs)

    0xC0, // End Collection
};
#define HID_LOGICAL_MAX_32(n) 0x27, (uint8_t)((n) & 0xFF), (uint8_t)(((n) >> 8) & 0xFF), (uint8_t)(((n) >> 16) & 0xFF), (uint8_t)(((n) >> 24) & 0xFF)


uint8_t const desc_hid_report[] = 
{
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_JOYSTICK ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        HID_REPORT_ID(JOYSTICK_STRUCT)
        
        HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP ),
        HID_USAGE      ( HID_USAGE_DESKTOP_POINTER ),
        HID_LOGICAL_MIN( 0 ),
        HID_LOGICAL_MAX_32( 65535 ), 
        HID_REPORT_SIZE( 32 ),
        HID_REPORT_COUNT( 6 ),
        
        HID_COLLECTION ( HID_COLLECTION_PHYSICAL ),
            HID_USAGE ( HID_USAGE_DESKTOP_X  ),
            HID_USAGE ( HID_USAGE_DESKTOP_Y  ),
            HID_USAGE ( HID_USAGE_DESKTOP_Z  ),
            HID_USAGE ( HID_USAGE_DESKTOP_RX ),
            HID_USAGE ( HID_USAGE_DESKTOP_RY ),
            HID_USAGE ( HID_USAGE_DESKTOP_RZ ),
            HID_INPUT ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        HID_COLLECTION_END,
        
    HID_COLLECTION_END,
    HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2 ),
    HID_USAGE        ( 0x01 ),
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),
        HID_REPORT_ID(HID_PAYLOAD_INPUT) 
        HID_USAGE(0x10), 
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(63),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        HID_REPORT_ID(HID_PAYLOAD_OUTPUT) 
        HID_USAGE(0x10), 
        HID_REPORT_SIZE(8),
        HID_REPORT_COUNT(63),
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        
    HID_COLLECTION_END,
    
};

class TinyusbJoystick {
private:
    static const uint16_t JOYSTICK_MIN_VALUE = INT16_MIN;
    static const uint16_t JOYSTICK_MAX_VALUE = INT16_MAX;
    static const uint16_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;

    // USB HID object
    // Report payload for the two axes
    typedef struct
    {
        int32_t x;
        int32_t y;
        int32_t z;
        int32_t rx;
        int32_t ry;
        int32_t rz;
    } hid_report_t;
    
    hid_report_t hid_report = {0};
    uint16_t checksumCal(uint8_t * data, uint16_t length);


public:
  uint8_t rxBuffer[4096]; 
  uint8_t buffDis[64];
  int rxIndex = 0;
  int buffSizeDis=0;
  int reportType;
  int reportID;
  bool isReceiving = false;
  bool isGetData=false;
  Adafruit_USBD_HID usb_hid;
  bool isActionGet[8];
  bool isConfigGet[3];
  bool isTestConfigGet[3];
  bool isBridgeActionGet;
  bool isOtaActionGet;
  DAP_actions_st tmpAction[8];
  DAP_config_st tmpConfig[3];
  DAP_bridge_state_st tmpBridgeAction;
  DAP_action_ota_st tmpOtaAction;
  uint8_t rawLength;
  static TinyusbJoystick* instance;
  TinyusbJoystick();
  bool IsReady();
  void begin(int VID, int PID);
  void setXAxis(int32_t value);
  void setYAxis(int32_t value);
  void setZAxis(int32_t value);
  void setRxAxis(int32_t value);
  void setRyAxis(int32_t value);
  void setRzAxis(int32_t value);
  void sendState();
  void sendData(uint8_t* data, size_t totalLen);
  static void context_callback(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
  void onHIDReceived(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
  void ProcessFullData(uint8_t *rxBuffer, uint8_t totalLen);
  void printf(const char *log,...);
};