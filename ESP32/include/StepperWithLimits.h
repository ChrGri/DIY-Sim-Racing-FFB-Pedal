#pragma once

#include <FastNonAccelStepper.h>
#include "isv57communication.h"
#include "DiyActivePedal_types.h"
#include "Main.h"

// these are physical properties of the stepper
static const int32_t MAXIMUM_STEPPER_ACCELERATION = INT32_MAX / 10;
static const int32_t MAXIMUM_STEPPER_IDLE_TIMEOUT = 1800000; //set the servo idle timeout      
static const float STEPPER_WAKEUP_FORCE = 1.0f; //set the servo wakeup force in kg.
 // steps/s²
// 10000000; //
enum ServoStatus
{
	SERVO_NOT_CONNECTED,
	SERVO_CONNECTED,
	SERVO_IDLE_NOT_CONNECTED,
	SERVO_FORCE_STOP
};

class StepperWithLimits {
private:
	FastNonAccelStepper* _stepper;
	int32_t _endstopLimitMin, _endstopLimitMax;    // stepper position at limit switches
	int32_t _posMin,   _posMax;      // stepper position at min/max of travel

	Isv57Communication isv57;
	
	
	bool isv57LifeSignal_b = false;
	bool invertMotorDir_global_b = false;
	int32_t servoPos_i16 = 0;
	int32_t servo_offset_compensation_steps_i32 = 0;

	bool restartServo = false;
	void setLifelineSignal();

	bool enableSteplossRecov_b = true;
	bool enableCrashDetection_b = true;

	uint16_t posCommandSmoothingFactor_u16 = 0;
	uint8_t ratioOfInertia_u8 = 1;

	bool logAllServoParams = false;
	bool clearAllServoAlarms_b = false;
	bool resetServoRegistersToFactoryValues_b = false;

	bool updateServoParams_b = false;

	int32_t servoPos_local_corrected_i32 = 0;

	uint32_t stepsPerMotorRev_u32 = 3200u;
	bool brakeResistorState_b = false;

	

public:
	StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32, uint8_t ratioOfInertia_arg_u8, uint8_t _endstopDetectionThreshold);
	bool hasValidStepper() const { return NULL != _stepper; }

	void checkLimitsAndResetIfNecessary();
	void updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct);
	void pauseTask();
	bool isAtMinPos();
	void correctPos();
	void findMinMaxSensorless(DapConfig_t dap_config_st);
	void forceStop();
	int8_t moveTo(int32_t position, bool blocking = false);
	void moveSlowlyToPos(int32_t targetPos_ui32);
	void moveToPosWithSpeed(int32_t targetPos_ui32, uint32_t speedInHz_u32);

	int32_t getCurrentPositionFromMin() const;
	int32_t getMinPosition() const;
	void setMinPosition();
	int32_t getCurrentPosition() const;
	float getCurrentPositionFraction() const;
	float getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const;
	int32_t getTargetPositionSteps() const;
	int32_t getCurrentSpeedInMilliHz();
	uint32_t getMaxSpeedInMilliHz();

	int32_t getLimitMin() const { return _endstopLimitMin; }
	int32_t getLimitMax() const { return _endstopLimitMax; }
	int32_t getTravelSteps() const { return _posMax - _posMin; }
	void setSpeed(uint32_t speedInStepsPerSecond);

	int32_t getPositionDeviation();
	int32_t getServosInternalPosition();
	//int32_t getStepLossCompensation();
	int32_t getServosVoltage();
	int32_t getServosCurrent();
	int32_t getServosPos();
	int32_t getServosPosError();
	int32_t getEstimatedPosError();
	//int32_t getEstimatedPosError_getCurrentStepperPos();
	
	bool getLifelineSignal();
	
	void configSteplossRecovAndCrashDetection(uint8_t flags_u8);
	void configSetPositionCommandSmoothingFactor(uint8_t posCommandSmoothingFactorArg_u8);
	void configSetRatioOfInertia(uint8_t ratioOfInertia_arg_u8);
	void printAllServoParameters();
	void clearAllServoAlarms();
	void resetServoParametersToFactoryValues();
	void configSetProfilingFlag(bool proFlag_b);


	void setServosInternalPositionCorrected(int32_t posCorrected_i32);
	int32_t getServosInternalPositionCorrected();


	static void servoCommunicationTask( void * pvParameters );
	bool getBrakeResistorState();
	
	bool servoIdleAction();
	uint8_t servoStatus=0;
	uint8_t endstopDetectionThreshold_u8=30;

	

};
