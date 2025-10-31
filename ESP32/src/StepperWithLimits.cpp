#include "StepperWithLimits.h"
#include "Main.h"
#include "Math.h"

#include "FunctionProfiler.h"

#define STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT 30
#define MIN_POS_MAX_ENDSTOP 10000 // servo has to drive minimum N steps before it allows the detection of the max endstop
#define INCLUDE_vTaskDelete 1

#define BRAKE_RESISTOR_DEACTIVATION_TIME_IN_MS 1000

static const uint8_t LIMIT_TRIGGER_VALUE = LOW;                                   // does endstop trigger high or low
static const int32_t ENDSTOP_MOVEMENT = (float)100; // how much to move between trigger checks
static const int32_t ENDSTOP_MOVEMENT_SENSORLESS = ENDSTOP_MOVEMENT * 5;


#define MAX_ESTIMATED_SERVO_OFFSET (int16_t)5000

TaskHandle_t task_iSV_Communication;
unsigned long cycleTimeLastCall_lifelineCheck = 0;//micros();
bool previousIsv57LifeSignal_b = true;
#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS 200
#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_CRASH_IN_MS 10000
#define TWO_TO_THE_POWER_OF_15_MINUS_1 (uint32_t)32767 // 2^15 - 1
//#define INT16_MAX (int32_t)65536
static SemaphoreHandle_t semaphore_lifelineSignal = NULL;
static SemaphoreHandle_t semaphore_resetServoPos = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_readServoValues = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_getSetCorrectedServoPos = xSemaphoreCreateMutex();


static float servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_36V;
static bool servoBusVoltageParameterized_b = true;

static bool printProfilingFlag_b = false;

bool setServoToSleep_b = false;

#define BRAKE_RESISTOR_UPPER_TRHESHOLD_VOLTAGE 4.0f
#define BRAKE_RESISTOR_LOWER_TRHESHOLD_VOLTAGE 1.0f

#define STEPPER_TASK_TIME_IN_MS (uint8_t)1
#define STEPPER_FORWARD_PLANNING_TIME_IN_MS (uint8_t)1


StepperWithLimits::StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32, uint8_t ratioOfInertia_arg_u8)
  :  _endstopLimitMin(0),    _endstopLimitMax(0)
  , _posMin(0),      _posMax(0)
  , stepsPerMotorRev_u32(stepsPerMotorRev_arg_u32)
  , ratioOfInertia_u8(ratioOfInertia_arg_u8)
{

  	_stepper = new FastNonAccelStepper(pinStep, pinDirection, invertMotorDir_b); 


	// pinMode(pinMin, INPUT);
	// pinMode(pinMax, INPUT);

	Serial.printf("InvertStepperDir: %d\n", invertMotorDir_b);
	// _stepper = stepperEngine().stepperConnectToPin(pinStep);	

	// _stepper->setDirectionPin(pinDirection, invertMotorDir_b);
    // _stepper->setAutoEnable(true);
    // _stepper->setAbsoluteSpeedLimit( maxSpeedInTicks ); // ticks
    // _stepper->setSpeedInTicks( maxSpeedInTicks ); // ticks
    // _stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);  // steps/s²
	// _stepper->setLinearAcceleration(0);
    // _stepper->setForwardPlanningTimeInMs(STEPPER_FORWARD_PLANNING_TIME_IN_MS);

	
	/************************************************************/
	/* 					iSV57 initialization					*/
	/************************************************************/
	#ifdef SERVO_POWER_PIN
        //turn on the servo's power
        gpio_set_direction((gpio_num_t)SERVO_POWER_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)SERVO_POWER_PIN, 1);
        //wait for the servo to initialize
        delay(500);
    #endif
	//delay(3000);
	// find iSV57 servo ID
	bool isv57slaveIdFound_b = isv57.findServosSlaveId();
	ActiveSerial->print("iSV57 slaveId found:  ");
	ActiveSerial->println( isv57slaveIdFound_b );
	
	// restart ESP when no servo was detected
	if (!isv57slaveIdFound_b)
	{
		ActiveSerial->println( "No servo found! Restarting ESP" );
		ESP.restart();
	}

	// check whether iSV57 is connected
	// isv57LifeSignal_b = isv57.checkCommunication();
	setLifelineSignal();
	if (getLifelineSignal() == false)
	{
		ActiveSerial->println( "No lifeline detected! Restarting ESP" );
		ESP.restart();
	}
	else
	{
		// read servos alarm history
		isv57.readAlarmHistory();

		// reset iSV57 alarms
		//bool servoAlarmsCleared = isv57.clearServoAlarms();

		ActiveSerial->print("iSV57 communication state:  ");
		ActiveSerial->println( getLifelineSignal() );

		

		// flash iSV57 registers
		isv57.setupServoStateReading();
		invertMotorDir_global_b = invertMotorDir_b;
		isv57.sendTunedServoParameters(invertMotorDir_global_b, stepsPerMotorRev_u32, ratioOfInertia_u8);


		delay(30);
		isv57.enableAxis();
		delay(100);
		
		// ToDo: 
		// - set servos internal rotation direction via debug port, thus ESPs and servos direction are aligned
		
		
		// print all servo registers
		/*if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS) 
		{
			isv57.readAllServoParameters();
		}*/


		// start read task
		xTaskCreatePinnedToCore(
						  this->servoCommunicationTask,   
						  "servoCommunicationTask", 
						  4096,  
						  //STACK_SIZE_FOR_TASK_2,    
						  this,//NULL,      
						  TASK_PRIORITY_SERVO_COMMUNICATION_TASK,         
						  &task_iSV_Communication,    
						  CORE_ID_SERVO_COMMUNICATION_TASK);   


						  
	}

	
	
	
	
	
  
}


