#include "isv57communication.h"
#include "Main.h"
#include "isv57_tunedParameters.h"

Modbus modbus(Serial2);


Stream *ActiveSerialForServoCommunication = nullptr;


void printDecodedAlarmString(uint16_t alarm_code) 
{

  switch (alarm_code & 0x0FFF) { // Mask to get lower 12 bits
    case 0x000:
        ActiveSerial->println("Normal\n");
        break;
    case 0x0E1:
    case 0x0E0:
        ActiveSerial->println("Overcurrent\n");
        break;
    case 0x100:
        ActiveSerial->println("Overload\n");
        break;
    case 0x180:
        ActiveSerial->println("Excessive position deviation\n");
        break;
    case 0x1A0:
        ActiveSerial->println("Overspeed\n");
        break;
    case 0x1A1:
        ActiveSerial->println("Motor out of control\n");
        break;
    case 0x0D0:
        ActiveSerial->println("Undervoltage\n");
        break;
    case 0x0C0:
        ActiveSerial->println("Overvoltage\n");
        break;
    case 0x171:
    case 0x172:
        ActiveSerial->println("Encoder parameter error\n");
        break;
    case 0x190:
        ActiveSerial->println("Excessive motor vibration\n");
        break;
    case 0x150:
        ActiveSerial->println("Encoder disconnected\n");
        break;
    case 0x151:
    case 0x170:
        ActiveSerial->println("Encoder data error\n");
        break;
    case 0x152:
        ActiveSerial->println("Encoder HALL signal error\n");
        break;
    case 0x240:
        ActiveSerial->println("Parameter saving error\n");
        break;
    case 0x570:
        ActiveSerial->println("Emergency stop\n");
        break;
    case 0x120:
        ActiveSerial->println("Regenerative energy overload\n");
        break;
    case 0x153:
        ActiveSerial->println("Encoder battery error\n");
        break;
    case 0x210:
    case 0x211:
    case 0x212:
        ActiveSerial->println("Input configuration error (Repeated/wrong input)\n");
        break;
    default:
        ActiveSerial->println("Unknown or refer to Chapter 9\n");
        break;
  }
}




// initialize the communication
Isv57Communication::Isv57Communication()
{
  
  

  //Serial1.begin(38400, SERIAL_8N2, ISV57_RXPIN, ISV57_TXPIN, true); // Modbus serial
  #if PCB_VERSION == 10 || PCB_VERSION == 9 || PCB_VERSION == 12 || PCB_VERSION == 13
    Serial2.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, false); // Modbus serial
  #else
    Serial2.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, true); // Modbus serial
  #endif


  // #ifdef USE_CDC_INSTEAD_OF_UART
  //   ActiveSerialForServoCommunication = &Serial2;
  // #else
  //   ActiveSerialForServoCommunication = &Serial2;
  // #endif

  ActiveSerialForServoCommunication = &Serial2;


  modbus.initialize(false);
}




// send tuned servo parameters
void Isv57Communication::setupServoStateReading() {

  // The iSV57 has four registers (0x0191, 0x0192, 0x0193, 0x0194) in which we can write, which values we want to obtain cyclicly
  // These registers can be obtained by sending e.g. the command: 0x63, 0x03, 0x0191, target_sate, CRC
  // tell the modbus slave, which registers will be read cyclicly
  modbus.writeAndVerifyDeviceParameter(slaveId, 0x0191, reg_add_position_given_p);
  modbus.writeAndVerifyDeviceParameter(slaveId, 0x0192, reg_add_velocity_current_feedback_percent);
  modbus.writeAndVerifyDeviceParameter(slaveId, 0x0193, reg_add_position_error_p);
  modbus.writeAndVerifyDeviceParameter(slaveId, 0x0194, reg_add_voltage_0p1V);
  //modbus.writeAndVerifyDeviceParameter(slaveId, 0x0195, reg_add_velocity_feedback_rpm);


  //modbus.writeAndVerifyDeviceParameter(slaveId, 0x0193, reg_add_position_feedback_p);
}


