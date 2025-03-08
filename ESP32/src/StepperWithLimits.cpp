#include "StepperWithLimits.h"
#include "RTDebugOutput.h"
#include "Main.h"
#include "Math.h"



#define STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT 30
#define MIN_POS_MAX_ENDSTOP 10000 // servo has to drive minimum N steps before it allows the detection of the max endstop
#define INCLUDE_vTaskDelete 1

//uint32_t speed_in_hz = TICKS_PER_S / ticks;
// TICKS_PER_S = 16000000L
// ticks = TICKS_PER_S / speed_in_hz
#define maxSpeedInTicks  (TICKS_PER_S / MAXIMUM_STEPPER_SPEED)

static const uint8_t LIMIT_TRIGGER_VALUE = LOW;                                   // does endstop trigger high or low
static const int32_t ENDSTOP_MOVEMENT = (float)100; // how much to move between trigger checks
static const int32_t ENDSTOP_MOVEMENT_SENSORLESS = ENDSTOP_MOVEMENT * 5;




TaskHandle_t task_iSV_Communication;
unsigned long cycleTimeLastCall_lifelineCheck = 0;//micros();
bool previousIsv57LifeSignal_b = true;
#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS 200
#define TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_CRASH_IN_MS 10000
#define TWO_TO_THE_POWER_OF_15_MINUS_1 (uint32_t)32767 // 2^15 - 1
//#define INT16_MAX (int32_t)65536
static SemaphoreHandle_t semaphore_lifelineSignal = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_resetServoPos = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_readServoValues = xSemaphoreCreateMutex();
static SemaphoreHandle_t semaphore_getSetCorrectedServoPos = xSemaphoreCreateMutex();


static float servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_36V;
static bool servoBusVoltageParameterized_b = true;

FastAccelStepperEngine& stepperEngine() {
  static FastAccelStepperEngine myEngine = FastAccelStepperEngine();   // this is a factory and manager for all stepper instances

  static bool firstTime = true;
  if (firstTime) {
     myEngine.init();
     firstTime = false;
  }
  
  myEngine.task_rate(4);
  return myEngine;
}


StepperWithLimits::StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32)
  :  _endstopLimitMin(0),    _endstopLimitMax(0)
  , _posMin(0),      _posMax(0)
  , stepsPerMotorRev_u32(stepsPerMotorRev_arg_u32)


{

  
	// pinMode(pinMin, INPUT);
	// pinMode(pinMax, INPUT);

	Serial.printf("InvertStepperDir: %d\n", invertMotorDir_b);
	_stepper = stepperEngine().stepperConnectToPin(pinStep);	

	_stepper->setDirectionPin(pinDirection, invertMotorDir_b);
    _stepper->setAutoEnable(true);
    _stepper->setAbsoluteSpeedLimit( maxSpeedInTicks ); // ticks
    _stepper->setSpeedInTicks( maxSpeedInTicks ); // ticks
    _stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);  // steps/s²
	_stepper->setLinearAcceleration(0);
    _stepper->setForwardPlanningTimeInMs(8);

	
	/************************************************************/
	/* 					iSV57 initialization					*/
	/************************************************************/
	//delay(3000);
	// find iSV57 servo ID
	bool isv57slaveIdFound_b = isv57.findServosSlaveId();
	Serial.print("iSV57 slaveId found:  ");
	Serial.println( isv57slaveIdFound_b );
	
	// restart ESP when no servo was detected
	if (!isv57slaveIdFound_b)
	{
		Serial.println( "No servo found! Restarting ESP" );
		ESP.restart();
	}

	// check whether iSV57 is connected
	// isv57LifeSignal_b = isv57.checkCommunication();
	setLifelineSignal();
	if (getLifelineSignal() == false)
	{
		Serial.println( "No lifeline detected! Restarting ESP" );
		ESP.restart();
	}
	else
	{
		// read servos alarm history
		isv57.readAlarmHistory();

		// reset iSV57 alarms
		bool servoAlarmsCleared = isv57.clearServoAlarms();

		Serial.print("iSV57 communication state:  ");
		Serial.println( getLifelineSignal() );

		

		// flash iSV57 registers
		isv57.setupServoStateReading();
		invertMotorDir_global_b = invertMotorDir_b;
		isv57.sendTunedServoParameters(invertMotorDir_global_b, stepsPerMotorRev_u32);


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
						  2000,  
						  //STACK_SIZE_FOR_TASK_2,    
						  this,//NULL,      
						  1,         
						  &task_iSV_Communication,    
						  0);   


						  
	}

	
	
	
	
	
  
}


