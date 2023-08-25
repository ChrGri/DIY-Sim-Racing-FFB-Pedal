#define ESTIMATE_LOADCELL_VARIANCE

//#define ALLOW_SYSTEM_IDENTIFICATION

/**********************************************************************************************/
/*                                                                                            */
/*                         function declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters();


bool systemIdentificationMode_b = false;
#include "sendConfig.h"




bool splineDebug_b = false;



#include <EEPROM.h>
#include <ArduinoJson.h>	



#include "ABSOscillation.h"
ABSOscillation absOscillation;
#define ABS_OSCILLATION



#include "DiyActivePedal_types.h"
DAP_config_st dap_config_st;
DAP_calculationVariables_st dap_calculationVariables_st;




#include "CycleTimer.h"
//#define PRINT_CYCLETIME

static CycleTimer timerPU("PU cycle time");
static CycleTimer timerSC("SC cycle time");

// target cycle time for pedal update task, to get constant cycle times, required for FIR filtering
#define PUT_TARGET_CYCLE_TIME_IN_US 100



#include "RTDebugOutput.h"


/**********************************************************************************************/
/*                                                                                            */
/*                         iterpolation  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "ForceCurve.h"
ForceCurve_Interpolated forceCurve;



/**********************************************************************************************/
/*                                                                                            */
/*                         multitasking  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/
#include "soc/rtc_wdt.h"

//#define PRINT_USED_STACK_SIZE
// https://stackoverflow.com/questions/55998078/freertos-task-priority-and-stack-size
#define STACK_SIZE_FOR_TASK_1 0.2 * (configTOTAL_HEAP_SIZE / 4)
#define STACK_SIZE_FOR_TASK_2 0.2 * (configTOTAL_HEAP_SIZE / 4)

TaskHandle_t Task1;
TaskHandle_t Task2;

static SemaphoreHandle_t semaphore_updateConfig=NULL;
  bool configUpdateAvailable = false;                              // semaphore protected data
  DAP_config_st dap_config_st_local;

static SemaphoreHandle_t semaphore_updateJoystick=NULL;
  int32_t joystickNormalizedToInt32 = 0;                           // semaphore protected data

bool resetPedalPosition = false;


/**********************************************************************************************/
/*                                                                                            */
/*                         target-specific  definitions                                       */
/*                                                                                            */
/**********************************************************************************************/

// Wiring connections
#if CONFIG_IDF_TARGET_ESP32S2
  #define minPin 11
  #define maxPin 10
  #define dirPinStepper    8//17//8//12//3 
  #define stepPinStepper   9//16//9//13//2  // step pin must be pin 9
#elif CONFIG_IDF_TARGET_ESP32
  #define minPin 34
  #define maxPin 35
  #define dirPinStepper    0//8
  #define stepPinStepper   4//9
#endif


/**********************************************************************************************/
/*                                                                                            */
/*                         controller  definitions                                            */
/*                                                                                            */
/**********************************************************************************************/

#include "Controller.h"




/**********************************************************************************************/
/*                                                                                            */
/*                         pedal mechanics definitions                                        */
/*                                                                                            */
/**********************************************************************************************/

#include "PedalGeometry.h"


/**********************************************************************************************/
/*                                                                                            */
/*                         Kalman filter definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "SignalFilter.h"
KalmanFilter* kalman = NULL;


/**********************************************************************************************/
/*                                                                                            */
/*                         FIR notch filter definitions                                       */
/*                                                                                            */
/**********************************************************************************************/

#include "SignalFilterFirNotch.h"
FirNotchFilter* firNotchFilter = NULL;



/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell definitions                                               */
/*                                                                                            */
/**********************************************************************************************/

#include "LoadCell.h"
LoadCell_ADS1256* loadcell = NULL;