void Isv57Communication::readAllServoParameters() {
  for (uint16_t reg_sub_add_u16 = 0;  reg_sub_add_u16 < (pr_7_00+49); reg_sub_add_u16++)
  {
    modbus.readDeviceParameter(slaveId, pr_0_00 + reg_sub_add_u16);
  }
}

// Disable aixs command
void Isv57Communication::disableAxis()
{

  ActiveSerial->println("Disabling servo");

  // 0x3f, 0x06, 0x00, 0x85, 0x03, 0x03, 0xdc, 0x0c
  //modbus.writeAndVerifyDeviceParameter(slaveId, 0x0085, 0x0303);
  modbus.writeHoldingRegisterToDevice(slaveId, 0x0085, 0x0303);
  // 0x3f, 0x06, 0x01, 0x39, 0x00, 0x00, 0x5c, 0xe5
  //modbus.writeAndVerifyDeviceParameter(slaveId, 0x0139, 0x0000); 
  modbus.writeHoldingRegisterToDevice(slaveId, 0x0139, 0x0008);
  delay(30);

  // read routine
  modbus.readHoldingRegisterFromDevice(slaveId, 0x0085);
  modbus.readHoldingRegisterFromDevice(slaveId, 0x0139);
  delay(5);
}

void Isv57Communication::enableAxis() 
{
  ActiveSerial->println("Enabling servo");

  // 0x3f, 0x06, 0x00, 0x85, 0x03, 0x83, 0xdd, 0xac
  // Pr4.08: 0x085
  modbus.writeHoldingRegisterToDevice(slaveId, 0x0085, 0x0383);
  // 0x3f, 0x06, 0x01, 0x39, 0x00, 0x08, 0x5d, 0x23
  modbus.writeHoldingRegisterToDevice(slaveId, 0x0139, 0x0008);
  delay(30);

  // read routine
  modbus.readHoldingRegisterFromDevice(slaveId, 0x0085);
  modbus.readHoldingRegisterFromDevice(slaveId, 0x0139);
  delay(5);

  // modbus.holdingRegisterRead(0x0085);
  // modbus.holdingRegisterRead(0x0139);
  
}


// void isv57communication::resetAxisCounter() 
// {
//   ActiveSerial->println("Reset axis counter");

//   modbus.holdingRegisterRead(0x0085);
//   delay(10);
//   modbus.holdingRegisterRead(0x0139);
//   delay(10);
  
// }







void  Isv57Communication::clearServoUnitPosition()
{
	// According to Leadshines User Manual of 2ELD2-RD DC Servo
	// https://www.leadshine.com/upfiles/downloads/a3d7d12a120fd8e114f6288b6235ac1a_1690179981835.pdf
	// Changing the position unit, will clear the position data

  modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+20, 0); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  delay(100);
	modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+20, 1); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  delay(100);
}

bool Isv57Communication::setServoVoltage(uint16_t voltageInVolt_u16)
{
  return modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+32, voltageInVolt_u16 + 2); // bleeder braking voltage. Voltage when braking is activated
}

bool Isv57Communication::setPositionSmoothingFactor(uint16_t posSmoothingFactor_u16)
{
  return modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+22, posSmoothingFactor_u16); // positional command smoothing factor in 0.1ms
}

bool Isv57Communication::setRatioOfInertia(uint8_t ratiOfInertia_u8)
{
  return modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+4, ratiOfInertia_u8); // positional command smoothing factor in 0.1ms
}


