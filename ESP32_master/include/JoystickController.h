#pragma once

#include "Arduino.h"
#include "Main.h"
#ifdef USB_JOYSTICK
#include "TinyusbJoystick.h"
bool IsControllerReady();
void SetupController();
void SetControllerOutputValueBrake(int16_t value);
void SetControllerOutputValueAccelerator(int16_t value);
void SetControllerOutputValueThrottle(int16_t value);
void SetControllerOutputValueRudder(int16_t value);
void SetControllerOutputValueRudder_brake(int16_t value, int16_t value2);
void joystickSendState();
#endif
static const int16_t JOYSTICK_MIN_VALUE = INT16_MIN;
static const int16_t JOYSTICK_MAX_VALUE = INT16_MAX;
static const int32_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;
uint16_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput);

//bool GetJoystickStatus();
//void RestartJoystick();