/**********************************************************************************************/
/*                                                                                            */
/*                         stepper motor definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "StepperWithLimits.h"
StepperWithLimits* stepper = NULL;
static const int32_t MIN_STEPS = 5;

#include "StepperMovementStrategy.h"








/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
void setup()
{
  //Serial.begin(115200);
  Serial.begin(921600);
  Serial.setTimeout(5);
void sendConfig(DAP_config_st data);
  

  

  // init controller
  SetupController();

  delay(1000);

  stepper = new StepperWithLimits(stepPinStepper, dirPinStepper, minPin, maxPin);
  loadcell = new LoadCell_ADS1256();

  loadcell->setZeroPoint();
  #ifdef ESTIMATE_LOADCELL_VARIANCE
    loadcell->estimateVariance();       // automatically identify sensor noise for KF parameterization
  #endif

  // find the min & max endstops
  stepper->findMinMaxEndstops();
  Serial.print("Min Position is "); Serial.println(stepper->getLimitMin());
  Serial.print("Max Position is "); Serial.println(stepper->getLimitMax());


  // setup Kalman filter
  Serial.print("Given loadcell variance: ");
  Serial.println(loadcell->getVarianceEstimate());
  kalman = new KalmanFilter(loadcell->getVarianceEstimate());


  // setup FIR filter
  firNotchFilter = new FirNotchFilter(15);






  // initialize configuration and update local variables
  #ifdef PEDAL_IS_BRAKE
    dap_config_st.initialiseDefaults();
  #endif

  #ifdef PEDAL_IS_ACCELERATOR
    dap_config_st.initialiseDefaults_Accelerator();
  #endif

  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(sizeof(DAP_config_st));
  dap_config_st.loadConfigFromEprom(dap_config_st);

  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st);

  // activate parameter update in first cycle
  configUpdateAvailable = true;
  // equalize pedal config for both tasks
  dap_config_st_local = dap_config_st;





  // setup multi tasking
  semaphore_updateJoystick = xSemaphoreCreateMutex();
  semaphore_updateConfig = xSemaphoreCreateMutex();
  delay(10);


  if(semaphore_updateJoystick==NULL)
  {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }
  if(semaphore_updateConfig==NULL)
  {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }

  disableCore0WDT();

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    pedalUpdateTask,   /* Task function. */
                    "pedalUpdateTask",     /* name of task. */
                    10000,       /* Stack size of task */
                    //STACK_SIZE_FOR_TASK_1,
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
                    serialCommunicationTask,   
                    "serialCommunicationTask", 
                    10000,  
                    //STACK_SIZE_FOR_TASK_2,    
                    NULL,      
                    1,         
                    &Task2,    
                    1);     
  delay(500);



  

  Serial.println("Setup end!");
  
}




/**********************************************************************************************/
/*                                                                                            */
/*                         Calc update function                                               */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters()
{

  dap_calculationVariables_st.updateFromConfig(dap_config_st);
  dap_calculationVariables_st.updateEndstops(stepper->getLimitMin(), stepper->getLimitMax());
  stepper->updatePedalMinMaxPos(dap_config_st.payLoadPedalConfig_.pedalStartPosition, dap_config_st.payLoadPedalConfig_.pedalEndPosition);
  //stepper->findMinMaxLimits(dap_config_st.payLoadPedalConfig_.pedalStartPosition, dap_config_st.payLoadPedalConfig_.pedalEndPosition);
  dap_calculationVariables_st.updateStiffness();

  // tune the PID settings
  tunePidValues(dap_config_st);

  // equalize pedal config for both tasks
  dap_config_st_local = dap_config_st;






}



/**********************************************************************************************/
/*                                                                                            */
/*                         Main function                                                      */
/*                                                                                            */
/**********************************************************************************************/
void loop() {
}


/**********************************************************************************************/
/*                                                                                            */
/*                         pedal update task                                                  */
/*                                                                                            */
/**********************************************************************************************/


//long lastCallTime = micros();
unsigned long cycleTimeLastCall = micros();
unsigned long minCyclesForFirToInit = 1000;
unsigned long firCycleIncrementer = 0;


unsigned long printCycleCounter = 0;


