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
  #if defined(ISV57_TXPIN) && (ISV57_TXPIN >= 0)
    pinMode(ISV57_TXPIN, OUTPUT);
    digitalWrite(ISV57_TXPIN, HIGH); // Assert idle UART state before serial controller init
  #endif
  #if defined(ISV57_RXPIN) && (ISV57_RXPIN >= 0)
    pinMode(ISV57_RXPIN, INPUT_PULLUP); // Pull up RX line to prevent floating UART noise
  #endif

  #if PCB_VERSION == 10 || PCB_VERSION == 9 || PCB_VERSION == 12 || PCB_VERSION == 13 || PCB_VERSION == 14
    Serial2.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, false); // Modbus serial
  #else
    Serial2.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, true); // Modbus serial
  #endif

  Serial2.setTimeout(12); // 12 ms timeout for ISR-driven non-blocking reception

  ActiveSerialForServoCommunication = &Serial2;

  modbus.initialize(true);
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
  modbus.readHoldingRegisterFromDevice(0x0085);
  modbus.readHoldingRegisterFromDevice(0x0139);
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
  modbus.readHoldingRegisterFromDevice(0x0085);
  modbus.readHoldingRegisterFromDevice(0x0139);
  delay(5);
}
  // modbus.holdingRegisterRead(0x0085);
  // modbus.holdingRegisterRead(0x0139);


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


// send tuned servo parameters
void Isv57Communication::sendTunedServoParameters(bool commandRotationDirection, uint32_t stepsPerMotorRev_u32) {
  
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
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+0, tuned_parameters[pr_0_00+0]); // control mode

  // according to the iSV2 manual chapter 5.6, the model following control (MFC) parameter should be larger then Pr1.01, velocity loop gain
  // float mfcLowerLimit_fl32 = tuned_parameters[pr_1_00+1] ;
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+1, tuned_parameters[pr_0_00+1]); // control mode #
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+6, tuned_parameters[pr_0_00+6]); // motor command direction
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+8, (long)stepsPerMotorRev_u32); // microsteps
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+9, tuned_parameters[pr_0_00+9]); // 1st numerator 
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+10, tuned_parameters[pr_0_00+10]); // & denominator
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_0_00+14, tuned_parameters[pr_0_00+14]); // position deviation setup

  // Pr1 register
  //uint16_t special_function_flags = 0x4 | 0x8 | 0x10 | 0x40 | 0x400;
  uint16_t special_function_flags = 0x4 | 0x8 | 0x10 | 0x20| 0x400;
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
  
  // Pr4 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_4_00+10, tuned_parameters[pr_4_00+10]); // Alarm port signal

  // Pr5 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+20, tuned_parameters[pr_5_00+20]); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_5_00+35, 1); // lock front panel
  
  // Pr7 register
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+0, tuned_parameters[pr_7_00+0]); // current loop gain
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+1, tuned_parameters[pr_7_00+1]); // current loop integral time

  // Enable & tune reactive pumping. This will act like a braking resistor and reduce EMF voltage.
  // See https://en.wikipedia.org/wiki/Bleeder_resistor
  // Info from iSV2 manual: The external resistance is activated when the actual bus voltage is higher than Pr7.32 plus Pr7.33 and is deactivated when the actual bus voltage is lower than Pr7.32 minus Pr7.33
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+31, tuned_parameters[pr_7_00+31]); // bleeder control mode; 0: is default and seems to enable braking mode, contrary to manual
  retValue_b |= setServoVoltage(SERVO_MAX_VOLTAGE_IN_V_36V);
  retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, pr_7_00+33, tuned_parameters[pr_7_00+33]); // bleeder hysteresis voltage; Contrary to the manual this seems to be an offset voltage, thus Braking disabling voltage = Pr7.32 + Pr.33

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


