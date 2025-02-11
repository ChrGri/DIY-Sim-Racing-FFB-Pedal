#include "isv57communication.h"
#include "Main.h"

Modbus modbus(Serial1);


// initialize the communication
isv57communication::isv57communication()
{
  
  //Serial1.begin(38400, SERIAL_8N2, ISV57_RXPIN, ISV57_TXPIN, true); // Modbus serial
  #if PCB_VERSION == 10 || PCB_VERSION == 9 || PCB_VERSION == 12
    Serial1.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, false); // Modbus serial
  #else
    Serial1.begin(38400, SERIAL_8N1, ISV57_RXPIN, ISV57_TXPIN, true); // Modbus serial
  #endif


  modbus.init(MODE);
}




// send tuned servo parameters
void isv57communication::setupServoStateReading() {

  
  // The iSV57 has four registers (0x0191, 0x0192, 0x0193, 0x0194) in which we can write, which values we want to obtain cyclicly
  // These registers can be obtained by sending e.g. the command: 0x63, 0x03, 0x0191, target_sate, CRC
  // tell the modbus slave, which registers will be read cyclicly
  modbus.checkAndReplaceParameter(slaveId, 0x0191, reg_add_position_given_p);
  modbus.checkAndReplaceParameter(slaveId, 0x0192, reg_add_velocity_current_feedback_percent);
  modbus.checkAndReplaceParameter(slaveId, 0x0193, reg_add_position_error_p);
  modbus.checkAndReplaceParameter(slaveId, 0x0194, reg_add_voltage_0p1V);

}


void isv57communication::readAllServoParameters() {
  for (uint16_t reg_sub_add_u16 = 0;  reg_sub_add_u16 < (pr_7_00+49); reg_sub_add_u16++)
  {
    modbus.readParameter(slaveId, pr_0_00 + reg_sub_add_u16);
  }
}

// Disable aixs command
void isv57communication::disableAxis()
{

  Serial.println("Disabling servo");

  // 0x3f, 0x06, 0x00, 0x85, 0x03, 0x03, 0xdc, 0x0c
  //modbus.checkAndReplaceParameter(slaveId, 0x0085, 0x0303);
  modbus.holdingRegisterWrite(slaveId, 0x0085, 0x0303);
  // 0x3f, 0x06, 0x01, 0x39, 0x00, 0x00, 0x5c, 0xe5
  //modbus.checkAndReplaceParameter(slaveId, 0x0139, 0x0000); 
  modbus.holdingRegisterWrite(slaveId, 0x0139, 0x0008);
  delay(30);

  // read routine
  modbus.holdingRegisterRead(0x0085);
  modbus.holdingRegisterRead(0x0139);
  delay(5);
}

void isv57communication::enableAxis() 
{
  Serial.println("Enabling servo");

  // 0x3f, 0x06, 0x00, 0x85, 0x03, 0x83, 0xdd, 0xac
  // Pr4.08: 0x085
  modbus.holdingRegisterWrite(slaveId, 0x0085, 0x0383);
  // 0x3f, 0x06, 0x01, 0x39, 0x00, 0x08, 0x5d, 0x23
  modbus.holdingRegisterWrite(slaveId, 0x0139, 0x0008);
  delay(30);

  // read routine
  modbus.holdingRegisterRead(0x0085);
  modbus.holdingRegisterRead(0x0139);
  delay(5);

  // modbus.holdingRegisterRead(0x0085);
  // modbus.holdingRegisterRead(0x0139);
  
}


// void isv57communication::resetAxisCounter() 
// {
//   Serial.println("Reset axis counter");

//   modbus.holdingRegisterRead(0x0085);
//   delay(10);
//   modbus.holdingRegisterRead(0x0139);
//   delay(10);
  
// }







