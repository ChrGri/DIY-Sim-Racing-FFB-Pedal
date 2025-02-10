
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779

#define ESTIMATE_LOADCELL_VARIANCE
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER 1
#define DEBUG_INFO_0_STEPPER_POS 2
#define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS 8
#define DEBUG_INFO_0_PRINT_ALL_SERVO_REGISTERS 16
#define DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT 32
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT 64
#define DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS 128




//#define PI 3.14159267
#define DEG_TO_RAD_FL32 0.017453292519943295769236907684886f

#include "Arduino.h"
#include "Main.h"
#include "Version_Board.h"
#ifdef Using_analog_output_ESP32_S3
#include <Wire.h>
#include <Adafruit_MCP4725.h>
  TwoWire MCP4725_I2C= TwoWire(1);
  //MCP4725 MCP(0x60, &MCP4725_I2C);
  Adafruit_MCP4725 dac;
  int current_use_mcp_index;
  bool MCP_status =false;
#endif



//#define ALLOW_SYSTEM_IDENTIFICATION

/**********************************************************************************************/
/*                                                                                            */
/*                         function declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters();
void pedalUpdateTask( void * pvParameters );
void serialCommunicationTask( void * pvParameters );
void servoCommunicationTask( void * pvParameters );
void OTATask( void * pvParameters );
void ESPNOW_SyncTask( void * pvParameters);
#define INCLUDE_vTaskDelete 1
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

// uint16_t checksumCalculator_withLog(uint8_t * data, uint16_t length)
// {
//    uint16_t curr_crc = 0x0000;
//    uint8_t sum1 = (uint8_t) curr_crc;
//    uint8_t sum2 = (uint8_t) (curr_crc >> 8);
//    int index;
//    for(index = 0; index < length; index = index+1)
//    {
//       sum1 = (sum1 + data[index]) % 255;
//       sum2 = (sum2 + sum1) % 255;

//       Serial.print("index: ");
//       Serial.print(index);

//       Serial.print(",    data: ");
//       Serial.print(data[index]);

//       Serial.print(",    Sum1: ");
//       Serial.print(sum1);

//       Serial.print(",    sum2: ");
//       Serial.println(sum2);


//    }
//    return (sum2 << 8) | sum1;
// }



bool systemIdentificationMode_b = false;




bool splineDebug_b = false;



#include <EEPROM.h>
#define EEPROM_offset 15


#include "ABSOscillation.h"
ABSOscillation absOscillation;
RPMOscillation _RPMOscillation;
BitePointOscillation _BitePointOscillation;
G_force_effect _G_force_effect;
WSOscillation _WSOscillation;
Road_impact_effect _Road_impact_effect;
Custom_vibration CV1;
Custom_vibration CV2;
Rudder _rudder;
Rudder_G_Force _rudder_g_force;
#define ABS_OSCILLATION



#include "DiyActivePedal_types.h"
DAP_config_st dap_config_st;
DAP_calculationVariables_st dap_calculationVariables_st;
DAP_state_basic_st dap_state_basic_st;
DAP_state_extended_st dap_state_extended_st;
DAP_ESPPairing_st dap_esppairing_st;//saving
DAP_ESPPairing_st dap_esppairing_lcl;//sending

#include "CycleTimer.h"





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
#ifndef CONFIG_IDF_TARGET_ESP32S3
  #include "soc/rtc_wdt.h"
#endif

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



static SemaphoreHandle_t semaphore_updatePedalStates=NULL;

/**********************************************************************************************/
/*                                                                                            */
/*                         target-specific  definitions                                       */
/*                                                                                            */
/**********************************************************************************************/




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
float motorRevolutionsPerSteps_fl32 = 1.0f / 3200.0f;


/**********************************************************************************************/
/*                                                                                            */
/*                         Kalman filter definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "SignalFilter.h"
KalmanFilter* kalman = NULL;


#include "SignalFilter_2nd_order.h"
KalmanFilter_2nd_order* kalman_2nd_order = NULL;




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
//static const int32_t MIN_STEPS = 5;

#include "StepperMovementStrategy.h"

bool moveSlowlyToPosition_b = false;
/**********************************************************************************************/
/*                                                                                            */
/*                         OTA                                                                */
/*                                                                                            */
/**********************************************************************************************/
//OTA update
#ifdef OTA_update
//#include "ota.h"
#include "OTA_Pull.h"
TaskHandle_t Task4;
char* APhost;
#endif
#ifdef OTA_update_ESP32
  #include "ota.h"
  //#include "OTA_Pull.h"
  TaskHandle_t Task4;
  char* APhost;
#endif

#ifndef OTA_update || OTA_update_ESP32
  #include "ota.h"
#endif


//ESPNOW
#ifdef ESPNOW_Enable
  #include "ESPNOW_lib.h"
  TaskHandle_t Task6;
#endif

#ifdef USING_LED
  #include "soc/soc_caps.h"
  #include <Adafruit_NeoPixel.h>
  #define LEDS_COUNT 1
  #ifdef LED_ENABLE_RGB
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO, NEO_RGB + NEO_KHZ800);
  #else
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO, NEO_GRB + NEO_KHZ800);
  #endif
  #define CHANNEL 0
  #define LED_BRIGHT 30
  /*
  static const crgb_t L_RED = 0xff0000;
  static const crgb_t L_GREEN = 0x00ff00;
  static const crgb_t L_BLUE = 0x0000ff;
  static const crgb_t L_WHITE = 0xe0e0e0;
  static const crgb_t L_YELLOW = 0xffde21;
  static const crgb_t L_ORANGE = 0xffa500;
  static const crgb_t L_CYAN = 0x00ffff;
  static const crgb_t L_PURPLE = 0x800080;
  */
#endif

#ifdef USING_BUZZER
  #include "Buzzer.h"
#endif



/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
void setup()
{

// setup brake resistor pin
#ifdef BRAKE_RESISTOR_PIN
  pinMode(BRAKE_RESISTOR_PIN, OUTPUT);  // Set GPIO13 as an output
  digitalWrite(BRAKE_RESISTOR_PIN, LOW);  // Turn the LED on
#endif



  //Serial.begin(115200);
  //Serial.begin(921600);
  //Serial.begin(512000);
  //
  #ifdef USING_LED
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0xff,0xff);
    pixels.show(); 
  #endif
  
  #ifdef USING_BUZZER
    Buzzer.initialized(ShutdownPin,1);
    Buzzer.single_beep_tone(770,100);
  #endif

  #if PCB_VERSION == 7
    Serial.setTxTimeoutMs(0);
    Serial.begin(921600);
  #else
    Serial.begin(921600, SERIAL_8N1);
    Serial.setTimeout(5);
  #endif
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  #ifndef CONTROLLER_SPECIFIC_VIDPID
    // init controller
    #if defined(BLUETOOTH_GAMEPAD) || defined(USB_JOYSTICK)
    SetupController();
    #endif
    //delay(3000);
  #endif
  //delay(3000);
  Serial.println("This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.");
  Serial.println("Please check github repo for more detail: https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal");
  //printout the github releasing version
  //#ifdef OTA_update
  Serial.print("Board: ");
  Serial.println(CONTROL_BOARD);
  Serial.print("Firmware Version:");
  Serial.println(DAP_FIRMWARE_VERSION);
  //#endif

  
	#ifdef Hardware_Pairing_button
    pinMode(Pairing_GPIO, INPUT_PULLUP);
  #endif

  #ifdef USING_LED
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0x00,0x00);
    pixels.show(); 
  #endif
