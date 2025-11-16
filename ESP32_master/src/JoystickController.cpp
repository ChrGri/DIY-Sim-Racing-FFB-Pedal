#include "JoystickController.h"

//#include "Main.h"
uint16_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput)
{
    float valRange = (maxVal - minVal);
    if (abs(valRange) < 0.01)
    {
        return JOYSTICK_MIN_VALUE; // avoid div-by-zero
    }

    float fractional = (value - minVal) / valRange;
    uint16_t controller = JOYSTICK_MIN_VALUE + (maxGameOutput / 100.0f) * (fractional * JOYSTICK_RANGE);
    return controller;
}


#ifdef USB_JOYSTICK
//TinyusbJoystick tinyusbJoystick_;

Joystick_ tinyusbJoystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                    0, 0,                 // Button Count, Hat Switch Count
                    true, true, true,  // X and Y, but no Z Axis
                    true, true, true,  // No Rx, Ry, or Rz
                    false, false,         // No rudder or throttle
                    false, false, false);  // No accelerator, brake, or steering;

void SetupController()
{
    
    int PID = 0x8213;
    int VID = 0x3035;
    tinyusbJoystick_.setVidPidProductVendorDescriptor(VID,PID,"DIY_FFB_PEDAL_JOYSTICK","OpenSource");    
    tinyusbJoystick_.setRxAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);
    tinyusbJoystick_.setRyAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);
    tinyusbJoystick_.setRzAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);
    tinyusbJoystick_.setXAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);//rudder
    tinyusbJoystick_.setYAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);//rudder brake brake
    tinyusbJoystick_.setZAxisRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);//rudder brake throttle
    tinyusbJoystick_.begin();
    
}

bool IsControllerReady()
{
    bool returnValue_b = tinyusbJoystick_.IsReady();
    return returnValue_b;
}

void SetControllerOutputValueBrake(uint16_t value)
{
    tinyusbJoystick_.setRxAxis(value);
}

void SetControllerOutputValueAccelerator(uint16_t value)
{
    tinyusbJoystick_.setRyAxis(value);
}

void SetControllerOutputValueThrottle(uint16_t value)
{
    tinyusbJoystick_.setRzAxis(value);
}

void SetControllerOutputValueRudder(uint16_t value)
{
    tinyusbJoystick_.setXAxis(value);
}
void SetControllerOutputValueRudder_brake(uint16_t value, uint16_t value2)
{

    tinyusbJoystick_.setYAxis(value);
    tinyusbJoystick_.setZAxis(value2);
}

void joystickSendState()
{
    tinyusbJoystick_.sendState();
}
#endif