// send tuned servo parameters
void Isv57Communication::sendTunedServoParameters(bool commandRotationDirection, uint32_t stepsPerMotorRev_u32, uint32_t ratioOfInertia_u32) {
  
  bool retValue_b = false;


  
// #define ADAPTIVE_SERVO_PARAMS
// #ifdef ADAPTIVE_SERVO_PARAMS
//   // see https://atbautomation.eu/uploads/User_Manual_Leadshine_iSV2-RS.pdf, p.22, Pr0.00
//   // 1) Pr0.01 = 0 --> position mode
//   // 2) Pr0.02 = 1 --> interpolation mode
//   // 3) Pr0.04 inertia ratio
//   // 4) Pr0.03 machine stiffness
//   // 5) Pr0.00 = 1 --> adaptive bandwidth
//   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+0, 1); // adaptive bandwidth modell following controll
//   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+2, 1); // positioning mode with auto tuning
//   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+3, 9); // machine stiffness
//   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+4, 1); // inertia
//   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+0, 2); // adaptive filter on all the time
// #endif


  // Pr0 register
  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+0, tuned_parameters[pr_0_00+0]); // control mode

  // according to the iSV2 manual chapter 5.6, the model following control (MFC) parameter should be larger then Pr1.01, velocity loop gain
  float mfcLowerLimit_fl32 = tuned_parameters[pr_1_00+1] ;
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+0, (int32_t)mfcLowerLimit_fl32);

  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+1, tuned_parameters[pr_0_00+1]); // control mode #
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+2, tuned_parameters[pr_0_00+2]); // deactivate auto gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+3, tuned_parameters[pr_0_00+3]); // machine stiffness
  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+4, ratioOfInertia_u32 ); // ratio of inertia
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+4, tuned_parameters[pr_0_00+3] ); // ratio of inertia
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+6, tuned_parameters[pr_0_00+6]); // motor command direction
  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+6, commandRotationDirection); // Command Pulse Rotational Direction
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+8, (long)stepsPerMotorRev_u32); // microsteps
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+9, tuned_parameters[pr_0_00+9]); // 1st numerator 
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+10, tuned_parameters[pr_0_00+10]); // & denominator
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+13, tuned_parameters[pr_0_00+13]); // 1st torque limit
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+14, tuned_parameters[pr_0_00+14]); // position deviation setup
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+16, tuned_parameters[pr_0_00+16]); // regenerative braking resitor
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+17, tuned_parameters[pr_0_00+17]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+18, tuned_parameters[pr_0_00+18]); // vibration suppression
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+19, tuned_parameters[pr_0_00+19]);

  // Pr1 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+0, tuned_parameters[pr_1_00+0]); // 1st position gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+1, tuned_parameters[pr_1_00+1]); // 1st velocity loop gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+2, tuned_parameters[pr_1_00+2]); // 1st time constant of velocity loop
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+3, tuned_parameters[pr_1_00+3]); // 1st filter of velocity detection
  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+4, tuned_parameters[pr_1_00+4]); // 1st torque filter

  // according to the iSV2 manual, the upper limit of the velocity loop gain is determined by the formula: upperLimit = 1000000 / (2 * pi * velocity loop gain * 4), where the velocity loop gain is determined by Pr1.01, so I want to make sure that the velocity loop gain is not set higher than this limit, otherwise the servo will be unstable. The factor 4 is a safety factor to make sure that we are well below the upper limit.
  float upperLimit_fl32 = 1000000.0f / (2.0f * PI_FL32 * tuned_parameters[pr_1_00+1] * 4.0f);
  upperLimit_fl32 = 20.0f;
  upperLimit_fl32 = constrain(upperLimit_fl32, 0, 2500.0f);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+4, upperLimit_fl32);

  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+10, tuned_parameters[pr_1_00+10]); // velocity feed forward gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+11, tuned_parameters[pr_1_00+11]); // velocity feed forward filter
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+12, tuned_parameters[pr_1_00+12]); // torque feed forward gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+13, tuned_parameters[pr_1_00+13]); // torque feed forward filter
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+15, tuned_parameters[pr_1_00+15]); // control switching mode
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+28, tuned_parameters[pr_1_00+28]); // 1st position loop integral time
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+29, tuned_parameters[pr_1_00+29]); // 1st position loop differential time
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+33, tuned_parameters[pr_1_00+33]); // speed given filter
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+35, tuned_parameters[pr_1_00+35]); // position command filter
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+36, tuned_parameters[pr_1_00+36]); // encoder feedback
  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+37, 1052); // special function register
  //uint16_t special_function_flags = 0x4 | 0x8 | 0x10 | 0x40 | 0x400;
  uint16_t special_function_flags = 0x4 | 0x8 | 0x10 | 0x400;
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_1_00+37, tuned_parameters[pr_1_00+37]); // special function register
  // see https://www.oyostepper.com/images/upload/File/ISV57T-180.pdf
  // 0x01: =0: Enablespeedfeed-forwardfiltering; =1:Disablespeed feed-forward filtering
  // 0x02: =0: Enabletorquefeed-forwardfiltering; =2:disabletorque feed-forward filtering
  // 0x04: =0: Enablemotor stall Er1A1 alarm; =4:Blockmotor stall Er1A1 alarm
  // 0x08: =0: Enable overshoot Er180 alarm; =8:Mask overshoot Er180alarm
  // 0x10: =0: Enable overload Er100 alarm; =0x10: Mask overload Er100alarm
  // 0x20: =0: dial input function not assignable; =0x20: dial input function assignable
  // 0x40: =0: Mask drive disable Er260 alarm; =0x40: Enable drive disable Er260 alarm
  // 0x400: =0: Mask undervoltage Er0D0 alarm; =0x400: Enable undervoltage Er0D0 alarm
  

  
  // Pr2 register
  // vibration suppression 
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00, tuned_parameters[pr_2_00]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+1, tuned_parameters[pr_2_00+1]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+2, tuned_parameters[pr_2_00+2]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+3, tuned_parameters[pr_2_00+3]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+4, tuned_parameters[pr_2_00+4]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+5, tuned_parameters[pr_2_00+5]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+6, tuned_parameters[pr_2_00+6]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+22, tuned_parameters[pr_2_00+22]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_2_00+23, tuned_parameters[pr_2_00+23]);// FIR based command smoothing time. Since the stpper task runs every 4ms, this time is selected to be larger than that. Unit is 0.1ms 
  

  // Pr3 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_3_00+12, tuned_parameters[pr_3_00+12]); // time setup acceleration
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_3_00+13, tuned_parameters[pr_3_00+13]); // time setup deceleration
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_3_00+24, tuned_parameters[pr_3_00+24]); // maximum rpm
  

  // Pr5 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+13, tuned_parameters[pr_5_00+13]); // overspeed level
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+20, tuned_parameters[pr_5_00+20]); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  // retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+35, 1); // lock front panel
  
  // Pr6 register
  // retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_6_00+17, 1); // lock front panel
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_6_00+11, tuned_parameters[pr_6_00+11]); // current response time. Default was 100%. Reducing value causes less noise

  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+32, 300); // command pulse input maximum setup

  // Pr7 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+0, tuned_parameters[pr_7_00+0]); // current loop gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+1, tuned_parameters[pr_7_00+1]); // current loop integral time
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+11, tuned_parameters[pr_7_00+11]); // Motor Maximum speed
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+28, tuned_parameters[pr_7_00+28]);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+29, tuned_parameters[pr_7_00+29]);

  // Enable & tune reactive pumping. This will act like a braking resistor and reduce EMF voltage.
  // See https://en.wikipedia.org/wiki/Bleeder_resistor
  // Info from iSV2 manual: The external resistance is activated when the actual bus voltage is higher than Pr7.32 plus Pr7.33 and is deactivated when the actual bus voltage is lower than Pr7.32 minus Pr7.33
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+31, tuned_parameters[pr_7_00+31]); // bleeder control mode; 0: is default and seems to enable braking mode, contrary to manual
  retValue_b |= setServoVoltage(SERVO_MAX_VOLTAGE_IN_V_36V);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+33, tuned_parameters[pr_7_00+33]); // bleeder hysteresis voltage; Contrary to the manual this seems to be an offset voltage, thus Braking disabling voltage = Pr7.32 + Pr.33




  

  //retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+33, 0); // pulse regenerative output limit setup [0,1]
  // retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_6_00+37, 0); // oscillation detection level [0, 1000] 0.1%


  // disable axis after servo startup --> ESP has to enable the axis first
  // Pr4.08
  // long servoEnableStatus = modbus.holdingRegisterRead(slaveId, 0x03, pr_4_00+8);
  // ActiveSerial->print("Servo enable setting: ");
  // ActiveSerial->println(servoEnableStatus, HEX);
  // delay(100);
  // if (servoEnableStatus != 0x303)
  // {
  //   isv57communication::disableAxis();
  // }
  // delay(100);
  // servoEnableStatus = modbus.holdingRegisterRead(slaveId, 0x03, pr_4_00+8);
  // ActiveSerial->print("Servo enable setting: ");
  // ActiveSerial->println(servoEnableStatus, HEX);

  // disable axis by default
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_4_00+8, tuned_parameters[pr_4_00+8]);
  


  // store the settings to servos NVM if necesssary
  if (retValue_b)
  {
    // disable axis a second time, since the second signal must be send to. Don't know yet the meaning of that signal.
    disableAxis();

    ActiveSerial->println("Servo registered in NVM have been updated! Please power cycle the servo and the ESP!");

    // identified with logic analyzer. See \StepperParameterization\Meesages\StoreSettingsToEEPROM_0.png
    modbus.writeHoldingRegisterToDevice(slaveId, 0x019A, 0x5555); // store the settings to servos NVM
    // ToDo: according to iSV57 manual, 0x2211 is the command to write values to EEPROM
    delay(500);
    
    
    // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x6666 is soft reset
    // modbus.writeHoldingRegister(slaveId, 0x019A, 0x6666); // store the settings to servos NVM
    
    
    isv57_update_parameter_b=true;
    delay(1000);
  }


}

