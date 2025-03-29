#include <string>
//#include <string>
#include "Controller.h"



#ifdef USB_JOYSTICK
  #include <Joystick_ESP32S2.h>
  
  Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   0, 0,                 // Button Count, Hat Switch Count
                   false, false, false,  // X and Y, but no Z Axis
                   false, false, false,  // No Rx, Ry, or Rz
                   false, false,         // No rudder or throttle
                   false, true, false);  // No accelerator, brake, or steering
  
  void SetupController() {
    Joystick.setBrakeRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);
    delay(100);
  
    Joystick.begin(false, 0);

    // rename HID device name, see e.g. https://github.com/schnoog/Joystick_ESP32S2/issues/8
    //USB.PID(0x8211);
    //USB.VID(0x303b);
    //USB.productName("DIY FFB pedal");
    //USB.manufacturerName("Open source");
    //USB.begin();

  }
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
    USB.PID(PID);    
    USB.VID(0x3035);
    USB.productName(APname);
    USB.manufacturerName("OpenSource");
    USB.begin();
    Joystick.setBrakeRange(JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);
    delay(100); 
    Joystick.begin();
  }
  bool IsControllerReady() { return true; }
  void SetControllerOutputValue(int32_t value) {
    Joystick.setBrake(value);
    Joystick.sendState();
  }
  void SetControllerOutputValue_rudder(int32_t value,int32_t value2)
  {
    Joystick.setBrake(value);
    Joystick.setAccelerator(value2);
  }


  
  
#elif defined BLUETOOTH_GAMEPAD
  #include <BleGamepad.h>


  
  // get the max address 
  // see https://arduino.stackexchange.com/questions/58677/get-esp32-chip-id-into-a-string-variable-arduino-c-newbie-here
  char ssid[23];
  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  unsigned int chip = (unsigned int)(chipid >> 32);
  std::string bluetoothName_lcl = "DiyFfbPedal_" + std::to_string( chip );
  BleGamepad bleGamepad(bluetoothName_lcl, bluetoothName_lcl, 100);
  
  
  void SetupController() {
    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_MULTI_AXIS); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
    bleGamepadConfig.setAxesMin(JOYSTICK_MIN_VALUE); // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
    bleGamepadConfig.setAxesMax(JOYSTICK_MAX_VALUE); // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
    //bleGamepadConfig.setWhichSpecialButtons(false, false, false, false, false, false, false, false);
    bleGamepadConfig.setWhichAxes(true, true, true, true, true, true, true, true);
    //bleGamepadConfig.setWhichSimulationControls(true, true, true, true, true); // only brake active 
    bleGamepadConfig.setButtonCount(0);
    bleGamepadConfig.setHatSwitchCount(0);
    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setPid(chip); // product id

    bleGamepad.begin(&bleGamepadConfig);

    //bleGamepad.deviceManufacturer = "DiyFfbPedal";
    //bleGamepad.deviceName = chip;
  }

  bool IsControllerReady() { return bleGamepad.isConnected(); }

  void SetControllerOutputValue(int32_t value) {
    //bleGamepad.setBrake(value);

    if (bleGamepad.isConnected() )
    {
      //bleGamepad.setAxes(value, 0, 0, 0, 0, 0, 0, 0);
      bleGamepad.setX(value);
      //bleGamepad.setSimulationControls(value, 0, 0, 0, 0);
      //bleGamepad.setSliders(value,0);
      bleGamepad.sendReport();
      
    }
    else
    {
      Serial.println("BLE not connected!");
      delay(500);
    }
    
    
  }
   void SetControllerOutputValue_rudder(int32_t value,int32_t value2) {
    //bleGamepad.setBrake(value);

    if (bleGamepad.isConnected() )
    {
      //bleGamepad.setAxes(value, 0, 0, 0, 0, 0, 0, 0);
      bleGamepad.setX(value);
      bleGamepad.setY(value2);
      //bleGamepad.setSimulationControls(value, 0, 0, 0, 0);
      //bleGamepad.setSliders(value,0);
      bleGamepad.sendReport();
      
    }
    else
    {
      Serial.println("BLE not connected!");
      delay(500);
    }
    
    
  }
  
#endif


int32_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput) {
  float valRange = (maxVal - minVal);
  if (abs(valRange) < 0.01) {
    return JOYSTICK_MIN_VALUE;   // avoid div-by-zero
  }

  float fractional = (value - minVal) / valRange;
  int32_t controller = JOYSTICK_MIN_VALUE + (fractional * JOYSTICK_RANGE);
  return constrain(controller, JOYSTICK_MIN_VALUE, (maxGameOutput/100.) * JOYSTICK_MAX_VALUE);
}