// Clear all servo alarms
void StepperWithLimits::resetServoParametersToFactoryValues()
{
	resetServoRegistersToFactoryValues_b = true;
}

// Clear all servo alarms
void StepperWithLimits::clearAllServoAlarms()
{
	clearAllServoAlarms_b = true;
}

// Log all servo params
void StepperWithLimits::printAllServoParameters()
{
	logAllServoParams = true;
}

void StepperWithLimits::configSetProfilingFlag(bool proFlag_b)
{
	printProfilingFlag_b = proFlag_b;
}


void StepperWithLimits::findMinMaxSensorless(DAP_config_st dap_config_st)
{

  if (! hasValidStepper()) return;


	if ( getLifelineSignal() )
	{

		/************************************************************/
		/* 					servo reading check 					*/
		/************************************************************/
		// check if servo readings are trustworthy, by checking if servos bus voltage is in reasonable range. Otherwise restart servo.
		bool servoRadingsTrustworthy_36VRange_b = false;
		bool servoRadingsTrustworthy_48VRange_b = false;
		for (uint16_t waitTillServoCounterWasReset_Idx = 0; waitTillServoCounterWasReset_Idx < 10; waitTillServoCounterWasReset_Idx++)
		{
			delay(100);

			// voltage return is given in 0.1V units --> 10V range --> threshold 100
			// at beginning the values typically are initialized with -1
			float servosBusVoltageInVolt_fl32 = ( (float)getServosVoltage() ) / 10.0f;
			
			servoRadingsTrustworthy_36VRange_b = ( servosBusVoltageInVolt_fl32 >= 16.0f) && ( servosBusVoltageInVolt_fl32 < SERVO_MAX_VOLTAGE_IN_V_36V);
			servoRadingsTrustworthy_48VRange_b = ( servosBusVoltageInVolt_fl32 >= 16.0f) && ( servosBusVoltageInVolt_fl32 < SERVO_MAX_VOLTAGE_IN_V_48V);

			if (true == servoRadingsTrustworthy_36VRange_b)
			{
				servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_36V;
				servoBusVoltageParameterized_b = false;
				ActiveSerial->print("Servos bus voltage in expected range (36V range): ");
				ActiveSerial->print( servosBusVoltageInVolt_fl32 );
				ActiveSerial->println("V");
				break;
			}

			if (true == servoRadingsTrustworthy_48VRange_b)
			{
				servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_48V;
				servoBusVoltageParameterized_b = false;
				ActiveSerial->print("Servos bus voltage in expected range (48V range): ");
				ActiveSerial->print( servosBusVoltageInVolt_fl32 );
				ActiveSerial->println("V");
				break;
			}
		}

		if ( (false == servoRadingsTrustworthy_36VRange_b) && (false == servoRadingsTrustworthy_48VRange_b) )
		{
			ActiveSerial->print("Servo bus voltage not in expected range (16V-50V). Restarting ESP!");
			ESP.restart();
		}
		

		





		/************************************************************/
		/* 					min endstop	detection					*/
		/************************************************************/
		bool endPosDetected = true; // abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		int32_t setPosition = 0;

		
		// wait some time to check if signal stabilized
		for (uint16_t tryIdx = 0; tryIdx < 500; tryIdx++)
		{
			delay(5);
			endPosDetected = abs( getServosCurrent() ) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;

			if (false == endPosDetected)
			{
				break;
			}	
		}
			
		// reduce speed and acceleration
		_stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED / 16);

		// run continously in one direction until endstop is hit
		ActiveSerial->println("Move to min");
		// _stepper->forceStopAndNewPosition(0);
		_stepper->keepRunningBackward(MAXIMUM_STEPPER_SPEED / 16);
		
		while( (!endPosDetected) && (getLifelineSignal()) ){
			delay(1);
			endPosDetected = abs( getServosCurrent() ) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		}
		setPosition = - 5 * ENDSTOP_MOVEMENT_SENSORLESS;
		delay(20);
		_stepper->forceStopAndNewPosition(setPosition);
		delay(100);
		
		ActiveSerial->println("Min endstop reached.");
		ActiveSerial->printf("Current pos: %d\n", _stepper->getCurrentPosition() );
		// move slightly away from the block to prevent mechanical hits during normal operation
		_stepper->moveTo(0, true);
		_endstopLimitMin = 0;
		//delay(1000);

		ActiveSerial->println("Moved to pos: 0");
		ActiveSerial->printf("Current pos: %d\n", _stepper->getCurrentPosition() );
		
		delay(200);
		isv57.setZeroPos();

		
		/************************************************************/
		/* 					max endstop	detection					*/
		/************************************************************/
		// calculate max steps for endstop limit
		float spindlePitch = max( dap_config_st.payLoadPedalConfig_.spindlePitch_mmPerRev_u8, (uint8_t)1 );
		float maxRevToReachEndPos = (float)dap_config_st.payLoadPedalConfig_.lengthPedal_travel / spindlePitch;
		float maxStepsToReachEndPos = maxRevToReachEndPos * (float)stepsPerMotorRev_u32;
  
		endPosDetected = false; //abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		
		// run continously in one direction until endstop is hit
		_stepper->move(INT32_MAX, false);

		// if endstop is reached, communication is lost or virtual endstop is hit
		while( (!endPosDetected) && (getLifelineSignal()) ){
			delay(1);
			if (_stepper->getCurrentPosition() > MIN_POS_MAX_ENDSTOP)
    		{
				endPosDetected = abs( getServosCurrent() ) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
			}

			// virtual endstop
			endPosDetected |= (_stepper->getCurrentPosition() > maxStepsToReachEndPos);

			//ActiveSerial->printf("Pos: %d\n", _stepper->getCurrentPosition());
		}
		_stepper->forceStop();
		delay(100);
		_endstopLimitMax = _stepper->getCurrentPosition() - 5 * ENDSTOP_MOVEMENT_SENSORLESS;

		ActiveSerial->printf("Max endstop reached: %d\n", _endstopLimitMax);
		
		// move slowly to min position
		moveSlowlyToPos(100);
		
		// increase speed back to normal
		_stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED);
	}	
	

}