// Log all servo params
void StepperWithLimits::printAllServoParameters()
{
	logAllServoParams = true;
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
				Serial.print("Servos bus voltage in expected range (36V range): ");
				Serial.print( servosBusVoltageInVolt_fl32 );
				Serial.println("V");
				break;
			}

			if (true == servoRadingsTrustworthy_48VRange_b)
			{
				servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_48V;
				servoBusVoltageParameterized_b = false;
				Serial.print("Servos bus voltage in expected range (48V range): ");
				Serial.print( servosBusVoltageInVolt_fl32 );
				Serial.println("V");
				break;
			}
		}

		if ( (false == servoRadingsTrustworthy_36VRange_b) && (false == servoRadingsTrustworthy_48VRange_b) )
		{
			Serial.print("Servo bus voltage not in expected range (16V-50V). Restarting ESP!");
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
		_stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED / 16);
		_stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION / 16);
		
		// run continously in one direction until endstop is hit
		//_stepper->keepRunningBackward(MAXIMUM_STEPPER_SPEED / 10);
		_stepper->move(INT32_MIN, false);
		
		while( (!endPosDetected) && (getLifelineSignal()) ){
			delay(1);
			endPosDetected = abs( getServosCurrent() ) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		}
		setPosition = - 5 * ENDSTOP_MOVEMENT_SENSORLESS;
		delay(20);
		_stepper->forceStopAndNewPosition(setPosition);
		delay(100);
		
		Serial.println("Min endstop reached.");
		Serial.printf("Current pos: %d\n", _stepper->getCurrentPosition() );
		// move slightly away from the block to prevent mechanical hits during normal operation
		_stepper->moveTo(0, true);
		_endstopLimitMin = 0;
		//delay(1000);

		Serial.println("Moved to pos: 0");
		Serial.printf("Current pos: %d\n", _stepper->getCurrentPosition() );
		
		
		

		/************************************************************/
		/* 			reset servos internal position counter,			*/
		/*			thus step loss recovery is simplified.			*/
		/************************************************************/
		// restart servo axis. This will reset the seros reg_add_position_given_p count to zero, thus equalizing the ESP zero and the servos zero position.
		/*restartServo = true;

		bool servoAxisResetSuccessfull_b = false;
		for (uint16_t waitTillServoCounterWasReset_Idx = 0; waitTillServoCounterWasReset_Idx < 10; waitTillServoCounterWasReset_Idx++)
		{
			delay(100);

			//bool servoPosRes_b = (50 > abs(isv57.servo_pos_given_p) ) || ( 50 > (INT16_MAX - abs(isv57.servo_pos_given_p))  );
			bool servoPosRes_b = 0 == (isv57.servo_pos_given_p); 
			if ( (false == restartServo) && (servoPosRes_b) )
			{
				Serial.print("Servo axis was reset succesfully! Current position: ");
				Serial.println(isv57.servo_pos_given_p);
				servoAxisResetSuccessfull_b = true;
				break;
			}
		}

		if(false == servoAxisResetSuccessfull_b)
		{
			Serial.print("Servo axis not reset. Restarting ESP!");
			ESP.restart();
		}*/
		

		// Serial.print("Servo axis current position (before clearing): ");
		// Serial.println(isv57.servo_pos_given_p);

		// restartServo = true;
		// delay(5000);

		// Serial.print("Servo axis current position (after clearing): ");
		// Serial.println(isv57.servo_pos_given_p);




		
		
		delay(200);
		isv57.setZeroPos();
		// setMinPosition();

		
		/************************************************************/
		/* 					max endstop	detection					*/
		/************************************************************/
		// calculate max steps for endstop limit
		float spindlePitch = max( dap_config_st.payLoadPedalConfig_.spindlePitch_mmPerRev_u8, (uint8_t)1 );
		float maxRevToReachEndPos = (float)dap_config_st.payLoadPedalConfig_.lengthPedal_travel / spindlePitch;
		float maxStepsToReachEndPos = maxRevToReachEndPos * (float)stepsPerMotorRev_u32;
  
		endPosDetected = false; //abs( isv57.servo_current_percent) > STEPPER_WITH_LIMITS_SENSORLESS_CURRENT_THRESHOLD_IN_PERCENT;
		
		// run continously in one direction until endstop is hit
		//_stepper->keepRunningForward(MAXIMUM_STEPPER_SPEED / 10);
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

			//Serial.printf("Pos: %d\n", _stepper->getCurrentPosition());
		}
		_stepper->forceStop();
		delay(100);
		_endstopLimitMax = _stepper->getCurrentPosition() - 5 * ENDSTOP_MOVEMENT_SENSORLESS;

		Serial.printf("Max endstop reached: %d\n", _endstopLimitMax);
		
		// move slowly to min position
		//moveSlowlyToPos(_posMin);
		//moveSlowlyToPos(5*ENDSTOP_MOVEMENT_SENSORLESS);
		moveSlowlyToPos(0);
		
		
		// increase speed and accelerartion back to normal
		//_stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED);
		_stepper->setAcceleration(MAXIMUM_STEPPER_ACCELERATION);
		_stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED);
	}	
	

}


