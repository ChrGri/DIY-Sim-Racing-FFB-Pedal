#include "Controller.h"

uint16_t IRAM_ATTR_FLAG NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput) {
  float valRange_fl32 = (maxVal - minVal);
  float deadzoneCorrection_fl32 = 0.005f * valRange_fl32;

  float corrected_min_value_fl32 = minVal + deadzoneCorrection_fl32;
  float corrected_max_value_fl32 = maxVal - deadzoneCorrection_fl32;
  float corrected_valRange_fl32 = (corrected_max_value_fl32 - corrected_min_value_fl32);
  
  if (abs(corrected_valRange_fl32) < 0.0000001f) {
    return JOYSTICK_MIN_VALUE;   // avoid div-by-zero
  }

  float fractional_fl32 = (value - corrected_min_value_fl32) / corrected_valRange_fl32;
  float controller_fl32 = JOYSTICK_MIN_VALUE + (fractional_fl32 * JOYSTICK_RANGE);
  uint16_t controller_u16 = constrain(controller_fl32, JOYSTICK_MIN_VALUE, (maxGameOutput * 0.01f) * JOYSTICK_MAX_VALUE);
  return controller_u16;
}


#ifdef USB_JOYSTICK

#include <string>
//#include "Adafruit_TinyUSB.h"
#include "Joystick_ESP32S2.h"

#define JOYSTICK_AXIS_MINIMUM 0
#define JOYSTICK_AXIS_MAXIMUM 65535

Joystick_ joystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                    0, 0,                 // Button Count, Hat Switch Count
                    false, false, false,  // no X and no Y, no Z Axis
                    true, false, false,  //  Rx, no Ry, no Rz
                    false, false,         // No rudder or throttle
                    false, false, false);  // No accelerator, brake, or steering;

void SetupController_USB(uint8_t pedal_ID) 
{
  int PID;
  char *APname;
  switch(pedal_ID)
  {
    case 0:
      PID=0x8214;
      APname="FFB_Pedal_Clutch";
      break;
    case 1:
      PID=0x8215;
      APname="FFB_Pedal_Brake";
      break;
    case 2:
      PID=0x8216;
      APname="FFB_Pedal_Throttle";
      break;
    default:
      PID=0x8217;
      APname="FFB_Pedal_NOASSIGNMENT";
      break;

  }
  int VID= 0x35;
  joystick_.setVidPidProductVendorDescriptor(VID,PID, APname, "OpenSource");
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM, JOYSTICK_AXIS_MAXIMUM);
  joystick_.begin();
}

void SetupController() 
{
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM, JOYSTICK_AXIS_MAXIMUM);
  joystick_.begin();
}

bool IsControllerReady() { 
  return joystick_.IsReady();
}

void SetControllerOutputValue(uint16_t value) {
  /*  
  uint8_t highByte = (uint8_t)(value >> 8);
	uint8_t lowByte = (uint8_t)(value & 0x00FF);

  hid_report.brake_lowerByte = lowByte;
  hid_report.brake_higherByte = highByte;

  usb_hid.sendReport(0, &hid_report, sizeof(hid_report));
  */
 joystick_.setRxAxis(value);
 joystick_.sendState();
}



#endif