void StepperWithLimits::moveToPosWithSpeed(int32_t targetPos_ui32, uint32_t speedInHz_u32) {
  _stepper->setMaxSpeed(speedInHz_u32);
  _stepper->moveTo(targetPos_ui32, false);
}


void StepperWithLimits::moveSlowlyToPos(int32_t targetPos_ui32) {

  // Get previous speed level
  uint32_t prevSpeed_u32 = _stepper->getMaxSpeed();

  // reduce speed
  _stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED / 8);

  // move to min
  _stepper->moveTo(targetPos_ui32, true);

  // reset speed to previous speedlevel
//   _stepper->setMaxSpeed(prevSpeed_u32);
  delay(1);
  _stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED);
  delay(1);
}


void StepperWithLimits::pauseTask()
{
	vTaskSuspend( task_iSV_Communication );
}

void StepperWithLimits::updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct) {
  int32_t limitRange = _endstopLimitMax - _endstopLimitMin;

//   ActiveSerial->printf("PedalStart: %d,    PedalEnd:%d\n", pedalStartPosPct, pedalEndPosPct);

  float helper;
  helper = _endstopLimitMin + (((float)limitRange * (float)pedalStartPosPct) * 0.01f);
  _posMin = (int32_t)helper;

  helper = _endstopLimitMin + (((float)limitRange * (float)pedalEndPosPct) * 0.01f);
  _posMax = (int32_t)helper;
}


void StepperWithLimits::forceStop() {
  _stepper->forceStop();
}

int8_t StepperWithLimits::moveTo(int32_t position, bool blocking) {

  _stepper->moveTo(position, blocking);
  return 1;
}

int32_t StepperWithLimits::getCurrentPositionFromMin() const {
  return _stepper->getCurrentPosition() - _posMin;
}

int32_t StepperWithLimits::getMinPosition() const {
  return _posMin;
}

// void StepperWithLimits::setMinPosition() {
//   _posMin = getCurrentPosition();
// }

int32_t StepperWithLimits::getCurrentPosition() const {
  return _stepper->getCurrentPosition();
}

float StepperWithLimits::getCurrentPositionFraction() const {
  return float(getCurrentPositionFromMin()) / getTravelSteps();
}

float StepperWithLimits::getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const {
  return ( (float)(extPos_i32))/ getTravelSteps();
}

int32_t StepperWithLimits::getTargetPositionSteps() const {
  return _stepper->getPositionAfterCommandsCompleted();
}



void StepperWithLimits::setSpeed(uint32_t speedInStepsPerSecond) 
{
  _stepper->setMaxSpeed(speedInStepsPerSecond);            // steps/s 
}

bool StepperWithLimits::isAtMinPos()
{

  bool isNotRunning = !_stepper->isRunning();
  bool isAtMinPos = abs( getCurrentPositionFromMin() ) < 10;

  return isAtMinPos && isNotRunning;
}

int32_t StepperWithLimits::getCurrentSpeedInMilliHz()
{
	return _stepper->getMaxSpeed();
	// return MAXIMUM_STEPPER_SPEED * 1000;
}

uint32_t StepperWithLimits::getMaxSpeedInMilliHz()
{
	// return _stepper->getMaxSpeedInMilliHz();
	// Hz to mHz: 1 Hz = 1000 milli Hz
	return MAXIMUM_STEPPER_SPEED * 1000;
}