// initialize configuration and update local variables
  dap_config_st.initialiseDefaults();

  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(2048);
  dap_config_st.loadConfigFromEprom(dap_config_st_local);


  // check validity of data from EEPROM  
  bool structChecker = true;
  uint16_t crc;
  if ( dap_config_st_local.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
    structChecker = false;
    /*Serial.print("Payload type expected: ");
    Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
    Serial.print(",   Payload type received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.payloadType);*/
  }
  if ( dap_config_st_local.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
    structChecker = false;
    /*Serial.print("Config version expected: ");
    Serial.print(DAP_VERSION_CONFIG);
    Serial.print(",   Config version received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.version);*/
  }
  // checksum validation
  crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
  if (crc != dap_config_st_local.payloadFooter_.checkSum){ 
    structChecker = false;
    /*Serial.print("CRC expected: ");
    Serial.print(crc);
    Serial.print(",   CRC received: ");
    Serial.println(dap_config_st_local.payloadFooter_.checkSum);*/
  }






  // if checks are successfull, overwrite global configuration struct
  if (structChecker == true)
  {
    Serial.println("Updating pedal config from EEPROM");
    dap_config_st = dap_config_st_local;          
  }
  else
  {

    Serial.println("Couldn't load config from EPROM due to mismatch: ");

    Serial.print("Payload type expected: ");
    Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
    Serial.print(",   Payload type received: ");
    Serial.println(dap_config_st_local.payLoadHeader_.payloadType);

    
    Serial.print("Target version: ");
    Serial.print(DAP_VERSION_CONFIG);
    Serial.print(",    Source version: ");
    Serial.println(dap_config_st_local.payLoadHeader_.version);

    Serial.print("CRC expected: ");
    Serial.print(crc);
    Serial.print(",   CRC received: ");
    Serial.println(dap_config_st_local.payloadFooter_.checkSum);

  }


  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st);
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x5f,0x5f,0x00);//yellow
      pixels.show(); 
      //delay(3000);
  #endif


  bool invMotorDir = dap_config_st.payLoadPedalConfig_.invertMotorDirection_u8 > 0;
  stepper = new StepperWithLimits(stepPinStepper, dirPinStepper, invMotorDir, dap_calculationVariables_st.stepsPerMotorRevolution); 

  motorRevolutionsPerSteps_fl32 = 1.0f / ( (float)dap_calculationVariables_st.stepsPerMotorRevolution );
  Serial.printf("Steps per motor revolution: %d\n", dap_calculationVariables_st.stepsPerMotorRevolution);

  loadcell = new LoadCell_ADS1256();

  loadcell->setLoadcellRating(dap_config_st.payLoadPedalConfig_.loadcell_rating);

  loadcell->setZeroPoint();
  #ifdef ESTIMATE_LOADCELL_VARIANCE
    loadcell->estimateVariance();       // automatically identify sensor noise for KF parameterization
  #endif

	// find the min & max endstops
	Serial.println("Start homing");
	stepper->findMinMaxSensorless(dap_config_st);

 
  Serial.print("Min Position is "); Serial.println(stepper->getLimitMin());
  Serial.print("Max Position is "); Serial.println(stepper->getLimitMax());


  // setup Kalman filters
  Serial.print("Given loadcell variance: ");
  Serial.println(loadcell->getVarianceEstimate());
  kalman = new KalmanFilter(loadcell->getVarianceEstimate());
  kalman_2nd_order = new KalmanFilter_2nd_order(loadcell->getVarianceEstimate());


  // LED signal 
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0, 0x80, 0x00, 0x80);//purple
      pixels.show(); 
      //delay(3000);
  #endif

  

  // activate parameter update in first cycle
  configUpdateAvailable = true;
  // equalize pedal config for both tasks
  dap_config_st_local = dap_config_st;





  // setup multi tasking
  semaphore_updateJoystick = xSemaphoreCreateMutex();
  semaphore_updateConfig = xSemaphoreCreateMutex();
  semaphore_updatePedalStates = xSemaphoreCreateMutex();
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

  Serial.println("Starting other tasks");

  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    pedalUpdateTask,   /* Task function. */
                    "pedalUpdateTask",     /* name of task. */
                    7000,       /* Stack size of task */
                    //STACK_SIZE_FOR_TASK_1,
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
  delay(500);

  xTaskCreatePinnedToCore(
                    serialCommunicationTask,   
                    "serialCommunicationTask", 
                    5000,  
                    //STACK_SIZE_FOR_TASK_2,    
                    NULL,      
                    1,         
                    &Task2,    
                    0);     
  delay(500);




  //Serial.begin(115200);
  #if defined(OTA_update)  || defined(OTA_update_ESP32)
  
    switch(dap_config_st.payLoadPedalConfig_.pedal_type)
    {
      case 0:
        APhost="FFBPedalClutch";
        break;
      case 1:
        APhost="FFBPedalBrake";
        break;
      case 2:
        APhost="FFBPedalGas";
        break;
      default:
        APhost="FFBPedal";
        break;        

    }   
    //Serial.begin(115200);
    xTaskCreatePinnedToCore(
                    OTATask,   
                    "OTATask", 
                    16000,  
                    //STACK_SIZE_FOR_TASK_2,    
                    NULL,      
                    1,         
                    &Task4,    
                    0);     
    delay(500);
  #endif

  //MCP setup
  #ifdef Using_analog_output_ESP32_S3
    //Wire.begin(MCP_SDA,MCP_SCL,400000);
    MCP4725_I2C.begin(MCP_SDA,MCP_SCL,400000);
    uint8_t i2c_address[8]={0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67};
    int index_address=0;
    int found_address=0;
    int error;
    for(index_address=0;index_address<8;index_address++)
    {
      MCP4725_I2C.beginTransmission(i2c_address[index_address]);
      error = MCP4725_I2C.endTransmission();
      if (error == 0)
      {
        Serial.print("I2C device found at address");
        Serial.print(i2c_address[index_address]);
        Serial.println("  !");
        found_address=index_address;
        break;
        
      }
      else
      {
        Serial.print("try address");
        Serial.println(i2c_address[index_address]);
      }
    }
    
    if(dac.begin(i2c_address[found_address], &MCP4725_I2C)==false)
    {
      Serial.println("Couldn't find MCP, will not have analog output");
      MCP_status=false;
    }
    else
    {
      Serial.println("MCP founded");
      MCP_status=true;
      //MCP.begin();
    }
  #endif

  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
      pixels.show(); 
      //delay(3000);
  #endif

  //print pedal role assignment
  if(dap_config_st.payLoadPedalConfig_.pedal_type!=4)
  {
    Serial.print("Pedal Assignment:");
    Serial.println(dap_config_st.payLoadPedalConfig_.pedal_type);
  }
  else
  {
    #ifdef PEDAL_HARDWARE_ASSIGNMENT
      Serial.println("Pedal Role Assignment:4, reading from CFG pins....");
    #else
      Serial.println("Pedal Role Assignment:4, Role assignment Error, Please send the ocnfig in to finish role assignment");
    #endif
  }
  
  #ifdef PEDAL_HARDWARE_ASSIGNMENT
    pinMode(CFG1, INPUT_PULLUP);
    pinMode(CFG2, INPUT_PULLUP);
    delay(50); // give the pin time to settle
    Serial.println("Overriding Pedal Role Assignment from Hardware switch......");
    uint8_t CFG1_reading=digitalRead(CFG1);
    uint8_t CFG2_reading=digitalRead(CFG2);
    uint8_t Pedal_assignment=CFG1_reading*2+CFG2_reading*1;//00=clutch 01=brk  02=gas
    if(Pedal_assignment==3)
    {
      Serial.println("Pedal Type:3, assignment error, please adjust dip switch on control board to finish role assignment.");
    }
    else
    {
      if(Pedal_assignment!=4)
        {
          //Serial.print("Pedal Type");
          //Serial.println(Pedal_assignment);
          if(Pedal_assignment==0)
          {
            Serial.println("Overriding Pedal as Clutch.");
          }
          if(Pedal_assignment==1)
          {
            Serial.println("Overriding Pedal as Brake.");
          }
          if(Pedal_assignment==2)
          {
            Serial.println("Overriding Pedal as Throttle.");
          }
          dap_config_st.payLoadPedalConfig_.pedal_type=Pedal_assignment;
        }
        else
        {
          Serial.println("Asssignment error, defective pin connection, pelase connect USB and send a config to finish assignment");
        }
    }
    /*
    if(dap_config_st.payLoadPedalConfig_.pedal_type==4)
    {
      Serial.println("Pedal type:4, Pedal not assignment, reading from CFG pins....");
      uint8_t CFG1_reading;
      uint8_t CFG2_reading;
      uint8_t Pedal_assignment;//00=clutch 01=brk  02=gas
      
      CFG1_reading=digitalRead(CFG1);
      CFG2_reading=digitalRead(CFG2);
      Pedal_assignment=CFG1_reading*2+CFG2_reading*1;
      if(Pedal_assignment==3)
      {
        Serial.println("Pedal Type:3, assignment error, please adjust dip switch on control board or connect USB and send a config to finish assignment.");
      }
      else
      {
        if(Pedal_assignment!=4)
        {
          //Serial.print("Pedal Type");
          //Serial.println(Pedal_assignment);
          if(Pedal_assignment==0)
          {
            Serial.println("Pedal is assigned as Clutch, please also send the config in.");
          }
          if(Pedal_assignment==1)
          {
            Serial.println("Pedal is assigned as Brake, please also send the config in.");
          }
          if(Pedal_assignment==2)
          {
            Serial.println("Pedal is assigned as Throttle, please also send the config in.");
          }
          dap_config_st.payLoadPedalConfig_.pedal_type=Pedal_assignment;
        }
        else
        {
          Serial.println("Asssignment error, defective pin connection, pelase connect USB and send a config to finish assignment");
        }
      }

    }
    */
  #endif

  //enable ESP-NOW
  #ifdef ESPNOW_Enable
  dap_calculationVariables_st.rudder_brake_status=false;
  if(dap_config_st.payLoadPedalConfig_.pedal_type==0||dap_config_st.payLoadPedalConfig_.pedal_type==1||dap_config_st.payLoadPedalConfig_.pedal_type==2)
  {
    Serial.println("Starting ESP now tasks");
    ESPNow_initialize();
    xTaskCreatePinnedToCore(
                        ESPNOW_SyncTask,   
                        "ESPNOW_update_Task", 
                        5000,  
                        //STACK_SIZE_FOR_TASK_2,    
                        NULL,      
                        1,         
                        &Task6,    
                        0);     
    delay(500);
  }
  else
  {
    Serial.println("ESPNOW task did not started due to Assignment error, please usb connect to Simhub and finish Assignment.");
  }
  #endif
  #ifdef CONTROLLER_SPECIFIC_VIDPID
    SetupController_USB(dap_config_st.payLoadPedalConfig_.pedal_type);
    delay(500);
  #endif

  Serial.println("Setup end");
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0xff,0x00);//Green
      pixels.show(); 
      //delay(3000);
  #endif

  #ifdef USING_BUZZER
    if(dap_config_st.payLoadPedalConfig_.pedal_type==0)
    {
      delay(500);
      Buzzer.single_beep_ledc_fade(NOTE_D4,3072,1);
      //Buzzer.single_beep_ledc_fade(NOTE_A4,1536,0.5);
    }
    if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
    {
      Buzzer.single_beep_ledc_fade(NOTE_A4,3072,1);
    }    
    if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
    {
      delay(500);
      //Buzzer.single_beep_ledc_fade(NOTE_A4,1536,0.5);
      Buzzer.single_beep_ledc_fade(NOTE_D4,3072,1);
    }    
    //Buzzer.single_beep_tone(440,1500);
  #endif
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
unsigned long joystick_state_last_update=millis();
void loop() {
  taskYIELD();
  /*
  #ifdef OTA_update
  server.handleClient();
  //delay(1);
  #endif
  */
  
  
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

float filteredReading_exp_filter = 0;
unsigned long printCycleCounter = 0;


uint printCntr = 0;


int64_t timeNow_pedalUpdateTask_l = 0;
int64_t timePrevious_pedalUpdateTask_l = 0;
#define REPETITION_INTERVAL_PEDALUPDATE_TASK (int64_t)0


uint32_t pos_printCount = 0;

uint32_t controlTask_stackSizeIdx_u32 = 0;
float Position_Next_Prev = 0.0f;

//IRAM_ATTR DAP_config_st dap_config_pedalUpdateTask_st;
DAP_config_st dap_config_pedalUpdateTask_st;
//void loop()

bool brakeResistorState = false;
void pedalUpdateTask( void * pvParameters )
{

  for(;;){


    // measure callback time and continue, when desired period is reached
    timeNow_pedalUpdateTask_l = millis();
    int64_t timeDiff_pedalUpdateTask_l = ( timePrevious_pedalUpdateTask_l + REPETITION_INTERVAL_PEDALUPDATE_TASK) - timeNow_pedalUpdateTask_l;
    uint32_t targetWaitTime_u32 = constrain(timeDiff_pedalUpdateTask_l, 0, REPETITION_INTERVAL_PEDALUPDATE_TASK);
    delay(targetWaitTime_u32);
    timePrevious_pedalUpdateTask_l = millis();


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
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
    {
      static CycleTimer timerPU("PU cycle time");
      timerPU.Bump();
    }




    // system identification mode
    #ifdef ALLOW_SYSTEM_IDENTIFICATION
      if (systemIdentificationMode_b == true)
      {
        measureStepResponse(stepper, &dap_calculationVariables_st, &dap_config_st, loadcell);
        systemIdentificationMode_b = false;
      }
    #endif
    

    

    

     

    // if a config update was received over serial, update the variables required for further computation
    if (configUpdateAvailable == true)
    {
      if(semaphore_updateConfig!=NULL)
      {

        bool configWasUpdated_b = false;
        // Take the semaphore and just update the config file, then release the semaphore
        if(xSemaphoreTake(semaphore_updateConfig, (TickType_t)1)==pdTRUE)
        {
          Serial.println("Updating pedal config");
          configUpdateAvailable = false;
          dap_config_st = dap_config_st_local;
          configWasUpdated_b = true;
          xSemaphoreGive(semaphore_updateConfig);
        }

        // update the calc params
        if (true == configWasUpdated_b)
        {
          Serial.println("Updating the calc params");
          configWasUpdated_b = false;

          if (true == dap_config_st.payLoadHeader_.storeToEeprom)
          {
            dap_config_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
            uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
            dap_config_st.payloadFooter_.checkSum = crc;
            dap_config_st.storeConfigToEprom(dap_config_st); // store config to EEPROM
          }
          
          updatePedalCalcParameters(); // update the calc parameters
          moveSlowlyToPosition_b = true;
        }

      }
      else
      {
        semaphore_updateConfig = xSemaphoreCreateMutex();
        //Serial.println("semaphore_updateConfig == 0");
      }
    }

    // copy struct to local variable for faster execution
    dap_config_pedalUpdateTask_st = dap_config_st;
  

    //#define RECALIBRATE_POSITION
    #ifdef RECALIBRATE_POSITION
      stepper->checkLimitsAndResetIfNecessary();
    #endif


    // compute pedal oscillation, when ABS is active
    float absForceOffset = 0.0f;
    float absPosOffset = 0.0f;
    dap_calculationVariables_st.Default_pos();
    #ifdef ABS_OSCILLATION
      absOscillation.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.absPattern, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.absForceOrTarvelBit, &absForceOffset, &absPosOffset);
      _RPMOscillation.trigger();
      _RPMOscillation.forceOffset(&dap_calculationVariables_st);
      _BitePointOscillation.forceOffset(&dap_calculationVariables_st);
      _G_force_effect.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.G_multi);
      _WSOscillation.forceOffset(&dap_calculationVariables_st);
      _Road_impact_effect.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Road_multi);
      CV1.forceOffset(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_freq_1,dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_amp_1);
      CV2.forceOffset(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_freq_2,dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_amp_2);
      _rudder_g_force.offset_calculate(&dap_calculationVariables_st);
      dap_calculationVariables_st.update_stepperMaxpos(_rudder_g_force.offset_filter);
      _rudder.offset_calculate(&dap_calculationVariables_st);
      dap_calculationVariables_st.update_stepperMinpos(_rudder.offset_filter);


      //_rudder.force_offset_calculate(&dap_calculationVariables_st);

    #endif

    //update max force with G force effect
      movingAverageFilter.dataPointsCount = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.G_window;
      movingAverageFilter_roadimpact.dataPointsCount = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Road_window;
      dap_calculationVariables_st.reset_maxforce();
      dap_calculationVariables_st.Force_Max += _G_force_effect.G_force;
      dap_calculationVariables_st.Force_Max += _Road_impact_effect.Road_Impact_force;
      dap_calculationVariables_st.dynamic_update();
      dap_calculationVariables_st.updateStiffness();
    


    // Get the loadcell reading
    float loadcellReading = loadcell->getReadingKg();

    // Invert the loadcell reading digitally if desired
    if (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.invertLoadcellReading_u8 == 1)
    {
      loadcellReading *= -1;
    }


    // Convert loadcell reading to pedal force
    float sledPosition = sledPositionInMM(stepper, &dap_config_pedalUpdateTask_st, motorRevolutionsPerSteps_fl32);
    float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, &dap_config_pedalUpdateTask_st);
    float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
    float d_phi_d_x = convertToPedalForceGain(sledPosition, &dap_config_pedalUpdateTask_st);

    // Serial.printf("SledPos:%f,    PedalAngle: %f\n", sledPosition, pedalInclineAngleInDeg_fl32);

    // delay(10);

    // compute gain for horizontal foot model
    float b = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_b;
    float d = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_d;
    float d_x_hor_d_phi = -(b+d) * sinf(pedalInclineAngleInDeg_fl32 * DEG_TO_RAD_FL32);
    d_x_hor_d_phi *= DEG_TO_RAD_FL32; // inner derivative
    
    // Do the loadcell signal filtering
    float filteredReading = 0.0f;
    float changeVelocity = 0.0f;
    float alpha_exp_filter = 1.0f - ( (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_modelNoise) / 5000.0f;
    // const velocity model denoising filter
    switch (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_modelOrder) {
      case 0:
        filteredReading = kalman->filteredValue(pedalForce_fl32, 0.0f, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_modelNoise);
        changeVelocity = kalman->changeVelocity();
        break;
      case 1:
        filteredReading = kalman_2nd_order->filteredValue(pedalForce_fl32, 0.0f, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_modelNoise);
        changeVelocity = kalman_2nd_order->changeVelocity();
        break;
      case 2:
        filteredReading_exp_filter = filteredReading_exp_filter * alpha_exp_filter + pedalForce_fl32 * (1.0f-alpha_exp_filter);
        filteredReading = filteredReading_exp_filter;
        break;
      default:
        filteredReading_exp_filter = filteredReading_exp_filter * alpha_exp_filter + pedalForce_fl32 * (1.0f-alpha_exp_filter);
        filteredReading = filteredReading_exp_filter;
        break;
    }






    //#define DEBUG_FILTER
    if (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_LOADCELL_READING) 
    {
      static RTDebugOutput<float, 3> rtDebugFilter({ "rawReading_g", "pedalForce_fl32", "filtered_g"});
      rtDebugFilter.offerData({ loadcellReading * 1000.0f, pedalForce_fl32*1000.0f, filteredReading * 1000.0f});
    }
      




    float stepperPosFraction = stepper->getCurrentPositionFraction();
    int32_t Position_Next = 0;
    
    // select control loop algo
    switch (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.control_strategy_b) {
      case 0:
        // static PID
        Position_Next = MoveByPidStrategy(filteredReading, stepperPosFraction, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0.0f/*effect_force*/, changeVelocity);
        break;
      case 1:
        // dynamic PID
        Position_Next = MoveByPidStrategy(filteredReading, stepperPosFraction, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0.0f/*effect_force*/, changeVelocity);
        break;
      default:
        // MPC
        Position_Next = MoveByForceTargetingStrategy(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0.0f/*effect_force*/, changeVelocity, d_phi_d_x, d_x_hor_d_phi);
        // Position_Next = MoveByForceTargetingStrategy_old(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0/*effect_force*/, changeVelocity, d_phi_d_x, d_x_hor_d_phi);
        break;
    }


    // float alphaPidOut = 0.9;
    // Position_Next = Position_Next*alphaPidOut + Position_Next_Prev * (1.0f - alphaPidOut);
    // Position_Next_Prev = Position_Next;

    // add dampening
    if (dap_calculationVariables_st.dampingPress  > 0.0001f)
    {
      // dampening is proportional to velocity --> D-gain for stability
      Position_Next -= dap_calculationVariables_st.dampingPress * changeVelocity * dap_calculationVariables_st.springStiffnesssInv;
    }
      


    // clip target position to configured target interval with RPM effect movement in the endstop
    Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax);
    
  
    // //Adding effects
    //Add effect by force
    float effect_force = _BitePointOscillation.BitePoint_Force_offset + _WSOscillation.WS_Force_offset + CV1.CV_Force_offset + CV2.CV_Force_offset;

    if(filteredReading>=dap_calculationVariables_st.Force_Min)
    {
      Position_Next -= absPosOffset;
      effect_force += absForceOffset;
    }
    int32_t Position_effect= effect_force/dap_calculationVariables_st.Force_Range*dap_calculationVariables_st.stepperPosRange;
    Position_Next -=_RPMOscillation.RPM_position_offset;

    Position_Next -= Position_effect;
    Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMinEndstop, dap_calculationVariables_st.stepperPosMaxEndstop);
    
    //bitepoint trigger
    int32_t BP_trigger_value = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.BP_trigger_value;
    int32_t BP_trigger_min = (BP_trigger_value-4);
    int32_t BP_trigger_max = (BP_trigger_value+4);
    int32_t Position_check = 100*((Position_Next-dap_calculationVariables_st.stepperPosMin) / dap_calculationVariables_st.stepperPosRange);
    int32_t Rudder_real_poisiton= 100*((Position_Next-dap_calculationVariables_st.stepperPosMin_default) / dap_calculationVariables_st.stepperPosRange_default);

    dap_calculationVariables_st.current_pedal_position = Position_Next;

    //Rudder initialzing and de initializing
    #ifdef ESPNOW_Enable
    if(dap_calculationVariables_st.Rudder_status)
    {
      if(Rudder_initializing)
      {
        moveSlowlyToPosition_b=true;
        //Serial.println("moving to center");
      }
      if(Rudder_initializing && (Rudder_real_poisiton<51 && Rudder_real_poisiton>49))
      {
        Rudder_initializing=false;
        moveSlowlyToPosition_b=false;
        Serial.println("initialized disable move slow");
      }
    }
    if(Rudder_deinitializing)
    {
      moveSlowlyToPosition_b=true;
        //Serial.println("moving to min end stop");
    }
    if(Rudder_deinitializing && (Rudder_real_poisiton< 2 ))
    {
      Rudder_deinitializing=false;
      moveSlowlyToPosition_b=false;
      Serial.println("deinitialized disable move slow");
    }
    #endif

    //Serial.println(Position_check);
    if(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.BP_trigger==1)
    {
      if(Position_check > BP_trigger_min)
      {
        if(Position_check < BP_trigger_max)
        {
          _BitePointOscillation.trigger();
        }
      }
    }

    // if pedal in min position, recalibrate position --> automatic step loss compensation

    stepper->configSteplossRecovAndCrashDetection(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.stepLossFunctionFlags_u8);
    if (stepper->isAtMinPos())
    {
      #if defined(OTA_update_ESP32) || defined(OTA_update)
        if(OTA_status==false)
        {
          stepper->correctPos();
        }    
      #else
        stepper->correctPos();
      #endif

    }


    // print all servo parameters for debug purposes
    if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS) )
    {
      // clear the debug bit
      dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS);
      delay(1000);  
			stepper->printAllServoParameters();
    }



    // if (pos_printCount == 1000)
    // {
    //   Serial.print("ESP pos: ");
    //   Serial.print(stepper->getCurrentPosition());
    //   Serial.print(", Serovs pos: ");
    //   Serial.print(stepper->getServosPos());
    //   Serial.print(", Serovs pos (corr.): ");
    //   Serial.println(stepper->getServosInternalPosition());
    //   pos_printCount = 0;
    // }
    // pos_printCount++;
    



    // Serial.print("Position next: ");
    // Serial.println(Position_Next);





  // Move to new position
  
  
  if (!moveSlowlyToPosition_b)
  {
    #if defined(OTA_update_ESP32) || defined(OTA_update)
      if(OTA_status==false)
      {
        stepper->moveTo(Position_Next, false);
      }
    #else
      stepper->moveTo(Position_Next, false);
    #endif
  }
  else
  {
    #if defined(OTA_update_ESP32) || defined(OTA_update)
      if(OTA_status==false)
      {
        moveSlowlyToPosition_b = false;
        stepper->moveSlowlyToPos(Position_Next);
      }
    #else
      moveSlowlyToPosition_b = false;
      stepper->moveSlowlyToPos(Position_Next);
    #endif

  }
  


    
     
    

    // compute controller output
    dap_calculationVariables_st.StepperPos_setback();
    dap_calculationVariables_st.reset_maxforce();
    dap_calculationVariables_st.dynamic_update();
    dap_calculationVariables_st.updateStiffness();
    

    // set joystick value
    if(semaphore_updateJoystick!=NULL)
    {
      if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)1)==pdTRUE) {

        
        if(dap_calculationVariables_st.Rudder_status&&dap_calculationVariables_st.rudder_brake_status)
        {
          if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
          {
            joystickNormalizedToInt32 = NormalizeControllerOutputValue((Position_Next-dap_calculationVariables_st.stepperPosRange/2), dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMin+dap_calculationVariables_st.stepperPosRange/2.0f, dap_config_st.payLoadPedalConfig_.maxGameOutput);
            joystickNormalizedToInt32 = constrain(joystickNormalizedToInt32,0,JOYSTICK_MAX_VALUE);
          }
          else
          {
            joystickNormalizedToInt32 = NormalizeControllerOutputValue((filteredReading), dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
          }
        }
        else
        {
          if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
          {
            joystickNormalizedToInt32 = NormalizeControllerOutputValue(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
          }
          else
          {            
            joystickNormalizedToInt32 = NormalizeControllerOutputValue(filteredReading, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
          }
        }
        
        xSemaphoreGive(semaphore_updateJoystick);
      }
    }
    else
    {
      semaphore_updateJoystick = xSemaphoreCreateMutex();
    }

    // provide joystick output on PIN
    #ifdef Using_analog_output
      int dac_value=(int)(joystickNormalizedToInt32*255/10000);
      dacWrite(D_O,dac_value);
    #endif

    #ifdef Using_analog_output_ESP32_S3
      if(MCP_status)
      {
        int dac_value=(int)(joystickNormalizedToInt32*4096*0.9/10000);//limit the max to 5V*0.9=4.5V to prevent the overvolatage
        dac.setVoltage(dac_value, false);
      }
    #endif

    
    float normalizedPedalReading_fl32 = 0.0f;
    if ( fabs(dap_calculationVariables_st.Force_Range) > 0.01f)
    {
        normalizedPedalReading_fl32 = constrain((filteredReading - dap_calculationVariables_st.Force_Min) / dap_calculationVariables_st.Force_Range, 0.0f, 1.0f);
    }
    
    // simulate ABS trigger 
    if(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Simulate_ABS_trigger==1)
    {
      int32_t ABS_trigger_value=dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Simulate_ABS_value;
      if( (normalizedPedalReading_fl32*100.0f) > ABS_trigger_value)
      {
        absOscillation.trigger();
      }
    }

    
//#ifdef UNCOMMENT
    // update pedal states
    if(semaphore_updatePedalStates!=NULL)
    {
      if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)1)==pdTRUE) 
      {
        
        // update basic pedal state struct
        dap_state_basic_st.payloadPedalState_Basic_.pedalForce_u16 =  normalizedPedalReading_fl32 * 65535.0f;
        dap_state_basic_st.payloadPedalState_Basic_.pedalPosition_u16 = constrain(stepperPosFraction, 0.0f, 1.0f) * 65535.0f;
        dap_state_basic_st.payloadPedalState_Basic_.joystickOutput_u16 = (float)joystickNormalizedToInt32 / 10000.0f * 32767.0f;//65535;

        dap_state_basic_st.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_BASIC;
        dap_state_basic_st.payLoadHeader_.version = DAP_VERSION_CONFIG;
        dap_state_basic_st.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_basic_st.payLoadHeader_)), sizeof(dap_state_basic_st.payLoadHeader_) + sizeof(dap_state_basic_st.payloadPedalState_Basic_));
        dap_state_basic_st.payLoadHeader_.PedalTag=dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;
        
        //error code
        dap_state_basic_st.payloadPedalState_Basic_.erroe_code_u8=0;
        #ifdef ESPNOW_Enable
        if(ESPNow_error_code!=0)
        {
          dap_state_basic_st.payloadPedalState_Basic_.erroe_code_u8=ESPNow_error_code;
          ESPNow_error_code=0;
        }
        #endif
        //dap_state_basic_st.payloadPedalState_Basic_.erroe_code_u8=200;
        /*if(isv57.isv57_update_parameter_b)
        {
          dap_state_basic_st.payloadPedalState_Basic_.erroe_code_u8=11;
          isv57.isv57_update_parameter_b=false;
        }*/
        if( stepper->getLifelineSignal()==false )
        {
          dap_state_basic_st.payloadPedalState_Basic_.erroe_code_u8=12;
        }
        // update extended struct 
        dap_state_extended_st.payloadPedalState_Extended_.timeInMs_u32 = millis();
        dap_state_extended_st.payloadPedalState_Extended_.pedalForce_raw_fl32 =  loadcellReading;
        dap_state_extended_st.payloadPedalState_Extended_.pedalForce_filtered_fl32 =  filteredReading;
        dap_state_extended_st.payloadPedalState_Extended_.forceVel_est_fl32 =  changeVelocity;

        //dap_state_extended_st.payloadPedalState_Extended_.servoPosition_i16 = stepper->getServosInternalPosition();
        dap_state_extended_st.payloadPedalState_Extended_.servoPosition_i16 = stepper->getServosInternalPositionCorrected()- stepper->getMinPosition();
        dap_state_extended_st.payloadPedalState_Extended_.servo_voltage_0p1V =  stepper->getServosVoltage();
        dap_state_extended_st.payloadPedalState_Extended_.servo_current_percent_i16 = stepper->getServosCurrent();
        


        //dap_state_extended_st.payloadPedalState_Extended_.servoPositionTarget_i16 = stepper->getCurrentPositionFromMin();
        dap_state_extended_st.payloadPedalState_Extended_.servoPositionTarget_i16 = stepper->getCurrentPosition() - stepper->getMinPosition();
        dap_state_extended_st.payLoadHeader_.PedalTag=dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;
        dap_state_extended_st.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_EXTENDED;
        dap_state_extended_st.payLoadHeader_.version = DAP_VERSION_CONFIG;
        dap_state_extended_st.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_extended_st.payLoadHeader_)), sizeof(dap_state_extended_st.payLoadHeader_) + sizeof(dap_state_extended_st.payloadPedalState_Extended_));

        // release semaphore
        xSemaphoreGive(semaphore_updatePedalStates);
      }
    }
    else
    {
      semaphore_updatePedalStates = xSemaphoreCreateMutex();
    }