void  isv57communication::clearServoUnitPosition()
{
	// According to Leadshines User Manual of 2ELD2-RD DC Servo
	// https://www.leadshine.com/upfiles/downloads/a3d7d12a120fd8e114f6288b6235ac1a_1690179981835.pdf
	// Changing the position unit, will clear the position data

  modbus.checkAndReplaceParameter(slaveId, pr_5_00+20, 0); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  delay(100);
	modbus.checkAndReplaceParameter(slaveId, pr_5_00+20, 1); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  delay(100);
}

bool isv57communication::setServoVoltage(uint16_t voltageInVolt_u16)
{
  return modbus.checkAndReplaceParameter(slaveId, pr_7_00+32, voltageInVolt_u16 + 4); // bleeder braking voltage. Voltage when braking is activated
}

// send tuned servo parameters
void isv57communication::sendTunedServoParameters(bool commandRotationDirection, uint32_t stepsPerMotorRev_u32) {
  
  bool retValue_b = false;


  // Pr0 register
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+1, 0); // control mode #
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+2, 0); // deactivate auto gain
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+3, 10); // machine stiffness
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+4, 80); // ratio of inertia
  //retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+6, commandRotationDirection); // Command Pulse Rotational Direction
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+8, (long)stepsPerMotorRev_u32); // microsteps
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+9, 1); // 1st numerator 
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+10, 1); // & denominator
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+13, 500); // 1st torque limit
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+14, 500); // position deviation setup
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+16, 500); // regenerative braking resitor
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+17, 500);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+18, 0); // vibration suppression
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_0_00+19, 0);

  // Pr1 register
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+0, 600); // 1st position gain
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+1, 300); // 1st velocity loop gain
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+2, 300); // 1st time constant of velocity loop
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+3, 15); // 1st filter of velocity detection
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+4, 150); // 1st torque filter
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+10, 200); // velocity feed forward gain
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+11, 6000); // velocity feed forward filter
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+12, 0); // torque feed forward gain
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+13, 0); // torque feed forward filter
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+15, 0); // control switching mode
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+33, 0); // speed given filter
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+35, 0); // position command filter
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+36, 0); // encoder feedback
  //retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+37, 1052); // special function register
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_1_00+37, 4); // special function register
  // see https://www.oyostepper.com/images/upload/File/ISV57T-180.pdf
  // 0x01 = 1: velocity feedforward disabled
  // 0x02 = 2: torque feedforward disabled
  // 0x04 = 4: motor overspeed alarm disabled
  // 0x08 = 8: position following alarm disabled
  // 0x10 = 16: overload alarm disabled
  // 0x400 = 1024: undervoltage disabled

  // Pr2 register
  // vibration suppression 
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+1, 50);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+2, 20);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+3, 99);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+4, 90);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+5, 20);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+6, 99);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+22, 0);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_2_00+23, 0);// FIR based command smoothing time. Since the stpper task runs every 4ms, this time is selected to be larger than that. Unit is 0.1ms 
  

  // Pr3 register
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_3_00+24, 5000); // maximum rpm

  // Pr5 register
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_5_00+13, 5000); // overspeed level
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_5_00+20, 1); // encoder output resolution  {0: Encoder units; 1: Command units; 2: 10000pulse/rotation}
  

  //retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_5_00+32, 300); // command pulse input maximum setup


  // Enable & tune reactive pumping. This will act like a braking resistor and reduce EMF voltage.
  // See https://en.wikipedia.org/wiki/Bleeder_resistor
  // Info from iSV2 manual: The external resistance is activated when the actual bus voltage is higher than Pr7.32 plus Pr7.33 and is deactivated when the actual bus voltage is lower than Pr7.32 minus Pr7.33
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+31, 0); // bleeder control mode; 0: is default and seems to enable braking mode, contrary to manual
  //retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+32, 42); // bleeder braking voltage. Voltage when braking is activated
  retValue_b |= setServoVoltage(SERVO_MAX_VOLTAGE_IN_V_36V);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+33, 1); // bleeder hysteresis voltage; Contrary to the manual this seems to be an offset voltage, thus Braking disabling voltage = Pr7.32 + Pr.33
  
  

  // retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+28, 1000);
  // retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+29, 100);
  
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+28, 1000);
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_7_00+29, 10);


  //retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_5_00+33, 0); // pulse regenerative output limit setup [0,1]
  // retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_6_00+37, 0); // oscillation detection level [0, 1000] 0.1%


  // disable axis after servo startup --> ESP has to enable the axis first
  // Pr4.08
  // long servoEnableStatus = modbus.holdingRegisterRead(slaveId, 0x03, pr_4_00+8);
  // Serial.print("Servo enable setting: ");
  // Serial.println(servoEnableStatus, HEX);
  // delay(100);
  // if (servoEnableStatus != 0x303)
  // {
  //   isv57communication::disableAxis();
  // }
  // delay(100);
  // servoEnableStatus = modbus.holdingRegisterRead(slaveId, 0x03, pr_4_00+8);
  // Serial.print("Servo enable setting: ");
  // Serial.println(servoEnableStatus, HEX);

  // disable axis by default
  retValue_b |= modbus.checkAndReplaceParameter(slaveId, pr_4_00+8, 0x0303);
  


  // store the settings to servos NVM if necesssary
  if (retValue_b)
  {
    // disable axis a second time, since the second signal must be send to. Don't know yet the meaning of that signal.
    disableAxis();

    Serial.println("Servo registered in NVM have been updated! Please power cycle the servo and the ESP!");

    // identified with logic analyzer. See \StepperParameterization\Meesages\StoreSettingsToEEPROM_0.png
    modbus.holdingRegisterWrite(slaveId, 0x019A, 0x5555); // store the settings to servos NVM
    // ToDo: according to iSV57 manual, 0x2211 is the command to write values to EEPROM
    delay(500);
    
    
    // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x6666 is soft reset
    // modbus.holdingRegisterWrite(slaveId, 0x019A, 0x6666); // store the settings to servos NVM
    
    
    isv57_update_parameter_b=true;
    delay(1000);
  }


}