//void loop()
void pedalUpdateTask( void * pvParameters )
{

  for(;;){


    // system identification mode
    #ifdef ALLOW_SYSTEM_IDENTIFICATION
      if (systemIdentificationMode_b == true)
      {
        measureStepResponse(stepper, &dap_calculationVariables_st, &dap_config_st, loadcell);
        systemIdentificationMode_b = false;
      }
    #endif
    

    // controll cycle time. Delay did not work with the multi tasking, thus this workaround was integrated
    unsigned long now = micros();
    if (now - cycleTimeLastCall < PUT_TARGET_CYCLE_TIME_IN_US) // 100us = 10kHz
    {
      // skip 
      continue;
    }
    {
      // if target cycle time is reached, update last time
      cycleTimeLastCall = now;
    }

    

    // print the execution time averaged over multiple cycles 
    #ifdef PRINT_CYCLETIME
      timerPU.Bump();
    #endif


    // if a config update was received over serial, update the variables required for further computation
    if (configUpdateAvailable == true)
    {
      if(semaphore_updateConfig!=NULL)
      {

        bool configWasUpdated_b = false;
        // Take the semaphore and just update the config file, then release the semaphore
        if(xSemaphoreTake(semaphore_updateConfig, 1)==pdTRUE)
        {
          Serial.println("Update pedal config!");
          configUpdateAvailable = false;
          dap_config_st = dap_config_st_local;
          configWasUpdated_b = true;
          xSemaphoreGive(semaphore_updateConfig);
        }

        // update the calc params
        if (true == configWasUpdated_b)
        {
          Serial.println("Updating the calc params!");
          configWasUpdated_b = false;
          dap_config_st.storeConfigToEprom(dap_config_st); // store config to EEPROM
          updatePedalCalcParameters(); // update the calc parameters
        }

      }
      else
      {
        Serial.println("semaphore_updateConfig == 0");
      }
    }



    // if reset pedal position was requested, reset pedal now
    // This function is implemented, so that in case of lost steps, the user can request a reset of the pedal psotion
    if (resetPedalPosition) {
      stepper->refindMinLimit();
      resetPedalPosition = false;
    }


    //#define RECALIBRATE_POSITION
    #ifdef RECALIBRATE_POSITION
      stepper->checkLimitsAndResetIfNecessary();
    #endif


      // compute pedal oscillation, when ABS is active
    float absForceOffset_fl32 = 0.0f;
    #ifdef ABS_OSCILLATION
      absForceOffset_fl32 = absOscillation.forceOffset(&dap_calculationVariables_st);
    #endif


    // compute the pedal incline angle 
    //#define COMPUTE_PEDAL_INCLINE_ANGLE
    #ifdef COMPUTE_PEDAL_INCLINE_ANGLE
      float sledPosition = sledPositionInMM(stepper);
      float pedalInclineAngle = pedalInclineAngleDeg(sledPosition, dap_config_st);

      // compute angular velocity & acceleration of incline angke
      float pedalInclineAngle_Accel = pedalInclineAngleAccel(pedalInclineAngle);

      //float legRotationalMoment = 0.0000001;
      //float forceCorrection = pedalInclineAngle_Accel * legRotationalMoment;

      //Serial.print(pedalInclineAngle_Accel);
      //Serial.println(" ");

    #endif


    // Get the loadcell reading
    float loadcellReading = loadcell->getReadingKg();

    // Do the loadcell signal filtering
    float filteredReading = kalman->filteredValue(loadcellReading, 0);
    float changeVelocity = kalman->changeVelocity();



    // Apply FIR notch filter to reduce force oscillation caused by ABS
    //#define APPLY_FIR_FILTER
    #ifdef APPLY_FIR_FILTER
      float filteredReading2 = firNotchFilter->filterValue(loadcellReading);
      if (firCycleIncrementer > minCyclesForFirToInit)
      {
        filteredReading = filteredReading2;
      }
      else
      {
          firCycleIncrementer++;
      }

      firCycleIncrementer++;
      /*if (firCycleIncrementer % 500 == 0)
      { 
        firCycleIncrementer = 0;
        Serial.print(filteredReading);
        Serial.print(",   ");
        Serial.print(filteredReading2);
        Serial.println("   ");
      }*/
      
    #endif
    

    //#define DEBUG_FILTER
    #ifdef DEBUG_FILTER
      static RTDebugOutput<float, 2> rtDebugFilter({ "rawReading_g", "filtered_g"});
      rtDebugFilter.offerData({ loadcellReading * 1000, filteredReading * 1000});
    #endif
      

    /*#ifdef ABS_OSCILLATION
      filteredReading += forceAbsOffset;
    #endif*/

    // use interpolation to determine local linearized spring stiffness
    double stepperPosFraction = stepper->getCurrentPositionFraction();
    //int32_t Position_Next = MoveByInterpolatedStrategy(filteredReading, stepperPosFraction, &forceCurve, &dap_calculationVariables_st, &dap_config_st);
    int32_t Position_Next = MoveByPidStrategy(filteredReading, stepperPosFraction, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_st, absForceOffset_fl32);


    
    //stepper->printStates();
    

    // add dampening
    if (dap_calculationVariables_st.dampingPress  > 0.0001)
    {
      // dampening is proportional to velocity --> D-gain for stability
      Position_Next -= dap_calculationVariables_st.dampingPress * changeVelocity * dap_calculationVariables_st.springStiffnesssInv;
    }
      


    

  
    // clip target position to configured target interval
    Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax);


    // get current stepper position right before sheduling a new move
    //int32_t stepperPosCurrent = stepper->getCurrentPositionSteps();
    int32_t stepperPosCurrent = stepper->getTargetPositionSteps();
    int32_t movement = abs(stepperPosCurrent - Position_Next);
    if (movement > MIN_STEPS)
    {
      stepper->moveTo(Position_Next, false);
    }

    // compute controller output
    if(semaphore_updateJoystick!=NULL)
    {
      if(xSemaphoreTake(semaphore_updateJoystick, 1)==pdTRUE) {
        joystickNormalizedToInt32 = NormalizeControllerOutputValue(filteredReading, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_st.payLoadPedalConfig_.maxGameOutput);
        xSemaphoreGive(semaphore_updateJoystick);
      }
    }
    else
    {
      Serial.println("semaphore_updateJoystick == 0");
    }

    #ifdef PRINT_USED_STACK_SIZE
      unsigned int temp2 = uxTaskGetStackHighWaterMark(nullptr);
      Serial.print("PU task stack size="); Serial.println(temp2);
    #endif
  }
}

  