bool Isv57Communication::findServosSlaveId()
{
  bool slaveIdFound = false;

  // typically the servo address is 63, so start with that
  int slaveIdTest = 63;
  if(modbus.sendRequestAndReceiveResponse(slaveIdTest, 0x03, 0x0000, 2) > 0)
  {
    slaveId = slaveIdTest;
    slaveIdFound = true;
    ActiveSerial->print("Found servo slave ID:");
    ActiveSerial->print(slaveId);
    ActiveSerial->print("\r\n");
  }


  if (false == slaveIdFound )
  {
    for (slaveIdTest = 0; slaveIdTest<256; slaveIdTest++)
    {
        if(modbus.sendRequestAndReceiveResponse(slaveIdTest, 0x03, 0x0000, 2) > 0)
        {
          slaveId = slaveIdTest;
          slaveIdFound = true;
          ActiveSerial->print("Found servo slave ID:");
          ActiveSerial->print(slaveId);
          ActiveSerial->print("\r\n");
          break;
        }

        delay(5);
    }
  }
  
  return slaveIdFound;
}




bool Isv57Communication::checkCommunication()
{
  if(modbus.sendRequestAndReceiveResponse(slaveId, 0x03, 0x0000, 2) > 0)
  {
    //ActiveSerial->println("Lifeline check: true");
    return true;
  }
  else
  {
    //ActiveSerial->println("Lifeline check: false");
    return false;
  }
  return false;
}