bool isv57communication::findServosSlaveId()
{
  bool slaveIdFound = false;

  // typically the servo address is 63, so start with that
  int slaveIdTest = 63;
  if(modbus.requestFrom(slaveIdTest, 0x03, 0x0000, 2) > 0)
  {
    slaveId = slaveIdTest;
    slaveIdFound = true;
    Serial.print("Found servo slave ID:");
    Serial.print(slaveId);
    Serial.print("\r\n");
  }


  if (false == slaveIdFound )
  {
    for (int slaveIdTest = 0; slaveIdTest<256; slaveIdTest++)
    {
        if(modbus.requestFrom(slaveIdTest, 0x03, 0x0000, 2) > 0)
        {
          slaveId = slaveIdTest;
          slaveIdFound = true;
          Serial.print("Found servo slave ID:");
          Serial.print(slaveId);
          Serial.print("\r\n");
          break;
        }

        delay(5);
    }
  }
  
  return slaveIdFound;
}




bool isv57communication::checkCommunication()
{
  if(modbus.requestFrom(slaveId, 0x03, 0x0000, 2) > 0)
  {
    //Serial.println("Lifeline check: true");
    return true;
  }
  else
  {
    //Serial.println("Lifeline check: false");
    return false;
  }
}



void isv57communication::setZeroPos()
{
  zeroPos = servo_pos_given_p;
}

void isv57communication::applyOfsetToZeroPos(int16_t givenPosOffset_i16)
{
  zeroPos += givenPosOffset_i16;
}

int16_t isv57communication::getZeroPos()
{
  return zeroPos;
}

int16_t isv57communication::getPosFromMin()
{
  return servo_pos_given_p - zeroPos;
}