/************************************************************/
/* 					Step loss recovery						*/
/************************************************************/
void StepperWithLimits::correctPos()
{
	if(semaphore_resetServoPos!=NULL)
	{
		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
		{
			// tune the current servo position to compesnate the position offset
			int32_t stepOffset =(int32_t)constrain(servo_offset_compensation_steps_i32, -10, 10);

			if (stepOffset != 0)
			{
				ActiveSerial->print("Position compensation: ");
				ActiveSerial->print(servo_offset_compensation_steps_i32);
				ActiveSerial->print(",   ");
				ActiveSerial->println(stepOffset);
			}

			// offset = ESPs position - servos position
			// new ESP pos = ESPs position - offset = ESPs position - ESPs position + servos position = servos position
			
			_stepper->setCurrentPosition(_stepper->getCurrentPosition() - stepOffset);
			servo_offset_compensation_steps_i32 = 0; // reset lost step variable to prevent overcompensation
			xSemaphoreGive(semaphore_resetServoPos);
		}
	}
	else
	{
		semaphore_resetServoPos = xSemaphoreCreateMutex();
	}
}





/************************************************************/
/* 					lifeline set and get					*/
/************************************************************/

bool StepperWithLimits::getLifelineSignal()
{
	bool signal_b = false;
	// if(semaphore_lifelineSignal!=NULL)
	// {
	// 	// Take the semaphore and just update the config file, then release the semaphore
	// 	if(xSemaphoreTake(semaphore_lifelineSignal, pdMS_TO_TICKS(1))==pdTRUE)
	// 	{
		  signal_b = isv57LifeSignal_b;
	// 	}

	// }

	return signal_b;
}

void StepperWithLimits::setLifelineSignal()
{
	// if(semaphore_lifelineSignal!=NULL)
	// {
	// 	// Take the semaphore and just update the config file, then release the semaphore
	// 	if(xSemaphoreTake(semaphore_lifelineSignal, pdMS_TO_TICKS(1))==pdTRUE)
	// 	{
		  isv57LifeSignal_b = isv57.checkCommunication();
// 		}
// 	}
}








int32_t StepperWithLimits::getServosVoltage()
{
	return isv57.isv57dynamicStates_.servo_voltage_0p1V;
}

int32_t StepperWithLimits::getServosCurrent()
{
	return isv57.isv57dynamicStates_.servo_current_percent;
}

int32_t StepperWithLimits::getServosPos()
{
	return isv57.getPosFromMin();
}

int32_t StepperWithLimits::getServosPosError()
{
	return isv57.isv57dynamicStates_.servo_pos_error_p;
}

int32_t StepperWithLimits::getEstimatedPosError()
{
	return isv57.isv57dynamicStates_.estimated_pos_error_i16;
}

// int32_t StepperWithLimits::getEstimatedPosError_getCurrentStepperPos()
// {
// 	return isv57.isv57dynamicStates_.estimated_pos_error_currentStepperPos_i16;
// }





void StepperWithLimits::setServosInternalPositionCorrected(int32_t posCorrected_i32)
{

	if(semaphore_getSetCorrectedServoPos!=NULL)
	{
	  if(xSemaphoreTake(semaphore_getSetCorrectedServoPos, (TickType_t)1)==pdTRUE) {
		servoPos_local_corrected_i32 = posCorrected_i32;
		xSemaphoreGive(semaphore_getSetCorrectedServoPos);
	  }
	}
	else
	{
	  semaphore_readServoValues = xSemaphoreCreateMutex();
	}

}

int32_t StepperWithLimits::getServosInternalPositionCorrected()
{
	int32_t pos_i32 = 0;
	if(semaphore_getSetCorrectedServoPos!=NULL)
	{
	  if(xSemaphoreTake(semaphore_getSetCorrectedServoPos, (TickType_t)1)==pdTRUE) {
		pos_i32 = servoPos_local_corrected_i32;
		xSemaphoreGive(semaphore_getSetCorrectedServoPos);
	  }
	}
	else
	{
	  semaphore_readServoValues = xSemaphoreCreateMutex();
	}

	return pos_i32;
}



int32_t StepperWithLimits::getServosInternalPosition()
{
	int32_t servoPos_local_i32 = 0;
	
	if(semaphore_readServoValues!=NULL)
	{
	  if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {
		servoPos_local_i32 = servoPos_i16;
		xSemaphoreGive(semaphore_readServoValues);
	  }
	}
	else
	{
	  semaphore_readServoValues = xSemaphoreCreateMutex();
	}
	
	return servoPos_local_i32;
}


void StepperWithLimits::configSteplossRecovAndCrashDetection(uint8_t flags_u8)
{
	enableSteplossRecov_b = (flags_u8 >> 0) & 1;
	enableCrashDetection_b = (flags_u8 >> 1) & 1;
}

void StepperWithLimits::configSetPositionCommandSmoothingFactor(uint8_t posCommandSmoothingFactorArg_u8)
{
	if (posCommandSmoothingFactor_u16 != (uint16_t)posCommandSmoothingFactorArg_u8)
	{
		posCommandSmoothingFactor_u16 = constrain( (uint16_t)posCommandSmoothingFactorArg_u8, 0, 255);
		updateServoParams_b = true;
	}
}