void Isv57Communication::setZeroPos()
{
  zeroPos = isv57dynamicStates_.servo_pos_given_p;
}

void Isv57Communication::applyOfsetToZeroPos(int16_t givenPosOffset_i16)
{
  zeroPos += givenPosOffset_i16;
}

int16_t Isv57Communication::getZeroPos()
{
  return zeroPos;
}

int16_t Isv57Communication::getPosFromMin()
{
  return isv57dynamicStates_.servo_pos_given_p - zeroPos;
}

int32_t Isv57Communication::getServoCycleCounter()
{
  return isv57dynamicStates_.servo_cycleCounter_u32;
}

uint32_t Isv57Communication::getServoCycleTimestamp()
{
  return isv57dynamicStates_.lastUpdateTimeInMS_u32;
}


// read servo states
void Isv57Communication::readServoStates() {

  // initialize with -1 to indicate non-trustworthiness
  for (uint8_t i = 0; i < NUMBER_OF_ISV57_REGISTERS_TO_READ_IN_CYCLIC_READ; i++) {
    regArray[i] = -1;
  }

  // read the four registers simultaneously
  int8_t numberOfRegistersToRead_u8 = NUMBER_OF_ISV57_REGISTERS_TO_READ_IN_CYCLIC_READ;
  int bytesReceived_i = modbus.sendRequestAndReceiveResponse(slaveId, 0x03, ref_cyclic_read_0, numberOfRegistersToRead_u8);
  if(bytesReceived_i == (numberOfRegistersToRead_u8*2))
  {
    modbus.getRawRxBuffer(raw,  len);
    for (uint8_t regIdx = 0; regIdx < numberOfRegistersToRead_u8; regIdx++)
    { 
      regArray[regIdx] = modbus.convertRxBufferToInt16(regIdx);
    }

    // write to public variables
    isv57dynamicStates_.servo_pos_given_p = regArray[0];
    isv57dynamicStates_.servo_current_percent = regArray[1];
    isv57dynamicStates_.servo_pos_error_p = regArray[2];
    isv57dynamicStates_.servoVoltage0p1V_i16 = regArray[3];

    isv57dynamicStates_.lastUpdateTimeInMS_u32 = millis();
    isv57dynamicStates_.servo_cycleCounter_u32++;
  }

  
  
  
  //#define ENABLE_SERVO_STATE_PRINTING
  #ifdef ENABLE_SERVO_STATE_PRINTING
  // print registers
  // print only every 100ms to avoid flooding the serial monitor, since the servo states are read every 10ms^
  static uint32_t lastTimePrintedServoStatesInMS_u32 = 0;
  if (millis() - lastTimePrintedServoStatesInMS_u32 > 200)
  {
    lastTimePrintedServoStatesInMS_u32 = millis();

    ActiveSerial->print("Bytes :");
    ActiveSerial->println(bytesReceived_i);

    ActiveSerial->print("Pos_given:");
    ActiveSerial->print(isv57dynamicStates_.servo_pos_given_p);

    ActiveSerial->print(",Pos_error:");
    ActiveSerial->print(isv57dynamicStates_.servo_pos_error_p);

    ActiveSerial->print(",Cur_given:");
    ActiveSerial->print(isv57dynamicStates_.servo_current_percent);

    ActiveSerial->print(",Voltage:");
    ActiveSerial->print(isv57dynamicStates_.servoVoltage0p1V_i16);

    ActiveSerial->print(",Velocity_given:");
    ActiveSerial->print(isv57dynamicStates_.servo_velocity_given_rpm_i16);

    ActiveSerial->println(" "); 
  }
  #endif
  
}