// read servo states
void isv57communication::readServoStates() {

  // initialize with -1 to indicate non-trustworthyness
  regArray[0] = -1;
  regArray[1] = -1;
  regArray[2] = -1;
  regArray[3] = -1;

  // read the four registers simultaneously
  int8_t numberOfRegistersToRead_u8 = 4;
  int bytesReceived_i = modbus.requestFrom(slaveId, 0x03, ref_cyclic_read_0, numberOfRegistersToRead_u8);
  if(bytesReceived_i == (numberOfRegistersToRead_u8*2))
  {
    modbus.RxRaw(raw,  len);
    for (uint8_t regIdx = 0; regIdx < numberOfRegistersToRead_u8; regIdx++)
    { 
      regArray[regIdx] = modbus.uint16(regIdx);
    }
  }

  // write to public variables
  servo_pos_given_p = regArray[0];
  servo_current_percent = regArray[1];
  servo_pos_error_p = regArray[2];
  servo_voltage_0p1V = regArray[3];
  
  //Serial.print("Bytes :");
  //Serial.println(bytesReceived_i);
  
  
  
  // print registers
  if (0)
  {
    Serial.print("Pos_given:");
    Serial.print(servo_pos_given_p);

    Serial.print(",Pos_error:");
    Serial.print(servo_pos_error_p);

    Serial.print(",Cur_given:");
    Serial.print(servo_current_percent);

    Serial.print(",Voltage:");
    Serial.print(servo_voltage_0p1V);

    Serial.println(" "); 
  }
  
}



bool isv57communication::clearServoAlarms() {

  // read the alarm list
  int8_t numberOfRegistersToRead_u8 = 0;
  // Alarm register address: 0x02
  //int bytesReceived_i = modbus.requestFrom(slaveId, 0x03, 0x02, numberOfRegistersToRead_u8);

  // clear alarm list
  modbus.holdingRegisterWrite(slaveId, 0x019a, 0x7777); 
  
  
  // ToDo: soft reset servo. The iSV57 docu says Pr0.25: 0x1111 resets current alarm; 0x1122 resets alarm history
    
  return 1;
}


bool isv57communication::readCurrentAlarm() {
  int bytesReceived_i = modbus.requestFrom(slaveId, 0x03, 0x01F2, 1);
  if(bytesReceived_i == (2))
  {
    modbus.RxRaw(raw,  len);
    for (uint8_t regIdx = 0; regIdx < 1; regIdx++)
    { 
      uint16_t tmp = modbus.uint16(regIdx) && 0x0FFF; // mask the first half byte as it does not contain info
      Serial.print("Current iSV57 alarm: ");
      Serial.println( tmp, HEX);
    }
  }

  return 1;
}


bool isv57communication::readAlarmHistory() {

	// 
	Serial.println("\niSV57 alarm history: ");
	for (uint8_t idx=0; idx < 12; idx++)
	{
	  // example signal, read the 9th alarm
	  // 0x3f, 0x03, 0x12, 0x09, 0x00, 0x01, 0x55, 0xAE

	  // read the four registers simultaneously
	  int bytesReceived_i = modbus.requestFrom(slaveId, 0x03, 0x1200 + idx, 1);
	  if(bytesReceived_i == (2))
	  {
		modbus.RxRaw(raw,  len);
		for (uint8_t regIdx = 0; regIdx < 1; regIdx++)
		{ 
		  uint16_t tmp = modbus.uint16(regIdx) & 0x0FFF; // mask the first half byte as it does not contain info

		  if (tmp > 0)
		  {
			Serial.print("Alarm Idx: ");
			Serial.print(idx);
			Serial.print(",    Alarm Code: ");
			Serial.println( tmp, HEX);
		  }
		  
		}
	  }
	}
	Serial.print("\n");
    
	return 1;
}

void isv57communication::resetToFactoryParams() 
{
  // identified with logic analyzer. See \StepperParameterization\Meesages\ResetToFactorySettings_0.png
	//modbus.holdingRegisterWrite(slaveId, 0x01F0, 0x0001);
  // 0x3f, 0x03, 0x01, 0xF0, 0x00, 0x01, 0x81, 0x1B
  modbus.holdingRegisterRead(0x01F0);
}