/**********************************************************************************************/
/*                                                                                            */
/*                         communication task                                                 */
/*                                                                                            */
/**********************************************************************************************/
// https://www.tutorialspoint.com/cyclic-redundancy-check-crc-in-arduino
uint16_t checksumCalculator(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}




int32_t joystickNormalizedToInt32_local = 0;
void serialCommunicationTask( void * pvParameters )
{

  for(;;){

      
      

    // average cycle time averaged over multiple cycles 
    #ifdef PRINT_CYCLETIME
      timerSC.Bump();
    #endif





    // read serial input 
    byte n = Serial.available();

    // likely config structure 
    if ( n == sizeof(DAP_config_st) )
    {
      
      if(semaphore_updateConfig!=NULL)
      {
        if(xSemaphoreTake(semaphore_updateConfig, 1)==pdTRUE)
        {
          DAP_config_st * dap_config_st_local_ptr;
          dap_config_st_local_ptr = &dap_config_st_local;
          Serial.readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));

          

          // check if data is plausible
          bool structChecker = true;
          if ( dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
            structChecker = false;
            Serial.print("Payload type expected: ");
            Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
            Serial.print(",   Payload type received: ");
            Serial.println(dap_config_st_local.payLoadHeader_.payloadType);
          }
          if ( dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
            structChecker = false;
            Serial.print("Config version expected: ");
            Serial.print(DAP_VERSION_CONFIG);
            Serial.print(",   Config version received: ");
            Serial.println(dap_config_st_local.payLoadHeader_.version);
          }
          // checksum validation
          uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st_local_ptr->payLoadPedalConfig_)), sizeof(dap_config_st_local.payLoadPedalConfig_) );
          if (crc != dap_config_st_local.payLoadHeader_.checkSum){ 
            structChecker = false;
            Serial.print("CRC expected: ");
            Serial.print(crc);
            Serial.print(",   CRC received: ");
            Serial.println(dap_config_st_local.payLoadHeader_.checkSum);
          }


          // if checks are successfull, overwrite global configuration struct
          if (structChecker == true)
          {
            Serial.println("Update pedal config!");
            configUpdateAvailable = true;          
          }
          xSemaphoreGive(semaphore_updateConfig);
        }
      }
    }
    else
    {
      if (n!=0)
      {
        int menuChoice = Serial.parseInt();
        switch (menuChoice) {
          // resset minimum position
          case 1:
            Serial.println("Reset position!");
            resetPedalPosition = true;
            break;

          // toggle ABS
          case 2:
            //Serial.print("Second case:");
            absOscillation.trigger();
            break;
          // de-/activate spline debug 
          case 3:
            Serial.println("Start system identification");
            systemIdentificationMode_b = true;
            break;
			case 4:
            Serial.println("Send Config");
            delay(1000);           
            sendConfig(dap_config_st);
            break; 
          default:
            Serial.println("Default case:");
            break;
        }

      }
    }


    // transmit controller output
    if (IsControllerReady()) {
      if(semaphore_updateJoystick!=NULL)
      {
        if(xSemaphoreTake(semaphore_updateJoystick, 1)==pdTRUE)
        {
          joystickNormalizedToInt32_local = joystickNormalizedToInt32;
          xSemaphoreGive(semaphore_updateJoystick);
        }
        else
        {
          Serial.println("semaphore_updateJoystick == 0");
        }
      }
      SetControllerOutputValue(joystickNormalizedToInt32_local);
    }

  }
}