bool Isv57Communication::clearServoAlarms() {

  // read the alarm list
  // int8_t numberOfRegistersToRead_u8 = 0;
  // Alarm register address: 0x02
  //int bytesReceived_i = modbus.requestFrom(slaveId, 0x03, 0x02, numberOfRegistersToRead_u8);

  // clear alarm list
  //modbus.writeHoldingRegister(slaveId, 0x019a, 0x7777); 
  modbus.writeHoldingRegisterToDevice(slaveId, 0x019a, 0x7788);
  
  // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x1111 resets current alarm; 0x1122 resets alarm history
    
  return 1;
}


bool Isv57Communication::readCurrentAlarm() {
  int bytesReceived_i = modbus.sendRequestAndReceiveResponse(slaveId, 0x03, 0x01F2, 1);
  if(bytesReceived_i == (2))
  {
    modbus.getRawRxBuffer(raw,  len);
    for (uint8_t regIdx = 0; regIdx < 1; regIdx++)
    { 
      uint16_t tmp = modbus.convertRxBufferToInt16(regIdx) && 0x0FFF; // mask the first half byte as it does not contain info
      ActiveSerial->print("Current iSV57 alarm: ");
      ActiveSerial->println( tmp, HEX);
    }
  }

  return 1;
}


bool Isv57Communication::readAlarmHistory() {

  bool alarmWasFound_b = false;
	ActiveSerial->print("\niSV57 alarm history: ");
	for (uint8_t idx=0; idx < 12; idx++)
	{
	  // example signal, read the 9th alarm
	  // 0x3f, 0x03, 0x12, 0x09, 0x00, 0x01, 0x55, 0xAE

	  // read the four registers simultaneously
	  int bytesReceived_i = modbus.sendRequestAndReceiveResponse(slaveId, 0x03, 0x1200 + idx, 1);
    
	  if(bytesReceived_i == (2))
	  {
      modbus.getRawRxBuffer(raw,  len);
      for (uint8_t regIdx = 0; regIdx < 1; regIdx++)
      { 
        uint16_t alarm_code = modbus.convertRxBufferToInt16(regIdx) & 0x0FFF; // mask the first half byte as it does not contain info

        if (alarm_code > 0)
        {
          ActiveSerial->print("Alarm Idx: ");
          ActiveSerial->print(idx);
          ActiveSerial->print(",    Alarm Code: ");
          ActiveSerial->print( alarm_code, HEX);
          ActiveSerial->print(" --> ");
          printDecodedAlarmString(alarm_code);
          alarmWasFound_b = true;
        }
        
      }
	  }
	}

  // In case of no alarm --> indicate with string
  if (false == alarmWasFound_b)
  {
    ActiveSerial->print("No alarm was found.");
  }

	ActiveSerial->print("\n");
    
	return 1;
}