// read servo states (immediate UART ISR-driven reception)
void Isv57Communication::readServoStates() {
  const uint8_t expectedRegisters = NUMBER_OF_ISV57_REGISTERS_TO_READ_IN_CYCLIC_READ; // 4
  const uint8_t expectedDataBytes = expectedRegisters * 2; // 8 bytes
  const uint8_t expectedTotalBytes = 1 + 1 + 1 + expectedDataBytes + 2; // 13 bytes

  // 1. Drain any stale noise from RX FIFO before transmitting request
  while (Serial2.available() > 0) {
    Serial2.read();
  }

  // 2. Transmit pre-assembled 8-byte request frame directly to hardware UART TX FIFO (< 1 µs)
  if (!cachedCyclicTxValid_ || cachedSlaveId_ != slaveId) {
    cachedCyclicTxFrame_[0] = (uint8_t)slaveId;
    cachedCyclicTxFrame_[1] = 0x03;
    cachedCyclicTxFrame_[2] = (ref_cyclic_read_0 >> 8) & 0xFF;
    cachedCyclicTxFrame_[3] = ref_cyclic_read_0 & 0xFF;
    cachedCyclicTxFrame_[4] = 0x00;
    cachedCyclicTxFrame_[5] = expectedRegisters;
    int32_t crc = Modbus::computeCrc(cachedCyclicTxFrame_, 6);
    cachedCyclicTxFrame_[6] = crc & 0xFF;
    cachedCyclicTxFrame_[7] = (crc >> 8) & 0xFF;
    cachedCyclicTxValid_ = true;
    cachedSlaveId_ = slaveId;
  }

  Serial2.write(cachedCyclicTxFrame_, 8);

  // 3. Block on FreeRTOS UART ring buffer (0% CPU load).
  // The hardware UART ISR unblocks this task the microsecond the 13th byte arrives!
  uint8_t rxBuffer[16];
  size_t bytesReceived = Serial2.readBytes((char*)rxBuffer, expectedTotalBytes);

  isv57dynamicStates_.servo_receivedPacketIsValid_b = false;

  // 4. Verify frame and decode immediately upon arrival
  if (bytesReceived == expectedTotalBytes) {
    if (rxBuffer[0] == (uint8_t)slaveId && rxBuffer[1] == 0x03 && rxBuffer[2] == expectedDataBytes) {
      int32_t receivedCrc = ((uint16_t)rxBuffer[expectedTotalBytes - 1] << 8) | rxBuffer[expectedTotalBytes - 2];
      int32_t computedCrc = Modbus::computeCrc(rxBuffer, expectedTotalBytes - 2);

      if (receivedCrc == computedCrc) {
        for (uint8_t regIdx = 0; regIdx < expectedRegisters; regIdx++) {
          regArray[regIdx] = (int16_t)(((uint16_t)rxBuffer[3 + regIdx * 2] << 8) | rxBuffer[3 + regIdx * 2 + 1]);
        }

        // Update dynamic states immediately
        isv57dynamicStates_.servo_pos_given_p = regArray[0];
        isv57dynamicStates_.servo_current_percent = regArray[1];
        isv57dynamicStates_.servo_pos_error_p = regArray[2];
        isv57dynamicStates_.servoVoltage0p1V_i16 = regArray[3];

        isv57dynamicStates_.lastUpdateTimeInMS_u32 = millis();
        isv57dynamicStates_.servo_cycleCounter_u32++;
        isv57dynamicStates_.servo_receivedPacketIsValid_b = (isv57dynamicStates_.servoVoltage0p1V_i16 >= 50);
      }
    }
  }
}

int Isv57Communication::readRegisters(
        uint16_t startAddr_u16, uint8_t count_u8, int16_t* out_pi16)
{
    if (count_u8 == 0 || out_pi16 == nullptr) return -1;

    int bytesReceived = modbus.sendRequestAndReceiveResponse(
        slaveId, 0x03, (int32_t)startAddr_u16, (int32_t)count_u8);

    if (bytesReceived != (count_u8 * 2)) return -1;

    modbus.getRawRxBuffer(raw, len);
    for (uint8_t i = 0; i < count_u8; i++) {
        out_pi16[i] = modbus.convertRxBufferToInt16(i);
    }
    return (int)count_u8;
}

int32_t Isv57Communication::writeHoldingRegisterToDevice(
        int32_t slaveId_i32, int32_t registerAddress_i32, uint16_t value_u16)
{
    return modbus.writeHoldingRegisterToDevice(slaveId_i32, registerAddress_i32, value_u16);
}

int32_t Isv57Communication::writeHoldingRegistersToDevice(
        int32_t slaveId_i32, int32_t registerAddress_i32, uint16_t* values_u16, uint8_t count_u8)
{
    return modbus.writeHoldingRegistersToDevice(slaveId_i32, registerAddress_i32, values_u16, count_u8);
}



bool Isv57Communication::clearServoAlarms() {
  // Command 0x019A = 0x1111 clears active servo alarms (Pr0.25 in iSV57 manual)
  return (modbus.writeHoldingRegisterToDevice(slaveId, 0x019A, 0x1111) > 0);
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


  // disable axis first
  disableAxis();
  ActiveSerial->println("Disabling axis first\n");
  delay(1000);

  // identified with logic analyzer. See \StepperParameterization\Meesages\ResetToFactorySettings_0.png
  long tmp = modbus.readHoldingRegisterFromDevice(0x01F0);

  if (tmp == 0x00)
  {
    ActiveSerial->println("First test passed\n");
    modbus.writeHoldingRegisterToDevice(slaveId, 0x019a, 0x4444);

    tmp = modbus.readHoldingRegisterFromDevice(0x01F7);

    if (tmp == 0x5555)
    {
      ActiveSerial->println("Reset to factory settings successfull\n");
    }
  }



  // disableAxis();

  // bool retValue_b = false;
  
  // for (uint16_t registerIndex_u16 = 0; registerIndex_u16 < ISV57_NMB_OF_REGISTERS; registerIndex_u16++)
  // {
  //   retValue_b |= modbus.writeAndVerifyDeviceParameter(slaveId, registerIndex_u16, tuned_parameters[registerIndex_u16]);
  // }



  // // store the settings to servos NVM if necesssary
  // if (retValue_b)
  // {

  //   ActiveSerial->println("Servo registered in NVM have been updated! Please power cycle the servo and the ESP!");

  //   // identified with logic analyzer. See \StepperParameterization\Meesages\StoreSettingsToEEPROM_0.png
  //   modbus.writeHoldingRegisterToDevice(slaveId, 0x019A, 0x5555); // store the settings to servos NVM
  //   // ToDo: according to iSV57 manual, 0x2211 is the command to write values to EEPROM
  //   delay(500);
    
  //   // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x6666 is soft reset
  //   // modbus.writeHoldingRegister(slaveId, 0x019A, 0x6666); // store the settings to servos NVM
    
  //   isv57_update_parameter_b=true;
  //   delay(1000);
  // }
  
}
