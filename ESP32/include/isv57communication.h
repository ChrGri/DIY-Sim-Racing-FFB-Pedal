#ifndef ISV57_COMMUNICATION_H
#define ISV57_COMMUNICATION_H

//#include <SoftwareSerial.h>
#include "Modbus.h"

#define NUMBER_OF_ISV57_REGISTERS_TO_READ_IN_CYCLIC_READ 4 // 4 registers to read servo states. Tried 5 but wasn't working.


// registers identified from Stepperonline app vialogic analyzer
// Info can also be seen here https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/main/StepperParameterization/README_state_decoding 

// servo states register addresses
#define reg_add_position_given_p 0x0001 // "PositionGiven(p)"
#define reg_add_position_feedback_p 0x0002 // "PositionFeedback(p)"
#define reg_add_position_error_p 0x0003 // "PositionError(p)"
#define reg_add_command_position_given_p 0x0004 // "CommandPositionGiven(p)"

#define reg_add_position_relative_error_p 0x0005 // "PositonRelativeError(p)"
#define reg_add_velocity_given_rpm 0x0040 // "VelocityGiven(rpm)": command velocity in rpm, output by the position controller, positive for forward direction and negative for backward direction
#define reg_add_velocity_feedback_rpm 0x0041 // "VelocityFeedback(rpm)": feedback velocity in rpm, positive for forward direction and negative for backward direction
#define reg_add_velocity_error_rpm 0x0042 // "VelocityError(rpm)": 

#define reg_add_velocity_feedback_no_filt_rpm 0x0048 // "VelocityFeedbackNoFilt(rpm)": sime to reg_add_velocity_feedback_rpm, but without filter, so more noisy but also more responsive
#define reg_add_position_command_velocity_rpm 0x0049 // "PositionCommandVelocity(rpm)": velocity that the position controller is commanding, in rpm, positive for forward direction and negative for backward direction
#define reg_add_internal_position_command_velocity_rpm 0x004A // "InternalPositionCommandVelocity(rpm)": internal velocity command of the position controller, in rpm, positive for forward direction and negative for backward direction
#define reg_add_velocity_current_given_percent 0x0080 // "CurrentGiven(%)": current command in percent, positive for forward direction and negative for backward direction

#define reg_add_velocity_current_feedback_percent 0x0081 // "CurrentFeedback(%)": current feedback in percent, positive for forward direction and negative for backward direction
#define reg_add_current_complexor_percent 0x0000 // "CurrentComplexor(%)": complexor current in percent, positive for forward direction and negative for backward direction
#define reg_add_u_axis_current_ampere 0x00C0 // "UAxisCurrent(A)": current in ampere, positive for forward direction and negative for backward direction
#define reg_add_v_axis_current_ampere 0x00C1 // "VAxisCurrent(A)": current in ampere, positive for forward direction and negative for backward direction
#define reg_add_w_axis_current_ampere 0x00C2 // "WAxisCurrent(A)": current in ampere, positive for forward direction and negative for backward direction

#define reg_add_voltage_0p1V 0x0140 // "DCBusVoltage(0.1V)": voltage in 0.1V units
#define reg_add_analog1_0p01V 0x0180 // "Analog1(0.01V)": analog input 1 in 0.01V units
#define reg_add_analog2_0p01V 0x0181 // "Analog2(0.01V)": analog input 1 in 0.01V units
#define reg_add_analog3_0p01V 0x0182 // "Analog3(0.01V)": analog input 1 in 0.01V units

#define reg_add_velocity_feedforward_in_rpm 0x0043 // "VelocityFeedForwardIn(rpm)": signal after Pr1.10 but before Pr1.11
#define reg_add_velocity_feedforward_out_rpm 0x0044 // "VelocityFeedForwardOut(rpm)": signal after Pr1.11
#define reg_add_velocity_smooth_in_rpm 0x0045 // "VelocitySmoothIn(rpm)": Pr1.03 input signal
#define reg_add_velocity_smooth_out_rpm 0x0046 // "VelocitySmoothOut(rpm)": Pr1.03 output signal