void StepperWithLimits::moveSlowlyToPos(int32_t targetPos_ui32) {
  // reduce speed and accelerartion
  //_stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED / 4);
  _stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED / 4);

  // move to min
  _stepper->moveTo(targetPos_ui32, true);


  // increase speed and accelerartion
  //_stepper->setMaxSpeed(MAXIMUM_STEPPER_SPEED);
  _stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED);
}




void StepperWithLimits::updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct) {
  int32_t limitRange = _endstopLimitMax - _endstopLimitMin;

//   Serial.printf("PedalStart: %d,    PedalEnd:%d\n", pedalStartPosPct, pedalEndPosPct);

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
  //_stepper->setMaxSpeed(speedInStepsPerSecond);            // steps/s 
  _stepper->setSpeedInHz(MAXIMUM_STEPPER_SPEED);
}

bool StepperWithLimits::isAtMinPos()
{

  bool isNotRunning = !_stepper->isRunning();
  bool isAtMinPos = abs( getCurrentPositionFromMin() ) < 10;

  return isAtMinPos && isNotRunning;
}

int32_t StepperWithLimits::getCurrentSpeedInMilliHz()
{
	return _stepper->getCurrentSpeedInMilliHz();
}

uint32_t StepperWithLimits::getMaxSpeedInMilliHz()
{
	return _stepper->getMaxSpeedInMilliHz();
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

			// if (stepOffset != 0)
			// {
			// 	Serial.print("Position compensation: ");
			// 	Serial.print(servo_offset_compensation_steps_i32);
			// 	Serial.print(",   ");
			// 	Serial.println(stepOffset);
			// }

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
	/*if(semaphore_lifelineSignal!=NULL)
	{

		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_lifelineSignal, (TickType_t)1)==pdTRUE)
		{
		  signal_b = isv57LifeSignal_b;
		}

	}
	else
	{
		semaphore_lifelineSignal = xSemaphoreCreateMutex();
	}*/
	

	signal_b = isv57LifeSignal_b;

	return signal_b;
}