void Isv57Communication::resetToFactoryParams() 
{
  // Identified with Free Device Monitoring Studio: https://hhdsoftware.com/device-monitoring-studio
  // Data view
  // Write:  3F 03 01 F0 00 01 81 1B
  // Read: 3F 03 02 00 00 91 81

  // Write:  3F 06 01 9A 44 44 9F F4
  // Read:  3F 06 01 9A 44 44 9F F4

  // Write:  3F 03 01 F7 00 01 30 DA
  // Read:  3F 03 02 55 55 6E EE


  // // disable axis first
  // disableAxis();
  // ActiveSerial->println("Disabling axis first\n");
  // delay(500);


  // // identified with logic analyzer. See \StepperParameterization\Meesages\ResetToFactorySettings_0.png
  // long tmp = modbus.holdingRegisterRead(0x01F0);

  // if (tmp == 0x00)
  // {
  //   ActiveSerial->println("First test passed\n");
  //   modbus.writeHoldingRegister(slaveId, 0x019a, 0x4444);

  //   tmp = modbus.holdingRegisterRead(0x01F7);

  //   if (tmp == 0x5555)
  //   {
  //     ActiveSerial->println("Reset to factory settings successfull\n");
  //   }
  // }



  disableAxis();

  bool retValue_b = false;
  
  for (uint16_t registerIndex_u16 = 0; registerIndex_u16 < ISV57_NMB_OF_REGISTERS; registerIndex_u16++)
  {
    retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, registerIndex_u16, tuned_parameters[registerIndex_u16]);
  }



  // store the settings to servos NVM if necesssary
  if (retValue_b)
  {

    ActiveSerial->println("Servo registered in NVM have been updated! Please power cycle the servo and the ESP!");

    // identified with logic analyzer. See \StepperParameterization\Meesages\StoreSettingsToEEPROM_0.png
    modbus.writeHoldingRegisterToDevice(slaveId, 0x019A, 0x5555); // store the settings to servos NVM
    // ToDo: according to iSV57 manual, 0x2211 is the command to write values to EEPROM
    delay(500);
    
    // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x6666 is soft reset
    // modbus.writeHoldingRegister(slaveId, 0x019A, 0x6666); // store the settings to servos NVM
    
    isv57_update_parameter_b=true;
    delay(1000);
  }
  
}

