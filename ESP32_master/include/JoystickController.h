#pragma once

#include "Arduino.h"
#include "Main.h"
#ifdef USB_JOYSTICK

#include "TinyusbJoystick.h"
//#include "Joystick_ESP32S2.h"
extern uint8_t *hidDescriptorBufferForCheck; 
//extern Joystick_ tinyusbJoystick_;  // No accelerator, brake, or steering;
extern TinyusbJoystick tinyusbJoystick_;  // No accelerator, brake, or steering;
extern uint16_t reportSize;

bool IsControllerReady();
void SetupController();
void SetControllerOutputValueBrake(uint16_t value);
void SetControllerOutputValueAccelerator(uint16_t value);
void SetControllerOutputValueThrottle(uint16_t value);
void SetControllerOutputValueRudder(uint16_t value);
void SetControllerOutputValueRudder_brake(uint16_t value, uint16_t value2);
void joystickSendState();
#endif
static const uint16_t JOYSTICK_MIN_VALUE = 0;
static const uint16_t JOYSTICK_MAX_VALUE = UINT16_MAX;
static const int32_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;
uint16_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput);

//bool GetJoystickStatus();
//void RestartJoystick();