void StepperWithLimits::setLifelineSignal()
{

	isv57LifeSignal_b = isv57.checkCommunication();


	/*if(semaphore_lifelineSignal!=NULL)
	{

		// Take the semaphore and just update the config file, then release the semaphore
		if(xSemaphoreTake(semaphore_lifelineSignal, (TickType_t)1)==pdTRUE)
		{
		  isv57LifeSignal_b = isv57.checkCommunication();
		}

	}
	else
	{
		semaphore_lifelineSignal = xSemaphoreCreateMutex();
	}*/
}








int32_t StepperWithLimits::getServosVoltage()
{
	return isv57.servo_voltage_0p1V;
}

int32_t StepperWithLimits::getServosCurrent()
{
	return isv57.servo_current_percent;
}

int32_t StepperWithLimits::getServosPos()
{
	//return isv57.servo_pos_given_p;
	return isv57.getPosFromMin();
}



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

int64_t timeSinceLastServoPosChange_l = 0;
int64_t timeDiff = 0;
int16_t servoPos_last_i16 = 0;
int64_t timeNow_l = 0;


uint32_t stackSizeIdx_u32 = 0;



int64_t timeNow_isv57SerialCommunicationTask_l = 0;
int64_t timePrevious_isv57SerialCommunicationTask_l = 0;
#define REPETITION_INTERVAL_ISV57_SERIALCOMMUNICATION_TASK (int64_t)10

