#pragma once

#include <FastNonAccelStepper.h>
#include "isv57communication.h"
#include "DiyActivePedal_types.h"
#include "Main.h"

// ==============================================================================
// Physical Stepper Properties & Limits
// ==============================================================================
static const int32_t MAXIMUM_STEPPER_ACCELERATION = INT32_MAX / 10; // Maximum acceleration in steps/s²
static const int32_t MAXIMUM_STEPPER_IDLE_TIMEOUT = 1800000;        // Timeout in ms before the servo goes idle
static const float STEPPER_WAKEUP_FORCE = 1.0f;                     // Force in kg required to wake up the servo

// ==============================================================================
// Enumerations
// ==============================================================================
enum ServoStatus
{
    SERVO_NOT_CONNECTED,
    SERVO_CONNECTED,
    SERVO_IDLE_NOT_CONNECTED,
    SERVO_FORCE_STOP
};

class FunctionProfiler; // Forward declaration for the performance profiler

// ==============================================================================
// Class Definition: StepperWithLimits
// Handles the physical stepper motor, virtual endstops, and Modbus communication 
// with the iSV57 servo drive.
// ==============================================================================
class StepperWithLimits {
private:
    FastNonAccelStepper* _stepper;                 // Pointer to the high-speed pulse generator
    int32_t _endstopLimitMin, _endstopLimitMax;    // Absolute stepper positions at the physical hard limits
    int32_t _posMin, _posMax;                      // Configured minimum and maximum travel positions (soft limits)

    Isv57Communication isv57;                      // Handles serial Modbus communication with the servo
    
    // --- Servo State and Control Flags ---
    bool isv57LifeSignal_b = false;                // True if communication with servo is established
    bool invertMotorDir_global_b = false;          // True if the motor direction is mechanically inverted
    int32_t servoPos_i16 = 0;                      // Raw servo position read via Modbus
    int32_t servo_offset_compensation_steps_i32 = 0; // Steps to inject to recover from lost steps
    
    bool restartServo = false;                     // Flag to trigger a servo restart cycle
    bool enableSteplossRecov_b = true;             // Toggles automatic step-loss recovery
    bool enableCrashDetection_b = true;            // Toggles endstop crash detection

    uint16_t posCommandSmoothingFactor_u16 = 0;    // Smoothing parameter for the iSV57 internal trajectory generator

    bool logAllServoParams = false;                // Trigger flag to dump all servo registers to serial
    bool clearAllServoAlarms_b = false;            // Trigger flag to clear servo fault states
    bool resetServoRegistersToFactoryValues_b = false; // Trigger flag to perform a factory reset
    bool updateServoParams_b = false;              // Trigger flag to push new parameters to the servo

    int32_t servoPos_local_corrected_i32 = 0;      // Fully unwrapped and corrected servo position
    uint32_t stepsPerMotorRev_u32 = 3200u;         // Configured microsteps per full motor revolution
    bool brakeResistorState_b = false;             // True if the external brake resistor circuit is active

    // --- Variables Refactored from Global Scope for Thread Safety ---
    int64_t timeNow_isv57SerialCommunicationTask_l = 0;
    int64_t cycleTimeLastCall_lifelineCheck = 0;
    bool previousIsv57LifeSignal_b = true;
    uint32_t servoLastStateCycleCounter_u32 = 0;
    uint32_t servoLastTimeStampInMs_u32 = 0;
    int32_t lastTrackingError_i32 = 0;
    float trackingErrorChangeInStepsPerS_fl32 = 0;
    int16_t servoPos_last_i16 = 0;
    int64_t timeSinceLastServoPosChange_l = 0;
    int64_t timeDiff = 0;

    // --- Private Helper Methods ---
    void setLifelineSignal();
    void processPendingCommands();
    void updateLifeline();
    void processActiveServo(FunctionProfiler* profiler);
    void handleConnectionLoss();
    void readAndFormatServoPosition();
    void unwrapServoPosition();
    void calculateTrackingErrorChange();
    void performSafetyChecks();

public:
    uint8_t servoStatus = 0;                       // Current global state of the servo
    uint8_t endstopDetectionThreshold_u8 = 30;     // Current threshold (in percent) to detect a physical block

    // Constructor
    StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32, uint8_t _endstopDetectionThreshold);
    
    // Core validity check
    bool hasValidStepper() const { return NULL != _stepper; }

    // --- Movement & Limit Setup ---
    void checkLimitsAndResetIfNecessary();
    void updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct);
    void findMinMaxSensorless(DapConfig_t dap_config_st);
    void forceStop();
    int8_t moveTo(int32_t position, bool blocking = false);
    void moveSlowlyToPos(int32_t targetPos_ui32);
    void moveToPosWithSpeed(int32_t targetPos_ui32, uint32_t speedInHz_u32);
    void moveToWithSpeed(int32_t targetPos_i32, uint32_t speed_u32);
    void setSpeedLive(uint32_t speedInHz_u32);
    void setSpeed(uint32_t speedInStepsPerSecond);
    void keepRunningInDir(bool forwardDir_b, uint32_t speed_u32);
    
    // --- Position Getters ---
    int32_t getHardEndstopMinPosition() const { return _endstopLimitMin; }
    int32_t getHardEndstopMaxPosition() const { return _endstopLimitMax; }
    int32_t getCurrentPositionFromMin() const;
    int32_t getMinPosition() const;
    int32_t getCurrentPosition() const;
    float getCurrentPositionFraction() const;
    float getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const;
    int32_t getTargetPositionSteps() const;
    int32_t getLimitMin() const { return _endstopLimitMin; }
    int32_t getLimitMax() const { return _endstopLimitMax; }
    int32_t getTravelSteps() const { return _posMax - _posMin; }
    
    // --- Speed Getters ---
    int32_t getCurrentSpeedInHz();
    uint32_t getMaxSpeedInHz();
    
    // --- Status Checks ---
    bool isRunning();
    bool isAtMinPos();
    bool getLifelineSignal();
    bool getBrakeResistorState();
    float getBrakeResistorActivationVoltage(void);
    
    // --- Advanced Safety & Correction ---
    void correctPos();
    void setMinPosition();
    int32_t getPositionDeviation();
    
    // --- Servo Telemetry Data ---
    int32_t getServosInternalPosition();
    uint32_t getServoCycleCounter();
    int32_t getServosVoltage();
    int32_t getServosCurrent();
    int32_t getServosPos();
    int32_t getServosPosError();
    uint32_t getServoCycleTimestamp();
    int32_t getServosPosErrorChangeRateInStepsPerSecond();
    
    // --- Configuration Triggers ---
    void configSteplossRecovAndCrashDetection(uint8_t flags_u8);
    void printAllServoParameters();
    void clearAllServoAlarms();
    void resetServoParametersToFactoryValues();
    void configSetProfilingFlag(bool proFlag_b);

    // --- Internal Thread-Safe Setters/Getters ---
    void setServosInternalPositionCorrected(int32_t posCorrected_i32);
    int32_t getServosInternalPositionCorrected();

    // --- FreeRTOS Task ---
    static void servoCommunicationTask(void * pvParameters);
    void pauseTask();
    bool servoIdleAction();
};

void setDirection(bool forwardDir_b);