//    #endif

    #ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
      if( controlTask_stackSizeIdx_u32 == 1000)
      {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        Serial.print("StackSize (Pedal update): ");
        Serial.println(stackHighWaterMark);
        controlTask_stackSizeIdx_u32 = 0;
      }
      controlTask_stackSizeIdx_u32++;
    #endif

  }
}

  









/**********************************************************************************************/
/*                                                                                            */
/*                         communication task                                                 */
/*                                                                                            */
/**********************************************************************************************/
uint32_t communicationTask_stackSizeIdx_u32 = 0;
int64_t timeNow_serialCommunicationTask_l = 0;
int64_t timePrevious_serialCommunicationTask_l = 0;
#define REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK (int64_t)10

int32_t joystickNormalizedToInt32_local = 0;
void serialCommunicationTask( void * pvParameters )
{

  for(;;){

    // measure callback time and continue, when desired period is reached
    timeNow_serialCommunicationTask_l = millis();
    int64_t timeDiff_serialCommunicationTask_l = ( timePrevious_serialCommunicationTask_l + REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK) - timeNow_serialCommunicationTask_l;
    uint32_t targetWaitTime_u32 = constrain(timeDiff_serialCommunicationTask_l, 0, REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK);
    delay(targetWaitTime_u32);
    timePrevious_serialCommunicationTask_l = millis();



    // average cycle time averaged over multiple cycles 
    if (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
    {
      static CycleTimer timerSC("SC cycle time");
      timerSC.Bump();
    }

    uint16_t crc;




    //delay( SERIAL_COOMUNICATION_TASK_DELAY_IN_MS );

   
    { 
      // read serial input 
      uint8_t n = Serial.available();

      bool structChecker = true;
      
      if (n > 0)
      {
        switch (n) {

          // likely config structure 
          case sizeof(DAP_config_st):
              
              if(semaphore_updateConfig!=NULL)
              {
                if(xSemaphoreTake(semaphore_updateConfig, (TickType_t)1)==pdTRUE)
                {
                  DAP_config_st * dap_config_st_local_ptr;
                  dap_config_st_local_ptr = &dap_config_st_local;
                  Serial.readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));

                  

                  // check if data is plausible
                  
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
                  crc = checksumCalculator((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));
                  if (crc != dap_config_st_local.payloadFooter_.checkSum){ 
                    structChecker = false;
                    Serial.print("CRC expected: ");
                    Serial.print(crc);
                    Serial.print(",   CRC received: ");
                    Serial.println(dap_config_st_local.payloadFooter_.checkSum);

                    Serial.print("Headersize: ");
                    Serial.print(sizeof(dap_config_st_local.payLoadHeader_));
                    Serial.print(",    Configsize: ");
                    Serial.println(sizeof(dap_config_st_local.payLoadPedalConfig_));

                    // uint8_t * testOut = (uint8_t*)dap_config_st_local_ptr;
                    // for (uint16_t byteIdx = 0; byteIdx < sizeof(DAP_config_st); byteIdx++ )
                    // {

                    //   if(byteIdx % 10== 0)
                    //   {
                    //     Serial.println("");
                    //   }

                    //   Serial.print(*(testOut+byteIdx));
                    //   Serial.print(", ");

                    // }
                    // Serial.println("");
                    // Serial.println("");

                    // crc = checksumCalculator_withLog((uint8_t*)(&(dap_config_st_local.payLoadHeader_)), sizeof(dap_config_st_local.payLoadHeader_) + sizeof(dap_config_st_local.payLoadPedalConfig_));

                  }


                  // if checks are successfull, overwrite global configuration struct
                  if (structChecker == true)
                  {
                    Serial.println("Updating pedal config");
                    configUpdateAvailable = true;  
                    #ifdef USING_BUZZER
                      Buzzer.single_beep_tone(700,100);
                    #endif        
                  }
                  xSemaphoreGive(semaphore_updateConfig);
                }
              }
            break;

          // likely action structure 
          case sizeof(DAP_actions_st) :

            DAP_actions_st dap_actions_st;
            Serial.readBytes((char*)&dap_actions_st, sizeof(DAP_actions_st));

            if ( dap_actions_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_ACTION ){ 
              structChecker = false;
              Serial.print("Payload type expected: ");
              Serial.print(DAP_PAYLOAD_TYPE_ACTION);
              Serial.print(",   Payload type received: ");
              Serial.println(dap_config_st_local.payLoadHeader_.payloadType);
            }
            if ( dap_actions_st.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
              structChecker = false;
              Serial.print("Config version expected: ");
              Serial.print(DAP_VERSION_CONFIG);
              Serial.print(",   Config version received: ");
              Serial.println(dap_config_st_local.payLoadHeader_.version);
            }
            crc = checksumCalculator((uint8_t*)(&(dap_actions_st.payLoadHeader_)), sizeof(dap_actions_st.payLoadHeader_) + sizeof(dap_actions_st.payloadPedalAction_));
            if (crc != dap_actions_st.payloadFooter_.checkSum){ 
              structChecker = false;
              Serial.print("CRC expected: ");
              Serial.print(crc);
              Serial.print(",   CRC received: ");
              Serial.println(dap_actions_st.payloadFooter_.checkSum);
            }



            if (structChecker == true)
            {

              //2= restart pedal
              if (dap_actions_st.payloadPedalAction_.system_action_u8==2)
              {
                Serial.println("ESP restart by user request");
                ESP.restart();
              }
              //3= Wifi OTA
              #ifdef ESPNOW_Enable
              if (dap_actions_st.payloadPedalAction_.system_action_u8==3)
              {
                Serial.println("Get OTA command");
                OTA_enable_b=true;
                //OTA_enable_start=true;
                ESPNow_OTA_enable=false;
              }
              #endif
              //4 Enable pairing
              if (dap_actions_st.payloadPedalAction_.system_action_u8==4)
              {
                #ifdef ESPNow_Pairing_function
                  Serial.println("Get Pairing command");
                  software_pairing_action_b=true;
                #endif
                #ifndef ESPNow_Pairing_function
                  Serial.println("no supporting command");
                #endif
              }
              
              if (dap_actions_st.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
              {
                #ifdef ESPNow_S3
                  Serial.println("Restart into Download mode");
                  delay(1000);
                  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
                  ESP.restart();
                #else
                  Serial.println("Command not supported");
                  delay(1000);
                #endif
                //ESPNOW_BootIntoDownloadMode = false;
              }
              

              // trigger ABS effect
              if (dap_actions_st.payloadPedalAction_.triggerAbs_u8>0)
              {
                absOscillation.trigger();
                dap_calculationVariables_st.TrackCondition=dap_actions_st.payloadPedalAction_.triggerAbs_u8-1;
              }
              //RPM effect
              _RPMOscillation.RPM_value=dap_actions_st.payloadPedalAction_.RPM_u8;
              //G force effect
              _G_force_effect.G_value=dap_actions_st.payloadPedalAction_.G_value-128;       
              //wheel slip
              if (dap_actions_st.payloadPedalAction_.WS_u8)
              {
                _WSOscillation.trigger();
              }     
              //Road impact
              if(dap_calculationVariables_st.Rudder_status==false)
              {
                _Road_impact_effect.Road_Impact_value=dap_actions_st.payloadPedalAction_.impact_value_u8;
              }
              else
              {

              }
              
              // trigger system identification
              if (dap_actions_st.payloadPedalAction_.startSystemIdentification_u8)
              {
                systemIdentificationMode_b = true;
              }
              // trigger Custom effect effect 1
              if (dap_actions_st.payloadPedalAction_.Trigger_CV_1)
              {
                CV1.trigger();
              }
              // trigger Custom effect effect 2
              if (dap_actions_st.payloadPedalAction_.Trigger_CV_2)
              {
                CV2.trigger();
              }
              // trigger return pedal position
              if (dap_actions_st.payloadPedalAction_.returnPedalConfig_u8)
              {
                DAP_config_st * dap_config_st_local_ptr;
                dap_config_st_local_ptr = &dap_config_st;
                //uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                crc = checksumCalculator((uint8_t*)(&(dap_config_st.payLoadHeader_)), sizeof(dap_config_st.payLoadHeader_) + sizeof(dap_config_st.payLoadPedalConfig_));
                dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
                Serial.write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
                Serial.print("\r\n");
              }
              #ifdef ESPNOW_Enable
                if(dap_actions_st.payloadPedalAction_.Rudder_action==1)//Enable Rudder
                {
                  if(dap_calculationVariables_st.Rudder_status==false)
                  {
                    dap_calculationVariables_st.Rudder_status=true;
                    Serial.println("Rudder on");
                    Rudder_initializing=true;
                    moveSlowlyToPosition_b=true;
                    //Serial.print("status:");
                    //Serial.println(dap_calculationVariables_st.Rudder_status);
                  }
                  else
                  {
                    dap_calculationVariables_st.Rudder_status=false;
                    Serial.println("Rudder off");
                    Rudder_deinitializing=true;
                    moveSlowlyToPosition_b=true; 

                    //Serial.print("status:");
                    //Serial.println(dap_calculationVariables_st.Rudder_status);
                  }
                }
                if(dap_actions_st.payloadPedalAction_.Rudder_brake_action==1)
                {
                  if(dap_calculationVariables_st.rudder_brake_status==false&&dap_calculationVariables_st.Rudder_status==true)
                  {
                    dap_calculationVariables_st.rudder_brake_status=true;
                    Serial.println("Rudder brake on");
                    //Serial.print("status:");
                    //Serial.println(dap_calculationVariables_st.Rudder_status);
                  }
                  else
                  {
                    dap_calculationVariables_st.rudder_brake_status=false;
                    Serial.println("Rudder brake off");
                    //Serial.print("status:");
                    //Serial.println(dap_calculationVariables_st.Rudder_status);
                  }
                }
                //clear rudder status
                if(dap_actions_st.payloadPedalAction_.Rudder_action==2)
                {
                  dap_calculationVariables_st.Rudder_status=false;
                  dap_calculationVariables_st.rudder_brake_status=false;
                  Serial.println("Rudder Status Clear");
                  Rudder_deinitializing=true;
                  moveSlowlyToPosition_b=true;

                }
              #endif


            }

            break;
          case sizeof(Basic_WIfi_info) : 
          Serial.println("get basic wifi info");
          Serial.readBytes((char*)&_basic_wifi_info, sizeof(Basic_WIfi_info));
          #ifdef OTA_update
            if(_basic_wifi_info.device_ID==dap_config_st.payLoadPedalConfig_.pedal_type)
            {
              SSID=new char[_basic_wifi_info.SSID_Length+1];
              PASS=new char[_basic_wifi_info.PASS_Length+1];
              memcpy(SSID,_basic_wifi_info.WIFI_SSID,_basic_wifi_info.SSID_Length);
              memcpy(PASS,_basic_wifi_info.WIFI_PASS,_basic_wifi_info.PASS_Length);
              SSID[_basic_wifi_info.SSID_Length]=0;
              PASS[_basic_wifi_info.PASS_Length]=0;
              OTA_enable_b=true;
            }
          #endif
          
          break;
          default:

            // flush the input buffer
            while (Serial.available()) Serial.read();
            //Serial.flush();

            Serial.println("\nIn byte size: ");
            Serial.println(n);
            Serial.println("    Exp config size: ");
            Serial.println(sizeof(DAP_config_st) );
            Serial.println("    Exp action size: ");
            Serial.println(sizeof(DAP_actions_st) );

            break;  


            

        }
      }


      // send pedal state structs
      // update pedal states
      printCycleCounter++;
      DAP_state_basic_st dap_state_basic_st_lcl;
      DAP_state_extended_st dap_state_extended_st_lcl;

      if(semaphore_updatePedalStates!=NULL)
      {
        
        if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)1)==pdTRUE) 
        {
        
          // UPDATE basic pedal state struct
          dap_state_basic_st_lcl = dap_state_basic_st;

          // UPDATE extended pedal state struct
          dap_state_extended_st_lcl = dap_state_extended_st;
            
          // release semaphore
          xSemaphoreGive(semaphore_updatePedalStates);

        }
      }
      else
      {
        semaphore_updatePedalStates = xSemaphoreCreateMutex();
      }



      // send the pedal state structs
      // send basic pedal state struct
      if ( !(dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT) )
      {
        if (printCycleCounter >= 2)
        {
          printCycleCounter = 0;
          Serial.write((char*)&dap_state_basic_st_lcl, sizeof(DAP_state_basic_st));
          Serial.print("\r\n");
        }
      }

      if ( (dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT) )
      {
        Serial.write((char*)&dap_state_extended_st_lcl, sizeof(DAP_state_extended_st));
        Serial.print("\r\n");
      }

    }

    delay( SERIAL_COOMUNICATION_TASK_DELAY_IN_MS );
    if(semaphore_updateJoystick!=NULL)
    {
      if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)1)==pdTRUE)
      {
         //Serial.print(" 3");
        joystickNormalizedToInt32_local = joystickNormalizedToInt32;
        xSemaphoreGive(semaphore_updateJoystick);
      }
    }
    #if defined(USB_JOYSTICK) || defined(BLUETOOTH_GAMEPAD)
      if (IsControllerReady()) 
      {
        if(dap_calculationVariables_st.Rudder_status==false)
        {
          //general output
          SetControllerOutputValue(joystickNormalizedToInt32_local);
        }
      }
    #endif

    #ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
      if( communicationTask_stackSizeIdx_u32 == 1000)
			{
				UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
				Serial.print("StackSize (Serial communication): ");
				Serial.println(stackHighWaterMark);
				communicationTask_stackSizeIdx_u32 = 0;
			}
			communicationTask_stackSizeIdx_u32++;
    #endif

  }
}



