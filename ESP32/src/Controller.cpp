#include "Controller.h"
#include <math.h>

uint16_t IRAM_ATTR_FLAG NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput_u8)
{
  float valRange_fl32 = (maxVal - minVal);
  float deadzoneCorrection_fl32 = 0.005f * valRange_fl32;

  float corrected_min_value_fl32 = minVal + deadzoneCorrection_fl32;
  float corrected_max_value_fl32 = maxVal - deadzoneCorrection_fl32;
  float corrected_valRange_fl32 = (corrected_max_value_fl32 - corrected_min_value_fl32);
  
  if (fabsf(corrected_valRange_fl32) < 0.0000001f)
  {
    return s_JOYSTICK_MIN_VALUE_U16;   // avoid div-by-zero
  }

  float fractional_fl32 = (value - corrected_min_value_fl32) / corrected_valRange_fl32;
  float controller_fl32 = s_JOYSTICK_MIN_VALUE_U16 + (fractional_fl32 * s_JOYSTICK_RANGE_U16);
  uint16_t controller_u16 = constrain(controller_fl32, s_JOYSTICK_MIN_VALUE_U16, (maxGameOutput_u8 * 0.01f) * s_JOYSTICK_MAX_VALUE_U16);
  return controller_u16;
}


#ifdef USB_JOYSTICK

#include <string>
//#include "Adafruit_TinyUSB.h"
#include "Joystick_ESP32S2.h"

#define JOYSTICK_AXIS_MINIMUM_U16 0U
#define JOYSTICK_AXIS_MAXIMUM_U16 65535U

uint16_t pedalType_u16 = 0U; // 0=Clutch, 1=Brake, 2=Throttle

Joystick_ joystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                    0, 0,                 // Button Count, Hat Switch Count
                    false, false, false,  // no X and no Y, no Z Axis
                    true, true, true,  //  Rx, no Ry, no Rz
                    false, false,         // No rudder or throttle
                    false, false, false);  // No accelerator, brake, or steering;

void SetupController_USB(uint8_t pedal_ID)
{
  int pid_i32;
  char *apName_pc;

  pedalType_u16 = pedal_ID;

  switch (pedal_ID)
  {
    case 0:
      pid_i32 = 0x8332;
      apName_pc = "FFB_Pedal_Clutch";
      break;
    case 1:
      pid_i32 = 0x8333;
      apName_pc = "FFB_Pedal_Brake";
      break;
    case 2:
      pid_i32 = 0x8334;
      apName_pc = "FFB_Pedal_Throttle";
      break;
    default:
      pid_i32 = 0x8332;
      apName_pc = "FFB_Pedal_NOASSIGNMENT";
      break;

  }

  int vid_i32 = 0x303A;
  joystick_.setVidPidProductVendorDescriptor(vid_i32, pid_i32, apName_pc, "OpenSource");
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRyAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRzAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.begin();
}

void SetupController()
{
  joystick_.setRxAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRyAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.setRzAxisRange(JOYSTICK_AXIS_MINIMUM_U16, JOYSTICK_AXIS_MAXIMUM_U16);
  joystick_.begin();
}

bool IsControllerReady()
{
  return joystick_.IsReady();
}

void SetControllerOutputValue(uint16_t value)
{
  /*  
  uint8_t highByte = (uint8_t)(value >> 8);
	uint8_t lowByte = (uint8_t)(value & 0x00FF);

  hid_report.brake_lowerByte = lowByte;
  hid_report.brake_higherByte = highByte;

  usb_hid.sendReport(0, &hid_report, sizeof(hid_report));
  */

  switch (pedalType_u16)
  {
    case 0:
      joystick_.setRxAxis(value);
      break;
    case 1:
      joystick_.setRyAxis(value);
      break;
    case 2:
      joystick_.setRzAxis(value);
      break;
    default:
      joystick_.setRxAxis(value);
      break;
  }

  joystick_.sendState();
}



#endif