void StepperWithLimits::servoCommunicationTask(void *pvParameters)
{
  
  	// Cast the parameter to StepperWithLimits pointer
	StepperWithLimits* stepper_cl = static_cast<StepperWithLimits*>(pvParameters);

	for(;;){

	  

		// measure callback time and continue, when desired period is reached
		timeNow_isv57SerialCommunicationTask_l = millis();
		int64_t timeDiff_serialCommunicationTask_l = ( timePrevious_isv57SerialCommunicationTask_l + REPETITION_INTERVAL_ISV57_SERIALCOMMUNICATION_TASK) - timeNow_isv57SerialCommunicationTask_l;
		uint32_t targetWaitTime_u32 = constrain(timeDiff_serialCommunicationTask_l, 0, REPETITION_INTERVAL_ISV57_SERIALCOMMUNICATION_TASK);
		delay(targetWaitTime_u32);
		timePrevious_isv57SerialCommunicationTask_l = millis();

		

		
		/************************************************************/
		/* 					recheck lifeline						*/
		/************************************************************/
		// check if servo communication is still there every N milliseconds
		unsigned long now = millis();
		if ( (now - cycleTimeLastCall_lifelineCheck) > 500) 
		{
			// if target cycle time is reached, update last time
			cycleTimeLastCall_lifelineCheck = now;
			stepper_cl->setLifelineSignal();
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
		/* 					read servo states 						*/
		/*				and calculate step loss 					*/
		/************************************************************/
		if ( stepper_cl->getLifelineSignal() )
		{

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
				Serial.print("Setting virtual brake resistor to: ");
				Serial.print( servoBusVoltageParameterized_fl32 );
				Serial.println("V");

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
			stepper_cl->isv57.readServoStates();


			// Activate brake resistor once a certain voltage level is exceeded
			#ifdef BRAKE_RESISTOR_PIN
				if ( stepper_cl->getServosVoltage() > ((servoBusVoltageParameterized_fl32 + 4.0f)*10.0f) )
				{
				digitalWrite(BRAKE_RESISTOR_PIN, HIGH);
				}
				else
				{
				digitalWrite(BRAKE_RESISTOR_PIN, LOW);
				}
			#endif


			if(semaphore_readServoValues!=NULL)
			{
				if(xSemaphoreTake(semaphore_readServoValues, (TickType_t)1)==pdTRUE) {

					// caclulate servos positions from endstop
					//stepper_cl->servoPos_i16 = stepper_cl->isv57.servo_pos_given_p - stepper_cl->isv57.getZeroPos() ;
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
					// 4294967296 = 2^16
					servoPosCorrected_i32 += 4294967296;
					posCorrectedInLoop_b = true;
				}

				if ( ( espPos_i32 - servoPosCorrected_i32 ) < INT16_MIN )
				{
					// 4294967296 = 2^16
					servoPosCorrected_i32 -= 4294967296;
					posCorrectedInLoop_b = true;
				}

				if (false == posCorrectedInLoop_b)
				{
					break;
				}
			}
			


			stepper_cl->setServosInternalPositionCorrected(servoPosCorrected_i32);
			
			
			
			
			
			
			int32_t servo_offset_compensation_steps_local_i32 = 0;

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
				//int16_t servoPos_now_i16 = stepper_cl->isv57.servo_pos_given_p;
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

			


			


			
			// Serial.printf("Cond1: %d,    Cond2: %d,    Cond3: %d,    servoPos: %d\n", cond_stepperIsAtMinPos, cond_timeSinceHitMinPositionLargerThanThreshold_1, cond_timeSinceHitMinPositionLargerThanThreshold_2, servoPos_now_i16);

			// calculate zero position offset

			if (cond_stepperIsAtMinPos)
			{
				// When the servo turned off during driving, the servo loses its zero position and the correction might not be valid anymore. If still applied, the servo will somehow srive against the block
				// resulting in excessive servo load --> current load. We'll detect whether min or max block was reached, depending on the position error sign
				if (cond_timeSinceHitMinPositionLargerThanThreshold_2 && (true == stepper_cl->enableCrashDetection_b))
				{
					
					bool servoCurrentLow_b = abs(stepper_cl->isv57.servo_current_percent) < 50;//200;
					if (!servoCurrentLow_b)
					{

						// positive current means positive rotation 
						bool minBlockCrashDetected_b = false;
						bool maxBlockCrashDetected_b = false;
						if (stepper_cl->isv57.servo_current_percent > 0) // if current is positive, the rotation will be positive and thus the sled will move towards the user
						{
							minBlockCrashDetected_b = true; 
							stepper_cl->isv57.applyOfsetToZeroPos(-500); // bump up a bit to prevent the servo from pushing against the endstop continously
						}
						else
						{
							maxBlockCrashDetected_b = true;
							stepper_cl->isv57.applyOfsetToZeroPos(500); // bump up a bit to prevent the servo from pushing against the endstop continously
						}

						/*print_cycle_counter_u64++;
						print_cycle_counter_u64 %= 10;

						if (print_cycle_counter_u64 == 0)
						{
						Serial.print("minDet: ");
						Serial.print(minBlockCrashDetected_b);

						Serial.print("curr: ");
						Serial.print(isv57.servo_current_percent);
						
						Serial.print("posError: ");
						Serial.print(isv57.servo_pos_error_p);

						Serial.println();
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





			#ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
				if( stackSizeIdx_u32 == 1000)
				{
					UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
					Serial.print("StackSize (Servo communication): ");
					Serial.println(stackHighWaterMark);
					stackSizeIdx_u32 = 0;
				}
				stackSizeIdx_u32++;
			#endif

			
		}
		else
		{
			Serial.println("Servo communication lost!");
			delay(100);
			previousIsv57LifeSignal_b = false;


			// De-activate brake resistor once servo communication is lost to prevent resistor damage
			#ifdef BRAKE_RESISTOR_PIN
				digitalWrite(BRAKE_RESISTOR_PIN, LOW);
			#endif
		}


	}
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
	//Serial.println("semaphore_resetServoPos == 0");
	}

	return servo_offset_compensation_steps_i32;
}*/