#define reg_add_internal_text_chunnel1 0x01C0 // "InternalTextChannel1"
#define reg_add_internal_text_chunnel2 0x01C1 // "InternalTextChannel2"
#define reg_add_internal_text_chunnel3 0x01C2 // "InternalTextChannel3"
#define reg_add_internal_text_chunnel4 0x01C3 // "InternalTextChannel4"
#define reg_add_internal_text_chunnel5 0x01C4 // "InternalTextChannel5"
#define reg_add_internal_text_chunnel6 0x01C5 // "InternalTextChannel6"


// velocity give: asked velocity in rpm, positive for forward direction and negative for backward direction
// position command velocity: velocity asked for but in encoder units
// velocity smooth in: was always 0, perhaps in velocity mode, but in position mode it is not used, so we can ignore it for now. If we want to use it in the future, we can set it to a value between 0 and 10, where 0 means no smoothing and 10 means maximum smoothing. The iSV57 docu says that the default value is 0, but in the tuned parameters file it is set to 10, so we will set it to 10 for now. We can always change it later if we want to experiment with it.
// velocity smooth out: --"--
// velocity feed forward in: signal after Pr1.10 but before Pr1.11
// velocity feed forward out: signal after Pr1.11
// velocity feedback no filt: signal before velocity feedback filter Pr1.03, so more noisy but also more responsive, can be used for advanced control algorithms in the future, e.g. for feed forward control or for model predictive control
// velocity feedback: signal after velocity feedback filter Pr1.03, so less noisy but also less responsive, can be used for advanced control algorithms in the future, e.g. for feed forward control or for model predictive control, but for now we will use the velocity feedback without filter for the control algorithm and the velocity feedback with filter for monitoring and debugging purposes
// velocity error: 
// command position give: propably position given to servo, but in encoder units
// position feedback: propably position feedback from servo, in step units
// 


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



struct isv57dynamicStates {
    int32_t servo_cycleCounter_u32 = 0;
    int16_t servo_pos_given_p = 0;
    int16_t servo_pos_error_p = 0;
    int16_t servo_current_percent = 0;
    int16_t servoVoltage0p1V_i16 = 0;
    int16_t servo_velocity_given_rpm_i16 = 0;
    int16_t servo_position_feedback_i16 = 0;
    // int16_t estimated_pos_error_currentStepperPos_i16 = 0;
    unsigned long lastUpdateTimeInMS_u32 = 0;
    bool servo_receivedPacketIsValid_b = false;
};

class Isv57Communication {
	
	public:
    Isv57Communication();
    void setupServoStateReading();
    void sendTunedServoParameters(bool commandRotationDirection, uint32_t stepsPerMotorRev_u32, uint32_t ratioOfInertia_u32);
    void readAllServoParameters();
    void readServoStates();
    bool checkCommunication();
    bool findServosSlaveId();
    bool clearServoAlarms();
    bool readAlarmHistory();
    bool readCurrentAlarm();
    void resetToFactoryParams();
    bool setServoVoltage(uint16_t voltageInVolt_u16);
    bool setPositionSmoothingFactor(uint16_t posSmoothingFactor_u16);
    bool setRatioOfInertia(uint8_t ratiOfInertia_u8);
	
	void clearServoUnitPosition();
    void disableAxis();
	void enableAxis();
    //void resetAxisCounter(); 


    void setZeroPos();
    void applyOfsetToZeroPos(int16_t givenPosOffset_i16);
    int16_t getZeroPos();
    int16_t getPosFromMin();
    int32_t getServoCycleCounter();
    uint32_t getServoCycleTimestamp();
    int16_t regArray[NUMBER_OF_ISV57_REGISTERS_TO_READ_IN_CYCLIC_READ];

    int16_t slaveId = 63; 

    isv57dynamicStates isv57dynamicStates_;

    bool isv57_update_parameter_b=false;

  private:
    // declare variables
    uint8_t  raw[200];
    uint8_t len;
    int16_t zeroPos;
    bool printProfilingFlag_b;
    //Modbus modbus;
  
};

#endif