void StepperWithLimits::configSetRatioOfInertia(uint8_t ratioOfInertia_arg_u8)
{
	if (ratioOfInertia_u8 != (uint8_t)ratioOfInertia_arg_u8)
	{
		ratioOfInertia_u8 = constrain( (uint8_t)ratioOfInertia_arg_u8, 1, 255);
		updateServoParams_b = true;


	}
}



int64_t timeSinceLastServoPosChange_l = 0;
int64_t timeDiff = 0;
int16_t servoPos_last_i16 = 0;
int64_t timeNow_l = 0;


uint32_t stackSizeIdx_u32 = 0;



int64_t timeNow_isv57SerialCommunicationTask_l = 0;

#ifdef BRAKE_RESISTOR_PIN
int64_t time_brakeResistorLastPassive = 0;
#endif



static SemaphoreHandle_t timer_fireServoCommunication; // Semaphore to signal the pedal update task
void IRAM_ATTR timer_servoCommunication_callback(void* arg) {
  if(timer_fireServoCommunication != NULL)
  {
    // It immediately gives the semaphore to wake up myCore1Task.
    xSemaphoreGiveFromISR(timer_fireServoCommunication, NULL);
  }
//   else
//   {
//     timer_fireServoCommunication = xSemaphoreCreateBinary();
//   }
}