//OTA multitask
uint16_t OTA_count=0;
bool message_out_b=false;
bool OTA_enable_start=false;
uint32_t otaTask_stackSizeIdx_u32 = 0;
int OTA_update_status=99;
void OTATask( void * pvParameters )
{

  for(;;)
  {
    
    if(OTA_count>200)
    {
      message_out_b=true;
      OTA_count=0;
    }
    else
    {
      OTA_count++;
    }

    #if defined(OTA_update)  || defined(OTA_update_ESP32)
    if(OTA_enable_b)
    {
      if(message_out_b)
      {
        message_out_b=false;
        Serial1.println("OTA enable flag on");
      }
      if(OTA_status)
      {
        #ifdef OTA_update_ESP32
          server.handleClient();
        #endif
        #ifdef OTA_update
          if(OTA_update_status==0)
          {
            #ifdef USING_BUZZER
              Buzzer.play_melody_tone(melody_victory_theme, sizeof(melody_victory_theme)/sizeof(melody_victory_theme[0]),melody_durations_Victory_theme);              
            #endif
            ESP.restart();
          }
          else
          {
            #ifdef USING_BUZZER
              Buzzer.single_beep_tone(770,100);
            #endif
            #ifdef USING_LED
            pixels.setPixelColor(0,0xff,0x00,0x00);//red
            pixels.show(); 
            delay(500);
            pixels.setPixelColor(0,0x00,0x00,0x00);//no color
            pixels.show();
            delay(500);    
            #endif 
          }

        #endif
        

      }
      else
      {
        Serial.println("de-initialize espnow");
        Serial.println("wait...");
        #ifdef ESPNOW_Enable
          esp_err_t result= esp_now_deinit();
          ESPNow_initial_status=false;
          ESPNOW_status=false;
        #else
          esp_err_t result = ESP_OK;
        #endif
        delay(3000);
        if(result==ESP_OK)
        {
          OTA_status=true;
          #ifdef USING_BUZZER
            Buzzer.single_beep_tone(700,100);
          #endif 
          delay(1000);
          #ifdef OTA_update_ESP32
          ota_wifi_initialize(APhost);
          #endif
          #ifdef USING_LED
              //pixels.setBrightness(20);
              pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
              pixels.show(); 
              //delay(3000);
          #endif
          #ifdef OTA_update
          wifi_initialized(SSID,PASS);
          delay(2000);
          ESP32OTAPull ota;
          int ret;
          ota.SetCallback(OTAcallback);
          ota.OverrideBoard(CONTROL_BOARD);
          char* version_tag;
          if(_basic_wifi_info.wifi_action==1)
          {
            version_tag="0.0.0";
            Serial.println("Force update");
          }
          else
          {
            version_tag=DAP_FIRMWARE_VERSION;
          }
          switch (_basic_wifi_info.mode_select)
          {
            case 1:
              Serial.printf("Flashing to latest Main, checking %s to see if an update is available...\n", JSON_URL_main);
              ret = ota.CheckForOTAUpdate(JSON_URL_main, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
              Serial.printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
              OTA_update_status=ret;
              break;
            case 2:
              Serial.printf("Flashing to latest Dev, checking %s to see if an update is available...\n", JSON_URL_dev);
              ret = ota.CheckForOTAUpdate(JSON_URL_dev, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
              Serial.printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
              OTA_update_status=ret;
              break;
            default:
            break;
          }
          #endif

          delay(3000);
        }

      }
    }
    
    delay(1);
    #endif

    #ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
      if( otaTask_stackSizeIdx_u32 == 1000)
      {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        Serial.print("StackSize (OTA): ");
        Serial.println(stackHighWaterMark);
        otaTask_stackSizeIdx_u32 = 0;
      }
      otaTask_stackSizeIdx_u32++;
    #endif
    delay(2);
  }
}

#ifdef ESPNOW_Enable
int ESPNOW_count=0;
int error_count=0;
int print_count=0;
int ESPNow_no_device_count=0;
bool basic_state_send_b=false;
bool extend_state_send_b=false;
uint8_t error_out;

int64_t timeNow_espNowTask_l = 0;
int64_t timePrevious_espNowTask_l = 0;
#define REPETITION_INTERVAL_ESPNOW_TASK (int64_t)2

uint Pairing_timeout=20000;
bool Pairing_timeout_status=false;
bool building_dap_esppairing_lcl =false;
unsigned long Pairing_state_start;
unsigned long Pairing_state_last_sending;
unsigned long Debug_rudder_last=0;

uint32_t espNowTask_stackSizeIdx_u32 = 0;
void ESPNOW_SyncTask( void * pvParameters )
{
  for(;;)
  {
    //if(ESPNOW_status)

    // measure callback time and continue, when desired period is reached
    timeNow_espNowTask_l = millis();
    int64_t timeDiff_espNowTask_l = ( timePrevious_espNowTask_l + REPETITION_INTERVAL_ESPNOW_TASK) - timeNow_espNowTask_l;
    uint32_t targetWaitTime_u32 = constrain(timeDiff_espNowTask_l, 0, REPETITION_INTERVAL_ESPNOW_TASK);
    delay(targetWaitTime_u32); 
    timePrevious_espNowTask_l = millis();


    //restart from espnow
    if(ESPNow_restart)
    {
      Serial.println("ESP restart by ESP now request");
      ESP.restart();
    }

    
    //basic state sendout interval
    if(ESPNOW_count%9==0)
    {
      basic_state_send_b=true;
      
    }
    //entend state send out interval
    if(ESPNOW_count%13==0 && dap_config_st.payLoadPedalConfig_.debug_flags_0 == DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT)
    {
      extend_state_send_b=true;
      
    }

    
    ESPNOW_count++;
    if(ESPNOW_count>10000)
    {
      ESPNOW_count=0;
    }
    
    if(ESPNow_initial_status==false  )
    {
      if(OTA_enable_b==false)
      {
        ESPNow_initialize();
      }
      
    }
    else
    {
      #ifdef ESPNow_Pairing_function
       #ifdef Hardware_Pairing_button
        if(digitalRead(Pairing_GPIO)==LOW)
        {
          hardware_pairing_action_b=true;
        }
       #endif
        if(hardware_pairing_action_b||software_pairing_action_b)
        {
          Serial.println("Pedal Pairing.....");
          delay(1000);
          Pairing_state_start=millis();
          Pairing_state_last_sending=millis();
          ESPNow_pairing_action_b=true;
          building_dap_esppairing_lcl=true;
          software_pairing_action_b=false;
          hardware_pairing_action_b=false;
          
        }
        if(ESPNow_pairing_action_b)
        {
          unsigned long now=millis();
          //sending package
          if(building_dap_esppairing_lcl)
          {
            uint16_t crc=0;          
            building_dap_esppairing_lcl=false;
            dap_esppairing_lcl.payloadESPNowInfo_._deviceID=dap_config_st.payLoadPedalConfig_.pedal_type;
            dap_esppairing_lcl.payLoadHeader_.payloadType=DAP_PAYLOAD_TYPE_ESPNOW_PAIRING;
            dap_esppairing_lcl.payLoadHeader_.PedalTag=dap_config_st.payLoadPedalConfig_.pedal_type;
            dap_esppairing_lcl.payLoadHeader_.version=DAP_VERSION_CONFIG;
            crc = checksumCalculator((uint8_t*)(&(dap_esppairing_lcl.payLoadHeader_)), sizeof(dap_esppairing_lcl.payLoadHeader_) + sizeof(dap_esppairing_lcl.payloadESPNowInfo_));
            dap_esppairing_lcl.payloadFooter_.checkSum=crc;
          }
          if(now-Pairing_state_last_sending>400)
          {
            Pairing_state_last_sending=now;
            ESPNow.send_message(broadcast_mac,(uint8_t *) &dap_esppairing_lcl, sizeof(dap_esppairing_lcl));
          }

          

          //timeout check
          if(now-Pairing_state_start>Pairing_timeout)
          {
            ESPNow_pairing_action_b=false;
            Serial.print("Pedal: ");
            Serial.print(dap_config_st.payLoadPedalConfig_.pedal_type);
            Serial.println(" timeout.");
            #ifdef USING_BUZZER
              Buzzer.single_beep_tone(700,100);
            #endif 
            if(UpdatePairingToEeprom)
            {
              EEPROM.put(EEPROM_offset,_ESP_pairing_reg);
              EEPROM.commit();
              UpdatePairingToEeprom=false;
              //list eeprom
              ESP_pairing_reg ESP_pairing_reg_local;
              EEPROM.get(EEPROM_offset, ESP_pairing_reg_local);
              for(int i=0;i<4;i++)
              {
                if(ESP_pairing_reg_local.Pair_status[i]==1)
                {
                  Serial.print("#");
                  Serial.print(i);
                  Serial.print("Pair: ");
                  Serial.print(ESP_pairing_reg_local.Pair_status[i]);
                  Serial.printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", ESP_pairing_reg_local.Pair_mac[i][0], ESP_pairing_reg_local.Pair_mac[i][1], ESP_pairing_reg_local.Pair_mac[i][2], ESP_pairing_reg_local.Pair_mac[i][3], ESP_pairing_reg_local.Pair_mac[i][4], ESP_pairing_reg_local.Pair_mac[i][5]);
                }
              }
              //adding peer
              
              for(int i=0; i<4;i++)
              {
                if(_ESP_pairing_reg.Pair_status[i]==1)
                {
                  if(i==0)
                  {
                    ESPNow.remove_peer(Clu_mac);
                    memcpy(&Clu_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(100);
                    ESPNow.add_peer(Clu_mac);
                    
                  }
                  if(i==1)
                  {
                    ESPNow.remove_peer(Brk_mac);
                    memcpy(&Brk_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(100);
                    ESPNow.add_peer(Brk_mac);
                  }
                  if(i==2)
                  {
                    ESPNow.remove_peer(Gas_mac);
                    memcpy(&Gas_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(100);
                    ESPNow.add_peer(Gas_mac);
                  }        
                  if(i==3)
                  {
                    ESPNow.remove_peer(esp_Host);
                    memcpy(&esp_Host,&_ESP_pairing_reg.Pair_mac[i],6);
                    delay(100);
                    ESPNow.add_peer(esp_Host);                
                  }        
                  if(dap_config_st.payLoadPedalConfig_.pedal_type==1)
                  {
                    Recv_mac=Gas_mac;
                  }
                  if(dap_config_st.payLoadPedalConfig_.pedal_type==2)
                  {
                    Recv_mac=Brk_mac;
                  }
                }
              }
            }
          }
        }
      #endif
      //joystick sync
      sendMessageToMaster(joystickNormalizedToInt32);

      if(basic_state_send_b)
      {
        ESPNow.send_message(broadcast_mac,(uint8_t *) & dap_state_basic_st,sizeof(dap_state_basic_st));
        basic_state_send_b=false;
      }
      if(extend_state_send_b)
      {
        ESPNow.send_message(broadcast_mac,(uint8_t *) & dap_state_extended_st, sizeof(dap_state_extended_st));
        extend_state_send_b=false;
      }
      if(ESPNow_config_request)
      {
        ESPNow.send_message(broadcast_mac,(uint8_t *) & dap_config_st,sizeof(dap_config_st));
        ESPNow_config_request=false;
      }
      if(Config_update_b)
      {
        Config_update_b=false;
        #ifdef USING_BUZZER
          Buzzer.single_beep_tone(700,100);
        #endif 
      }
      if(ESPNow_OTA_enable)
      {
        Serial.println("Get OTA command");
        OTA_enable_b=true;
        OTA_enable_start=true;
        ESPNow_OTA_enable=false;
      }
      if(OTA_update_action_b)
      {
        Serial.println("Get OTA command");
        OTA_enable_b=true;
        OTA_enable_start=true;
        ESPNow_OTA_enable=false;
        Serial.println("get basic wifi info");
        Serial.readBytes((char*)&_basic_wifi_info, sizeof(Basic_WIfi_info));
        #ifdef OTA_update
          if(_basic_wifi_info.device_ID==dap_config_st.payLoadPedalConfig_.pedal_type)
          {
            SSID=new char[_basic_wifi_info.SSID_Length+1];
            PASS=new char[_basic_wifi_info.PASS_Length+1];
            memcpy(SSID,_basic_wifi_info.WIFI_SSID,_basic_wifi_info.SSID_Length);
            memcpy(PASS,_basic_wifi_info.WIFI_PASS,_basic_wifi_info.PASS_Length);
            SSID[_basic_wifi_info.SSID_Length]=0;
            PASS[_basic_wifi_info.PASS_Length]=0;
            OTA_enable_b=true;
          }
          #endif

      }

      if(ESPNOW_BootIntoDownloadMode)
      {
        #ifdef ESPNow_S3
          Serial.println("Restart into Download mode");
          delay(1000);
          REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
          ESP.restart();
        #else
          Serial.println("Command not supported");
          delay(1000);
        #endif
        ESPNOW_BootIntoDownloadMode = false;
      }
      //rudder sync
      if(dap_calculationVariables_st.Rudder_status)
      {              
        dap_calculationVariables_st.current_pedal_position_ratio=((float)(dap_calculationVariables_st.current_pedal_position-dap_calculationVariables_st.stepperPosMin_default))/((float)dap_calculationVariables_st.stepperPosRange_default);
        _ESPNow_Send.pedal_position_ratio=dap_calculationVariables_st.current_pedal_position_ratio;
        _ESPNow_Send.pedal_position=dap_calculationVariables_st.current_pedal_position;
        //ESPNow_send=dap_calculationVariables_st.current_pedal_position; 
        esp_err_t result =ESPNow.send_message(Recv_mac,(uint8_t *) &_ESPNow_Send,sizeof(_ESPNow_Send));                
        //if (result == ESP_OK) 
        //{
        //  Serial.println("Error sending the data");
        //}                
        if(ESPNow_update)
        {
          //dap_calculationVariables_st.sync_pedal_position=ESPNow_recieve;
          dap_calculationVariables_st.sync_pedal_position=_ESPNow_Recv.pedal_position;
          dap_calculationVariables_st.Sync_pedal_position_ratio=_ESPNow_Recv.pedal_position_ratio;
          ESPNow_update=false;
        }                
      }
          
    }

    #ifdef ESPNow_debug_rudder
      if(print_count>1000)
      {
        if(dap_calculationVariables_st.Rudder_status)
        {
          Serial.print("Pedal:");
          Serial.print(dap_config_st.payLoadPedalConfig_.pedal_type);
          Serial.print(", Send %: ");
          Serial.print(_ESPNow_Send.pedal_position_ratio);
          Serial.print(", Recieve %:");
          Serial.print(_ESPNow_Recv.pedal_position_ratio);
          Serial.print(", Send Position: ");
          Serial.print(dap_calculationVariables_st.current_pedal_position);
          Serial.print(", % in cal: ");
          Serial.print(dap_calculationVariables_st.current_pedal_position_ratio); 
          Serial.print(", min cal: ");
          Serial.print(dap_calculationVariables_st.stepperPosMin_default); 
          Serial.print(", max cal: ");
          Serial.print(dap_calculationVariables_st.stepperPosMax_default);
          Serial.print(", range in cal: ");
          Serial.println(dap_calculationVariables_st.stepperPosRange_default); 
        }

        //Debug_rudder_last=now_rudder;
        //Serial.println(dap_calculationVariables_st.current_pedal_position);                  
            
        print_count=0;
      }
      else
      {
        print_count++;
            
      } 
          
               
    #endif



    
      #ifdef PRINT_TASK_FREE_STACKSIZE_IN_WORDS
        if( espNowTask_stackSizeIdx_u32 == 1000)
        {
          UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
          Serial.print("StackSize (ESP-Now): ");
          Serial.println(stackHighWaterMark);
          espNowTask_stackSizeIdx_u32 = 0;
        }
        espNowTask_stackSizeIdx_u32++;
      #endif
  }

}
#endif


