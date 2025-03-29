// Simple example application that shows how to read four Arduino
// digital pins and map them to the USB Joystick library.
//

// Ground digital pins 9, 10, 11, and 12 to press the joystick 
// buttons 0, 1, 2, and 3.
//
// NOTE: This sketch file is for use with Arduino Leonardo and
//       Arduino Micro only.
//
// by Matthew Heironimus
// 2015-11-20
//--------------------------------------------------------------------

#include <Arduino.h>
//#include <Joystick_ESP32S2.h>
//USBCDC USBSerial;

#define JOYTICK_OUTPUT
#ifdef JOYTICK_OUTPUT
#include <USB.h>
#include <USBHIDGamepad.h>

// Define the gamepad
USBHIDGamepad gamepad;

/*Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   0, 0,                 // Button Count, Hat Switch Count
                   false, false, false,  // X and Y, but no Z Axis
                   false, false, false,  // No Rx, Ry, or Rz
                   false, false,         // No rudder or throttle
                   false, true, false);  // No accelerator, brake, or steering
*/

#endif
void setup() {

  // Disable wakeup sources
  // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // // Create and acquire a power management lock
  // esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "no_sleep", &pm_lock);
  // esp_pm_lock_acquire(pm_lock);

#ifdef JOYTICK_OUTPUT
  USB.begin();
  gamepad.begin();
#endif


  // Serial.begin();
  Serial.begin(9600, SERIAL_8N1);

  // Serial.setTimeout(5);
  // Serial.setHwFlowCtrlMode(UART_HW_FLOWCTRL_DISABLE);

  // Serial.setTxTimeoutMs(0);
  
  
  //Joystick.setBrakeRange(0, 1000);

  // Initialize Joystick Library
  //Joystick.begin();
}


uint64_t counter = 0;
void loop() {

  delay(2);

  // increment joystick output
  counter+=1;
  if (counter > 1000)
  {
    counter = 0;
  }

  Serial.println(counter);

  // USBCDC tmp;

  // write large packet
  uint8_t tmp[100];
  Serial.write((char*)&tmp[0], sizeof(uint8_t) * 100 );
  // Serial.flush();

  // delay(2); // <-- This helped a lot!
#ifdef JOYTICK_OUTPUT
  // send USB HID output
  //Joystick.setBrake(counter);
  //gamepad.joystickMove(0, 32767, 0, 0);   // Move joystick to the right
  gamepad.leftStick(0, counter);
  #endif
}

