#include <FastAccelStepper.h>
#include "isv57communication.h"
#include "DiyActivePedal_types.h"
#include "Main.h"

// these are physical properties of the stepper
static const int32_t MAXIMUM_STEPPER_ACCELERATION = INT32_MAX / 10;                                                 // steps/s²
// 10000000; //

class StepperWithLimits {
private:
	FastAccelStepper* _stepper;
	int32_t _endstopLimitMin, _endstopLimitMax;    // stepper position at limit switches
	int32_t _posMin,   _posMax;      // stepper position at min/max of travel

	isv57communication isv57;
	

	/*public: void StartTask(void)
	{      
		//Start Task with input parameter set to "this" class
		xTaskCreatePinnedToCore(
		this->servoCommunicationTask,        //Function to implement the task 
		"servoCommunicationTask",            //Name of the task
		5000,                   //Stack size in words 
		this,                   //Task input parameter 
		1,           //Priority of the task 
		&task_iSV_Communication,                 //Task handle.
		0);              //Core where the task should run 
	}*/




	
	bool isv57LifeSignal_b = false;
	bool invertMotorDir_global_b = false;
	int32_t servoPos_i16 = 0;
	int32_t servo_offset_compensation_steps_i32 = 0;

	bool restartServo = false;
	void setLifelineSignal();

	bool enableSteplossRecov_b = true;
	bool enableCrashDetection_b = true;

	bool logAllServoParams = false;

	int32_t servoPos_local_corrected_i32 = 0;

	uint32_t stepsPerMotorRev_u32 = 3200u;

	

public:
	StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32);
	bool hasValidStepper() const { return NULL != _stepper; }

	void checkLimitsAndResetIfNecessary();
	void updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct);
	bool isAtMinPos();
	void correctPos();
	void findMinMaxSensorless(DAP_config_st dap_config_st);
	void forceStop();
	int8_t moveTo(int32_t position, bool blocking = false);
	void moveSlowlyToPos(int32_t targetPos_ui32);

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
	bool getLifelineSignal();
	
	void configSteplossRecovAndCrashDetection(uint8_t flags_u8);
	void printAllServoParameters();


	void setServosInternalPositionCorrected(int32_t posCorrected_i32);
	int32_t getServosInternalPositionCorrected();


	static void servoCommunicationTask( void * pvParameters );

};