void IRAM_ATTR StepperWithLimits::servoCommunicationTask(void *pvParameters)
{
  
	timer_fireServoCommunication = xSemaphoreCreateBinary();

	// 1. Define timer configuration
	const esp_timer_create_args_t timer_args_servoCommunication = {
		.callback = &timer_servoCommunication_callback, // Function to call
		.name = "servo_communication"    // A name for debugging
	};

	// 2. Create the timer handle
	esp_timer_handle_t timer_handle_servoCommunication;
	esp_timer_create(&timer_args_servoCommunication, &timer_handle_servoCommunication);

	// 3. Start the timer to fire periodically
	// The second argument is the period in microseconds.
	esp_timer_start_periodic(timer_handle_servoCommunication, REPETITION_INTERVAL_SERVO_COMMUNICATION_TASK_IN_US); 

  	// Cast the parameter to StepperWithLimits pointer
	StepperWithLimits* stepper_cl = static_cast<StepperWithLimits*>(pvParameters);

	FunctionProfiler profiler_servoCommunication;
  	profiler_servoCommunication.setName("servoCommunication");
  	profiler_servoCommunication.setNumberOfCalls(300);
	// profiler_servoCommunication.activate( true );

	for(;;){

		// wait for the timer to fire
		// This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
		if(timer_fireServoCommunication != NULL)
		{
			if (xSemaphoreTake(timer_fireServoCommunication, portMAX_DELAY) == pdTRUE) {

				
				profiler_servoCommunication.activate( printProfilingFlag_b );


				// start profiler 0, overall function
      			profiler_servoCommunication.start(0);

				// measure callback time and continue, when desired period is reached
				timeNow_isv57SerialCommunicationTask_l = millis();
				// int64_t timeDiff_serialCommunicationTask_l = ( timePrevious_isv57SerialCommunicationTask_l + REPETITION_INTERVAL_ISV57_SERIALCOMMUNICATION_TASK) - timeNow_isv57SerialCommunicationTask_l;
				// uint32_t targetWaitTime_u32 = constrain(timeDiff_serialCommunicationTask_l, 0, REPETITION_INTERVAL_ISV57_SERIALCOMMUNICATION_TASK);
				// delay(targetWaitTime_u32);
				// timePrevious_isv57SerialCommunicationTask_l = millis();

				

				/************************************************************/
				/* 					disable servo due to timeout			*/
				/************************************************************/
				if(true == setServoToSleep_b)
				{
					stepper_cl->isv57.disableAxis();
					delay(500);	
					setServoToSleep_b = false;
				}



				/************************************************************/
				/* 					recheck lifeline						*/
				/************************************************************/
				// check if servo communication is still there every N milliseconds
				if ( (timeNow_isv57SerialCommunicationTask_l - cycleTimeLastCall_lifelineCheck) > 500) 
				{
					// if target cycle time is reached, update last time
					cycleTimeLastCall_lifelineCheck = timeNow_isv57SerialCommunicationTask_l;
					stepper_cl->setLifelineSignal();
				}


				/************************************************************/
				/* 					clear all servo alarms 					*/
				/************************************************************/
				if (true == stepper_cl->clearAllServoAlarms_b)
				{
					ActiveSerial->println("Clearing all servo alarms.");
					stepper_cl->isv57.clearServoAlarms();
					stepper_cl->isv57.readAlarmHistory();
					stepper_cl->clearAllServoAlarms_b = false;
				}
				
				/************************************************************/
				/* 					reset to factory parameters				*/
				/************************************************************/
				if (true == stepper_cl->resetServoRegistersToFactoryValues_b)
				{
					ActiveSerial->println("Reset to factory settings.");
					stepper_cl->isv57.resetToFactoryParams();
					stepper_cl->resetServoRegistersToFactoryValues_b = false;

					delay(500);
					ESP.restart();
				}

				/************************************************************/
				/* 					log all servo params 					*/
				/************************************************************/
				if (true == stepper_cl->logAllServoParams)
				{
					stepper_cl->logAllServoParams = false;
					stepper_cl->isv57.readAllServoParameters();
				}

				/************************************************************/
				/* 					update servo params 					*/
				/************************************************************/
				if (true == stepper_cl->updateServoParams_b)
				{
					stepper_cl->isv57.setPositionSmoothingFactor(stepper_cl->posCommandSmoothingFactor_u16);
					stepper_cl->isv57.setRatioOfInertia(stepper_cl->ratioOfInertia_u8);
					stepper_cl->updateServoParams_b = false;

					ActiveSerial->println("Updating Servo parameters.");
				}


				/************************************************************/
				/* 					read servo states 						*/
				/*				and calculate step loss 					*/
				/************************************************************/
				if ( stepper_cl->getLifelineSignal() )
				{
					if(stepper_cl->servoStatus!=SERVO_IDLE_NOT_CONNECTED)
					{
						stepper_cl->servoStatus=SERVO_CONNECTED;
					}
					// restarting servo axis
					if(true == stepper_cl->restartServo)
					{
						//stepper_cl->isv57.resetAxisCounter();

						stepper_cl->isv57.disableAxis();
						delay(15);				
						stepper_cl->isv57.enableAxis();
						stepper_cl->restartServo = false;
						delay(15);
					}

					if (false == servoBusVoltageParameterized_b)
					{
						ActiveSerial->print("Setting virtual brake resistor to ");
						ActiveSerial->print( servoBusVoltageParameterized_fl32 );
						ActiveSerial->println("V");

						#ifdef BRAKE_RESISTOR_PIN
							ActiveSerial->print("Setting real brake resistor thresholds to ");
							ActiveSerial->print( servoBusVoltageParameterized_fl32+BRAKE_RESISTOR_UPPER_TRHESHOLD_VOLTAGE );
							ActiveSerial->print("V and ");
							ActiveSerial->print( servoBusVoltageParameterized_fl32+BRAKE_RESISTOR_LOWER_TRHESHOLD_VOLTAGE );
							ActiveSerial->println("V");
						#endif

						// set iSV57 parameters for 36 or 48V range
						stepper_cl->isv57.setServoVoltage(servoBusVoltageParameterized_fl32);
						servoBusVoltageParameterized_b = true;
					}


					// when servo has been restarted, the read states need to be initialized first
					if (false == previousIsv57LifeSignal_b)
					{
						stepper_cl->isv57.setupServoStateReading();
						previousIsv57LifeSignal_b = true;
						delay(50);
					}
					
					
					
					
					
					
					
					// read servo states
					profiler_servoCommunication.start(1);
					stepper_cl->isv57.readServoStates();
					profiler_servoCommunication.end(1);
					

					// Activate brake resistor once a certain voltage level is exceeded, 
					// but deactivate brake resistor once certain activation time is exceeded to prevent damage due to overheating

					

					#ifdef BRAKE_RESISTOR_PIN

						float brakeResistorVoltageOn_inV_fl32 = (servoBusVoltageParameterized_fl32 + BRAKE_RESISTOR_UPPER_TRHESHOLD_VOLTAGE);
						float brakeResistorVoltageOff_inV_fl32 = (servoBusVoltageParameterized_fl32 + BRAKE_RESISTOR_LOWER_TRHESHOLD_VOLTAGE);
						
						float busVoltage_inV_fl32 = ( (float)stepper_cl->getServosVoltage() ) * 0.1f;
						int64_t brakeResistorUpTime_i64 = timeNow_isv57SerialCommunicationTask_l - time_brakeResistorLastPassive;
						
						if ((busVoltage_inV_fl32 > brakeResistorVoltageOn_inV_fl32 && !stepper_cl->brakeResistorState_b)
							|| (stepper_cl->brakeResistorState_b && 
								brakeResistorUpTime_i64 < BRAKE_RESISTOR_DEACTIVATION_TIME_IN_MS &&
								busVoltage_inV_fl32 > brakeResistorVoltageOff_inV_fl32))
						{
							digitalWrite(BRAKE_RESISTOR_PIN, HIGH); 
							stepper_cl->brakeResistorState_b = true;
						}
						else
						{
							digitalWrite(BRAKE_RESISTOR_PIN, LOW);
							stepper_cl->brakeResistorState_b = false;
							time_brakeResistorLastPassive = timeNow_isv57SerialCommunicationTask_l;
						}
						
					#endif


					if(semaphore_readServoValues!=NULL)
					{
						if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {

							// caclulate servos positions from endstop
							stepper_cl->servoPos_i16 = stepper_cl->isv57.getPosFromMin();

							// in normal configuration, where servo is at front of the pedal, a positive servo rotation will make the sled move to the front. We want it to be the other way around though. Movement to the back means positive rotation
							if (false == stepper_cl->invertMotorDir_global_b)
							{
								stepper_cl->servoPos_i16 *= -1;
							}

							xSemaphoreGive(semaphore_readServoValues);
						}
					}
					else
					{
						semaphore_readServoValues = xSemaphoreCreateMutex();
					}




					// unwrap the servos position by aligning it to the ESPs position
					int32_t servoPosCorrected_i32 = stepper_cl->getServosInternalPosition();
					int32_t espPos_i32 = stepper_cl->getCurrentPosition();
					// allow up to 50 wraps 
					// 1 rotation = 6400 steps
					// 2^15 / 6400 = 5.12 rotations until wrap
					// 5 rotations * 5mm/rotation * 50 wraps --> 250mm
					for (uint8_t wrapIndex_u8 = 0; wrapIndex_u8 < 50; wrapIndex_u8++)
					{
						bool posCorrectedInLoop_b = false;
						if ( ( espPos_i32 - servoPosCorrected_i32 ) > INT16_MAX )
						{
							// example positive int16 overflow:
							// esp pos = 2^15-1 = 32767
							// servo pos = -2^15 = -32768
							// servo pos + 2^16 = -32768 + 65536 = 32768
							
							// 65536 = 2^16
							servoPosCorrected_i32 += 65536;
							posCorrectedInLoop_b = true;
						}

						if ( ( espPos_i32 - servoPosCorrected_i32 ) < INT16_MIN )
						{
							// example, negative int16 overflow:
							// esp pos = -2^15 = -32768
							// servo pos = 2^15-1 = 32767
							// servo pos - 2^16 = 32767 - 65536 = -32769
							
							// 65536 = 2^16
							servoPosCorrected_i32 -= 65536;
							posCorrectedInLoop_b = true;
						}

						if (false == posCorrectedInLoop_b)
						{
							break;
						}
					}
					

					stepper_cl->setServosInternalPositionCorrected(servoPosCorrected_i32);
					
					
					// estimate position offset between ESPs target position and true servo position
					int16_t estServoOffsetInSteps_i16 = stepper_cl->getServosInternalPositionCorrected() + stepper_cl->getServosPosError() - stepper_cl->getCurrentPosition();
					estServoOffsetInSteps_i16 = constrain(estServoOffsetInSteps_i16, -MAX_ESTIMATED_SERVO_OFFSET, MAX_ESTIMATED_SERVO_OFFSET );
					stepper_cl->isv57.isv57dynamicStates_.estimated_pos_error_i16 = estServoOffsetInSteps_i16;
					// stepper_cl->isv57.isv57dynamicStates_.estimated_pos_error_currentStepperPos_i16 = stepper_cl->getCurrentPosition();
					
					
					
					int32_t servo_offset_compensation_steps_local_i32;// = 0;

					// condition 1: servo must be at halt
					// condition 2: the esp accel lib must be at halt	
					bool cond_stepperIsAtMinPos = false;
					bool cond_timeSinceHitMinPositionLargerThanThreshold_1 = false;
					bool cond_timeSinceHitMinPositionLargerThanThreshold_2 = false;

					// check whether target position from ESP hasn't changed and is at min endstop position
					cond_stepperIsAtMinPos = stepper_cl->isAtMinPos();

					
					int16_t servoPos_now_i16;
					if (cond_stepperIsAtMinPos == true)
					{
						//isv57.readServoStates();
						servoPos_now_i16 = stepper_cl->isv57.getPosFromMin();
						timeNow_l = millis();

						// check whether servo position has changed, in case, update the halt detection variable
						if (abs((int32_t)servoPos_last_i16 - (int32_t)servoPos_now_i16) > 30)
						///if (servoPos_last_i16 != servoPos_now_i16)
						{
							servoPos_last_i16 = servoPos_now_i16;
							timeSinceLastServoPosChange_l = timeNow_l;
						}

						// compute the time difference since last servo position change
						timeDiff = timeNow_l - timeSinceLastServoPosChange_l;

						// if time between last servo position is larger than a threshold, detect servo standstill 
						if ( (timeDiff > TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS) 
							&& (timeNow_l > 0) )
						{
							cond_timeSinceHitMinPositionLargerThanThreshold_1 = true;
						}
						else
						{
							cond_timeSinceHitMinPositionLargerThanThreshold_1 = false;
						}



						// if time between last servo position is larger than a threshold, detect servo standstill. Longer intervall for crash detection
						if ( (timeDiff > TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_CRASH_IN_MS) 
							&& (timeNow_l > 0) )
						{
							cond_timeSinceHitMinPositionLargerThanThreshold_2 = true;
						}
						else
						{
							cond_timeSinceHitMinPositionLargerThanThreshold_2 = false;
						}
						

					}
					else {}
					


					


					
					// ActiveSerial->printf("Cond1: %d,    Cond2: %d,    Cond3: %d,    servoPos: %d\n", cond_stepperIsAtMinPos, cond_timeSinceHitMinPositionLargerThanThreshold_1, cond_timeSinceHitMinPositionLargerThanThreshold_2, servoPos_now_i16);

					// calculate zero position offset

					if (cond_stepperIsAtMinPos)
					{
						// When the servo turned off during driving, the servo loses its zero position and the correction might not be valid anymore. If still applied, the servo will somehow srive against the block
						// resulting in excessive servo load --> current load. We'll detect whether min or max block was reached, depending on the position error sign
						if (cond_timeSinceHitMinPositionLargerThanThreshold_2 && (true == stepper_cl->enableCrashDetection_b))
						{
							
							bool servoCurrentLow_b = abs(stepper_cl->isv57.isv57dynamicStates_.servo_current_percent) < 50;//200;
							if (!servoCurrentLow_b)
							{

								// positive current means positive rotation 
								// bool minBlockCrashDetected_b = false;
								// bool maxBlockCrashDetected_b = false;
								if (stepper_cl->isv57.isv57dynamicStates_.servo_current_percent > 0) // if current is positive, the rotation will be positive and thus the sled will move towards the user
								{
									// minBlockCrashDetected_b = true; 
									stepper_cl->isv57.applyOfsetToZeroPos(-500); // bump up a bit to prevent the servo from pushing against the endstop continously
								}
								else
								{
									// maxBlockCrashDetected_b = true;
									stepper_cl->isv57.applyOfsetToZeroPos(500); // bump up a bit to prevent the servo from pushing against the endstop continously
								}

								/*print_cycle_counter_u64++;
								print_cycle_counter_u64 %= 10;

								if (print_cycle_counter_u64 == 0)
								{
								ActiveSerial->print("minDet: ");
								ActiveSerial->print(minBlockCrashDetected_b);

								ActiveSerial->print("curr: ");
								ActiveSerial->print(isv57.servo_current_percent);
								
								ActiveSerial->print("posError: ");
								ActiveSerial->print(isv57.servo_pos_error_p);

								ActiveSerial->println();
								}*/


								//servo_offset_compensation_steps_local_i32 = isv57.servo_pos_error_p;
							}
						}


						// step loss recovery
						if (cond_timeSinceHitMinPositionLargerThanThreshold_1)
						{
							if (true == stepper_cl->enableSteplossRecov_b)
							{
								// calculate encoder offset
								// movement to the back will reduce encoder value

								servo_offset_compensation_steps_local_i32 = espPos_i32 - servoPosCorrected_i32;
								// if (false == stepper_cl->invertMotorDir_global_b)
								// {
								// 	servo_offset_compensation_steps_local_i32 *= -1;
								// }
							}
							else
							{
								servo_offset_compensation_steps_local_i32 = 0;
							}

							if(semaphore_resetServoPos!=NULL)
							{
								// Take the semaphore and just update the config file, then release the semaphore
								if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
								{
									stepper_cl->servo_offset_compensation_steps_i32 = servo_offset_compensation_steps_local_i32;

									
									xSemaphoreGive(semaphore_resetServoPos);
								}
							}
							else
							{
								semaphore_resetServoPos = xSemaphoreCreateMutex();
							}
						}
						


					}
					else {}


					
					
				}
				else
				{
					if(stepper_cl->servoStatus!=SERVO_IDLE_NOT_CONNECTED && stepper_cl->servoStatus!=SERVO_FORCE_STOP)
					{
						stepper_cl->servoStatus=SERVO_NOT_CONNECTED;
					}

					if(stepper_cl->servoStatus==SERVO_NOT_CONNECTED)
					{
						ActiveSerial->println("Servo communication lost!");
					}
					
					delay(100);
					previousIsv57LifeSignal_b = false;
					// De-activate brake resistor once servo communication is lost to prevent resistor damage
					#ifdef BRAKE_RESISTOR_PIN
						digitalWrite(BRAKE_RESISTOR_PIN, LOW);
						stepper_cl->brakeResistorState_b = false;
					#endif
				}


				// start profiler 0, overall function
      			profiler_servoCommunication.end(0);
			}
		}

		// force a context switch
		taskYIELD();
	}
}


