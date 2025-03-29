#ifndef ISV57_COMMUNICATION_H
#define ISV57_COMMUNICATION_H

//#include <SoftwareSerial.h>
#include "Modbus.h"


// define modbus stuff
#define MODE  5





// servo states register addresses
#define reg_add_position_given_p 0x0001 // checked
#define reg_add_position_feedback_p 0x0002 // checked
#define reg_add_position_error_p 0x0003 // checked
#define reg_add_command_position_given_p 0x0004 // checked
#define reg_add_position_relative_error_p 0x0005 // checked
#define reg_add_velocity_given_rpm 0x0040 // checked
#define reg_add_velocity_feedback_rpm 0x0041 // checked
#define reg_add_velocity_error_rpm 0x0042 // checked
#define reg_add_velocity_feedback_no_filt_rpm 0x0048 // checked
#define reg_add_position_command_velocity_rpm 0x0049 // checked
#define reg_add_velocity_current_given_percent 0x0080 // checked
#define reg_add_velocity_current_feedback_percent 0x0081 // checked
#define reg_add_voltage_0p1V 0x0140 // checked


// DC bus voltage: 0B0AH --> 140 = 8c
// position feedback: 0B14H

#define ref_cyclic_read_0 0x01F3
#define ref_cyclic_read_1 0x01F4
#define ref_cyclic_read_2 0x01F5
#define ref_cyclic_read_3 0x01F6

// servo parameter addresses
#define pr_0_00 0x0000 // reserved parameter
#define pr_1_00 0x0000 + 25 // 1st position gain
#define pr_2_00 pr_1_00 + 40 // adaptive filter mode setup
#define pr_3_00 pr_2_00 + 30 // velocity control
#define pr_4_00 pr_3_00 + 30 // velocity torque control
#define pr_5_00 pr_4_00 + 50 // extension settings
#define pr_6_00 pr_5_00 + 40 // special settings
#define pr_7_00 pr_6_00 + 40 // special settings



class isv57communication {
	
	public:
    isv57communication();
    void setupServoStateReading();
    void sendTunedServoParameters(bool commandRotationDirection, uint32_t stepsPerMotorRev_u32);
    void readAllServoParameters();
    void readServoStates();
    bool checkCommunication();
    bool findServosSlaveId();
    bool clearServoAlarms();
    bool readAlarmHistory();
    bool readCurrentAlarm();
    void resetToFactoryParams();
    bool setServoVoltage(uint16_t voltageInVolt_u16);
	
	void clearServoUnitPosition();
    void disableAxis();
	void enableAxis();
    //void resetAxisCounter(); 


    void setZeroPos();
    void applyOfsetToZeroPos(int16_t givenPosOffset_i16);
    int16_t getZeroPos();
    int16_t getPosFromMin();
    int16_t regArray[4];

    int16_t slaveId = 63; 

    int16_t servo_pos_given_p = 0;
    int16_t servo_pos_error_p = 0;
    int16_t servo_current_percent = 0;
    int16_t servo_voltage_0p1V = 0;
    bool isv57_update_parameter_b=false;

  private:
    // declare variables
    uint8_t  raw[200];
    uint8_t len;
    int16_t zeroPos;
    //Modbus modbus;
  
};

#endif