bool StepperWithLimits::getBrakeResistorState()
{
	return brakeResistorState_b;
	//return true;
}

bool StepperWithLimits::servoIdleAction()
{

	bool returnValue_b = false;
	#ifdef SERVO_POWER_PIN
        //turn off the servo's power        
        gpio_set_level((gpio_num_t)SERVO_POWER_PIN, 0);
        //wait for the servo to initialize
        delay(500);
		returnValue_b = true;
    #endif

	#ifndef SERVO_POWER_PIN
		setServoToSleep_b = true;
		returnValue_b = true;
	#endif

	return returnValue_b;
}




/*int32_t StepperWithLimits::getStepLossCompensation()
{
	if(semaphore_resetServoPos!=NULL)
	{

	// Take the semaphore and just update the config file, then release the semaphore
	if(xSemaphoreTake(semaphore_resetServoPos, (TickType_t)1)==pdTRUE)
	{
	  servo_offset_compensation_steps_i32 = servo_offset_compensation_steps_local_i32;
	  xSemaphoreGive(semaphore_resetServoPos);
	}

	}
	else
	{
	semaphore_resetServoPos = xSemaphoreCreateMutex();
	//ActiveSerial->println("semaphore_resetServoPos == 0");
	}

	return servo_offset_compensation_steps_i32;
}*/




