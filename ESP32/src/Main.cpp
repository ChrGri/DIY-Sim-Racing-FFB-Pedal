
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779


#include "esp_timer.h" // Include the header for the high-resolution timer



#define ESTIMATE_LOADCELL_VARIANCE
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER 1
#define DEBUG_INFO_0_STEPPER_POS 2
// #define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS 8
#define DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS 16
#define DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT 32
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT 64
#define DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS 128




//#define PI 3.14159267
#define BAUD3M 3000000
#define DEFAULTBAUD 921600
#include "Arduino.h"
#include "Main.h"
#include "Version_Board.h"
#include "PedalInfoBuilder.h"
#ifdef Using_analog_output_ESP32_S3
#include <Wire.h>
#include <Adafruit_MCP4725.h>
  TwoWire MCP4725_I2C= TwoWire(1);
  //MCP4725 MCP(0x60, &MCP4725_I2C);
  Adafruit_MCP4725 dac;
  int current_use_mcp_index;
  bool MCP_status =false;
#endif


#include "FastTrig.h"



//#define ALLOW_SYSTEM_IDENTIFICATION




/**********************************************************************************************/
/*                                                                                            */
/*                         function declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters();
void pedalUpdateTask( void * pvParameters );
void loadcellReadingTask( void * pvParameters );
void serialCommunicationTask( void * pvParameters );
void joystickOutputTask( void * pvParameters );
void OTATask( void * pvParameters );
void ESPNOW_SyncTask( void * pvParameters);
void miscTask( void * pvParameters);
#define INCLUDE_vTaskDelete 1
// https://www.tutorialspoint.com/cyclic-redundancy-check-crc-in-arduino
inline uint16_t checksumCalculator(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index + 1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}




bool systemIdentificationMode_b = false;
bool previewConfigGet_b=false;
unsigned long saveToEEPRomDuration=0;



bool splineDebug_b = false;



#include <EEPROM.h>
#define EEPROM_offset 15


#include "ABSOscillation.h"
#include "Rudder.h"
ABSOscillation absOscillation;
RPMOscillation _RPMOscillation;
BitePointOscillation _BitePointOscillation;
G_force_effect _G_force_effect;
WSOscillation _WSOscillation;
Road_impact_effect _Road_impact_effect;
Custom_vibration CV1;
Custom_vibration CV2;
Rudder _rudder;
helicoptersRudder helicopterRudder_;
Rudder_G_Force _rudder_g_force;
MovingAverageFilter averagefilter_joystick(40);
#define ABS_OSCILLATION



#include "DiyActivePedal_types.h"

DAP_config_class global_dap_config_class;
DAP_calculationVariables_st dap_calculationVariables_st;
DAP_state_basic_st dap_state_basic_st;
DAP_state_extended_st dap_state_extended_st;
DAP_ESPPairing_st dap_esppairing_st;//saving
DAP_ESPPairing_st dap_esppairing_lcl;//sending




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


bool configUpdateAvailable = false;                              // semaphore protected data


static SemaphoreHandle_t semaphore_updateJoystick=NULL;
int32_t joystickNormalizedToInt32 = 0;                           // semaphore protected data
static SemaphoreHandle_t semaphore_updatePedalStates=NULL;
static SemaphoreHandle_t semaphore_updateLoadcellReading = NULL;

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
KalmanFilter* kalman_joystick = NULL;

#include "SignalFilter_2nd_order.h"
KalmanFilter_2nd_order* kalman_2nd_order = NULL;




/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell definitions                                               */
/*                                                                                            */
/**********************************************************************************************/

#ifdef USES_ADS1220
  /*  Uses ADS1220 */
  #include "LoadCell_ads1220.h"
  LoadCell_ADS1220* loadcell = NULL;

#else
  /*  Uses ADS1256 */
  #include "LoadCell.h"
  LoadCell_ADS1256* loadcell = NULL;
#endif


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

#if !defined(OTA_update) && !defined(OTA_update_ESP32)
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
#include <cstring>


/**********************************************************************************************/
/*                                                                                            */
/*                         profiler setup                                                     */
/*                                                                                            */
/**********************************************************************************************/
// #include "FunctionProfiler.h"
#include "ProfilerHooks.h"
#include "RuntimeStats.h"

TaskRuntimeProfiler profiler;



/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell reading                                                   */
/*                                                                                            */
/**********************************************************************************************/
float loadcellReading_global_fl32 = 0.0f;
void IRAM_ATTR loadcellReadingTask( void * pvParameters )
{
  for(;;){

    if (loadcell != NULL)
    {
      // no need for delay, since getReadingKg will block until DRDY edge down is detected
      float loadcellReading = loadcell->getReadingKg();
      
      if(semaphore_updateLoadcellReading != NULL)
      {
        if(xSemaphoreTake(semaphore_updateLoadcellReading, (TickType_t)1)==pdTRUE) {
          loadcellReading_global_fl32 = loadcellReading;
          xSemaphoreGive(semaphore_updateLoadcellReading);
        }
      }
      else
      {
        semaphore_updateLoadcellReading = xSemaphoreCreateMutex();
      }
    }
  }
}


/**********************************************************************************************/
/*                                                                                            */
/*                         pedal update                                                       */
/*                                                                                            */
/**********************************************************************************************/
#define REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US (int64_t)300

/**********************************************************************************************/
/*                                                                                            */
/*                         joystick update                                                    */
/*                                                                                            */
/**********************************************************************************************/
#define REPETITION_INTERVAL_JOYSTICKOUTPUT_TASK_IN_US (int64_t)10000

/**********************************************************************************************/
/*                                                                                            */
/*                         serial communication                                               */
/*                                                                                            */
/**********************************************************************************************/
#define REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US (int64_t)10000
#define REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_FAST_IN_US (int64_t)200


#define REPETITION_INTERVAL_ESPNOW_TASK_IN_US (int64_t)2000
#define REPETITION_INTERVAL_OTA_TASK_IN_US (int64_t)3000



// === Scheduler config ===
#define BASE_TICK_US 100   // base tick in microseconds
#define MAX_TASKS    10     // maximum tasks in scheduler

// Task entry struct
typedef struct {
  TaskHandle_t handle;
  const char *name;
  TaskFunction_t fn;
  uint16_t intervalTicks;
  uint16_t counter;
  uint32_t lastKick;   // last time task ran (micros)
  UBaseType_t priority;
  BaseType_t core;
} SchedTask;

// Task table
SchedTask tasks[MAX_TASKS];
uint8_t taskCount = 0;

// Timer handle
hw_timer_t *timer0 = NULL;

// === Scheduler ISR ===
void IRAM_ATTR onTimer(void* arg) {
  BaseType_t xHigherPriorityWoken = pdFALSE;

  for (int i = 0; i < taskCount; i++) {
    tasks[i].counter++;
    if (tasks[i].counter >= tasks[i].intervalTicks) {
      tasks[i].counter = 0;
      if(NULL != tasks[i].handle)
      {
        vTaskNotifyGiveFromISR(tasks[i].handle, &xHigherPriorityWoken);
      }
      
    }
  }

  // Yield if a higher-priority task was woken.
  if (xHigherPriorityWoken) {
    portYIELD_FROM_ISR();
  }
}

// === Scheduler API ===
void addScheduledTask(TaskFunction_t fn, const char *name, uint16_t intervalUs,
                      UBaseType_t priority, BaseType_t core, uint32_t stackSize = 2048u) {
  if (taskCount >= MAX_TASKS) return;  // limit reached

  uint16_t intervalTicks = intervalUs / BASE_TICK_US;
  if (intervalTicks == 0) intervalTicks = 1;  // minimum 1 tick

  // Create task
  xTaskCreatePinnedToCore(fn, name, stackSize, NULL, priority,
                          &tasks[taskCount].handle, core);

  tasks[taskCount].intervalTicks = intervalTicks;
  tasks[taskCount].counter = 0;
  taskCount++;
}




TaskHandle_t handle_pedalUpdateTask = NULL;
TaskHandle_t handle_joystickOutput = NULL;
TaskHandle_t handle_loadcellReadingTask = NULL;
TaskHandle_t handle_serialCommunication = NULL; 
TaskHandle_t handle_miscTask = NULL; 
TaskHandle_t handle_otaTask = NULL;
TaskHandle_t handle_espnowTask = NULL;

#define COUNTER_SIZE 4u
uint16_t tickCount_au16[COUNTER_SIZE] = {0};


static uint16_t timerTicks_espNowTask_u16 = REPETITION_INTERVAL_ESPNOW_TASK_IN_US / BASE_TICK_US;




/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
#include "driver/uart.h"


// Queue to handle UART events
static QueueHandle_t uart_queue;

#define UART_RX_BUF_SIZE   1024


/**
 * @brief Task to handle UART events.
 *
 * This task waits for a UART_PATTERN_DET event, which is triggered
 * by the hardware when the PACKET_EOF_CHAR is detected.
 */
static void uart_event_task(void *pvParameters) {
  uart_event_t event;
  uint8_t* dtmp = (uint8_t*) malloc(UART_RX_BUF_SIZE);

  for (;;) {
    delay(100);
    // Wait for the next event from the UART driver
    if (xQueueReceive(uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
      
      Serial.println("pattern detected");

      switch (event.type) {
        
        // --- MODIFIED: The primary event we now listen for ---
        // Event triggered when the hardware detects our pattern (EOF character)
        case UART_PATTERN_DET:
          {
            // Get the size of the data available in the buffer
            size_t buffered_size;
            uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
            
            // Read the complete packet from the UART buffer
            int pos = uart_read_bytes(UART_NUM_0, dtmp, buffered_size, pdMS_TO_TICKS(100));
            
            // Ensure we have at least two bytes and they match the EOF pattern.
            if (pos >= 2 && dtmp[pos - 2] == EOF_BYTE_0 && dtmp[pos - 1] == EOF_BYTE_1) {
              // Null-terminate the received data to treat it as a string
              dtmp[pos] = '\0';

              // Print a header to the main Serial Monitor for clarity
              Serial.printf("\n[UART PATTERN DETECTED - COMPLETE PACKET (%d bytes)]\n", pos);

              // Write the received packet to the main Serial Monitor
              Serial.printf("Packet Data: \"%s\"\n", (char*)dtmp);
              
              // --- YOUR PACKET PROCESSING LOGIC GOES HERE ---
              // You can now safely parse the complete packet in `dtmp`
            }
          }
          break;

        // Event for a FIFO buffer overflow
        case UART_FIFO_OVF:
          Serial.println("Hardware FIFO overflow");
          uart_flush_input(UART_NUM_0);
          xQueueReset(uart_queue);
          break;

        // Event for a ring buffer full condition
        case UART_BUFFER_FULL:
          Serial.println("Ring buffer full");
          uart_flush_input(UART_NUM_0);
          xQueueReset(uart_queue);
          break;

        // Other unhandled events (like UART_DATA) are now ignored but
        // can be handled here if needed for timeouts or error recovery.
        default:
          // Flushing the buffer on unexpected events can help prevent lock-ups
          uart_flush_input(UART_NUM_0);
          break;
      }
    }
  }
  // Free the temporary buffer and delete the task if the loop ever exits
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}



void setup()
{

  

  

  DAP_config_st dap_config_st_local;

// setup brake resistor pin
#ifdef BRAKE_RESISTOR_PIN
  pinMode(BRAKE_RESISTOR_PIN, OUTPUT);  // Set GPIO13 as an output
  digitalWrite(BRAKE_RESISTOR_PIN, LOW);  // Turn the LED on
#endif

#ifdef EMERGENCY_PIN
  pinMode(EMERGENCY_PIN,INPUT_PULLUP);
#endif

#ifdef ANGLE_SENSOR_GPIO

  pinMode(ANGLE_SENSOR_GPIO, INPUT);
  pinMode(ANGLE_SENSOR_GPIO_2, INPUT);
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
    Buzzer.initialized(BuzzerPin,1);
    Buzzer.single_beep_tone(770,100);
  #endif

  #if PCB_VERSION == 7
    Serial.setTxTimeoutMs(0);
    Serial.begin(DEFAULTBAUD);
  #else
    #ifdef BAUDRATE3M
      Serial.begin(BAUD3M, SERIAL_8N1);
      Serial.setTxBufferSize(256);
    #else
      Serial.begin(DEFAULTBAUD, SERIAL_8N1);
    #endif
    Serial.setTimeout(5);
  #endif
  parse_version(DAP_FIRMWARE_VERSION, &versionMajor, &versionMinor, &versionPatch);
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
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

  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(2048);
  global_dap_config_class.loadConfigFromEprom();
  dap_config_st_local = global_dap_config_class.getConfig();


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
    global_dap_config_class.setConfig(dap_config_st_local);
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
    //if the config check all failed, reinitialzie _config_st
    Serial.println("initialized config");
    global_dap_config_class.initializedConfig();
    dap_config_st_local=global_dap_config_class.getConfig();
  }


  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);

  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x5f,0x5f,0x00);//yellow
      pixels.show(); 
      //delay(3000);
  #endif


  bool invMotorDir = dap_config_st_local.payLoadPedalConfig_.invertMotorDirection_u8 > 0;
  stepper = new StepperWithLimits(stepPinStepper, dirPinStepper, invMotorDir, dap_calculationVariables_st.stepsPerMotorRevolution); 

  motorRevolutionsPerSteps_fl32 = 1.0f / ( (float)dap_calculationVariables_st.stepsPerMotorRevolution );
  // Serial.printf("Steps per motor revolution: %d\n", dap_calculationVariables_st.stepsPerMotorRevolution);

  #ifdef USES_ADS1220
    /*  Uses ADS1220 */
    loadcell = new LoadCell_ADS1220();

  #else
    /*  Uses ADS1256 */
    loadcell = new LoadCell_ADS1256();
  #endif

  

  loadcell->setLoadcellRating(dap_config_st_local.payLoadPedalConfig_.loadcell_rating);

  loadcell->estimateBiasAndVariance();       // automatically identify sensor noise for KF parameterization

	// find the min & max endstops
	Serial.println("Start homing");
	stepper->findMinMaxSensorless(dap_config_st_local);

 
  Serial.print("Min Position is "); Serial.println(stepper->getLimitMin());
  Serial.print("Max Position is "); Serial.println(stepper->getLimitMax());


  // setup Kalman filters
  // Serial.print("Given loadcell variance: ");
  // Serial.println(loadcell->getVarianceEstimate(), 5);
  kalman = new KalmanFilter(loadcell->getVarianceEstimate());
  kalman_joystick =new KalmanFilter(0.1f);
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
  dap_config_st_local = global_dap_config_class.getConfig();


  // setup multi tasking
  semaphore_updateJoystick = xSemaphoreCreateMutex();
  semaphore_updatePedalStates = xSemaphoreCreateMutex();
  semaphore_updateLoadcellReading = xSemaphoreCreateMutex();

  delay(10);


  if(semaphore_updateJoystick==NULL)
  {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }


  disableCore0WDT();

  Serial.println("Starting other tasks");

  // Register tasks
  addScheduledTask(pedalUpdateTask, "pedalUpdateTask", REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US, 1, CORE_ID_PEDAL_UPDATE_TASK, 7000);
  addScheduledTask(serialCommunicationTask, "serialCommunicationTask", REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US, 1, CORE_ID_SERIAL_COMMUNICATION_TASK, 5000);
  addScheduledTask(joystickOutputTask, "joystickOutputTask", REPETITION_INTERVAL_JOYSTICKOUTPUT_TASK_IN_US, 1, CORE_ID_JOYSTICK_TASK, 5000);

  // === Replace hw_timer with esp_timer ===
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &onTimer,
    .arg = NULL,
    .name = "sched_timer"
  };

  esp_timer_handle_t periodic_timer;
  esp_timer_create(&periodic_timer_args, &periodic_timer);

  // Start periodic timer at BASE_TICK_US interval
  esp_timer_start_periodic(periodic_timer, BASE_TICK_US);

  // the loadcell task does not need a dedicated timer, since it blocks by DRDY ready ISR
  xTaskCreatePinnedToCore(
                    loadcellReadingTask,   /* Task function. */
                    "loadcellReadingTask",     /* name of task. */
                    3000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &handle_loadcellReadingTask,      /* Task handle to keep track of created task */
                    CORE_ID_LOADCELLREADING_TASK);          /* pin task to core 1 */  

  xTaskCreatePinnedToCore(
                    miscTask,   
                    "miscTask", 
                    2000,  
                    NULL,      
                    1,         
                    &handle_miscTask,    
                    CORE_ID_MISC_TASK);     


                    
  // #define SERIAL_PATTERN_DETECTOR
  #ifdef SERIAL_PATTERN_DETECTOR
  // This prevents the "UART driver already installed" error.
  uart_driver_delete(UART_NUM_0);
  
  // --- MODIFIED: Install driver over the existing UART0 ---
  // Note: This will reconfigure the port used by the Arduino `Serial` object.
  esp_err_t err = uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE * 2, 0, 20, &uart_queue, 0);
  if (err != ESP_OK) {
    Serial.printf("Failed to install UART driver: %d\n", err);
    return;
  }

  // Configure UART parameters
  // SERIAL_8N1

  uart_config_t uart_config = {
      .baud_rate = BAUD3M,
      .data_bits = UART_DATA_8_BITS,
      .parity    = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_XTAL,
  };
  
  // Apply the UART configuration
  uart_param_config(UART_NUM_0, &uart_config);

  // --- NEW: Enable UART pattern detection ---
  uart_enable_pattern_det_baud_intr(UART_NUM_0, EOF_BYTE_1, 1, 9, 0, 0);

  // Create the task that will handle UART events
  xTaskCreate(
      uart_event_task,    // Task function
      "uart_event_task",  // Name of the task
      4096,               // Stack size
      NULL,               // Task input parameter
      12,                 // Priority of the task
      NULL                // Task handle
  );

  #endif
  



  //Serial.begin(115200);
  #if defined(OTA_update)  || defined(OTA_update_ESP32)
  
    switch(dap_config_st_local.payLoadPedalConfig_.pedal_type)
    {
      case 0:
        APhost=new char[strlen("FFBPedalClutch") + 1];
        strcpy(APhost, "FFBPedalClutch");
        //APhost="FFBPedalClutch";
        break;
      case 1:
        APhost=new char[strlen("FFBPedalBrake") + 1];
        strcpy(APhost, "FFBPedalBrake");
        //APhost="FFBPedalBrake";
        break;
      case 2:
        APhost=new char[strlen("FFBPedalGas") + 1];
        strcpy(APhost, "FFBPedalGas");
        //APhost="FFBPedalGas";
        break;
      default:
        APhost=new char[strlen("FFBPedal") + 1];
        strcpy(APhost, "FFBPedal");
        //APhost="FFBPedal";
        break;        

    }   
    //Serial.begin(115200);
    // xTaskCreatePinnedToCore(
    //                 OTATask,   
    //                 "OTATask", 
    //                 16000,  
    //                 //STACK_SIZE_FOR_TASK_2,    
    //                 NULL,      
    //                 1,         
    //                 &handle_otaTask,    
    //                 CORE_ID_OTA_TASK); 
                    
    addScheduledTask(OTATask, "OTATask", REPETITION_INTERVAL_OTA_TASK_IN_US, 1, CORE_ID_OTA_TASK, 16000);

    delay(200);
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
  if(dap_config_st_local.payLoadPedalConfig_.pedal_type!=4)
  {
    Serial.print("Pedal Assignment: ");
    Serial.println(dap_config_st_local.payLoadPedalConfig_.pedal_type);
  }
  else
  {
    #ifdef PEDAL_HARDWARE_ASSIGNMENT
      Serial.println("Pedal Role Assignment:4, reading from CFG pins....");
    #else
      Serial.println("Pedal Role Assignment:4, Role assignment Error, Please send the config in to finish role assignment.");
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
          DAP_config_st tmp = global_dap_config_class.getConfig();
          tmp.payLoadPedalConfig_.pedal_type = Pedal_assignment;
          dap_config_st_local.payLoadPedalConfig_.pedal_type = Pedal_assignment;
          global_dap_config_class.setConfig(tmp);

        }
        else
        {
          Serial.println("Asssignment error, defective pin connection, pelase connect USB and send a config to finish assignment");
        }
    }
   
  #endif

  //enable ESP-NOW
  #ifdef ESPNOW_Enable
  dap_calculationVariables_st.rudder_brake_status=false;
  if(dap_config_st_local.payLoadPedalConfig_.pedal_type==0||dap_config_st_local.payLoadPedalConfig_.pedal_type==1||dap_config_st_local.payLoadPedalConfig_.pedal_type==2)
  {
    Serial.println("Starting ESP now tasks");
    ESPNow_initialize();
    Serial.println("ESPNOW initialized, add task in");
    // xTaskCreatePinnedToCore(
    //                     ESPNOW_SyncTask,   
    //                     "ESPNOW_update_Task", 
    //                     10000,  
    //                     //STACK_SIZE_FOR_TASK_2,    
    //                     NULL,      
    //                     1,         
    //                     &handle_espnowTask,    
    //                     CORE_ID_ESPNOW_TASK);  
                        
    addScheduledTask(ESPNOW_SyncTask, "ESPNOW_update_Task", REPETITION_INTERVAL_ESPNOW_TASK_IN_US, 1, CORE_ID_ESPNOW_TASK, 10000);
    Serial.println("ESPNOW task added");
    delay(500);
  }
  else
  {
    Serial.println("ESPNOW task did not started due to Assignment error, please usb connect to Simhub and finish Assignment.");
  }
  #endif
  Serial.println("Setup Controller");
  #ifdef CONTROLLER_SPECIFIC_VIDPID
    SetupController_USB(dap_config_st_local.payLoadPedalConfig_.pedal_type);
    delay(500);
  #endif  
  #ifndef CONTROLLER_SPECIFIC_VIDPID
  // init controller
  #if defined(BLUETOOTH_GAMEPAD) || defined(USB_JOYSTICK)
  SetupController();
  #endif
  //delay(3000);
  #endif




  



  Serial.println("Setup end");
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0xff,0x00);//Green
      pixels.show(); 
      //delay(3000);
  #endif

  #ifdef USING_BUZZER
    if(dap_config_st_local.payLoadPedalConfig_.pedal_type==0)
    {
      delay(500);
      Buzzer.single_beep_ledc_fade(NOTE_D4,3072,1);
      //Buzzer.single_beep_ledc_fade(NOTE_A4,1536,0.5);
    }
    if(dap_config_st_local.payLoadPedalConfig_.pedal_type==1)
    {
      Buzzer.single_beep_ledc_fade(NOTE_A4,3072,1);
    }    
    if(dap_config_st_local.payLoadPedalConfig_.pedal_type==2)
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

  DAP_config_st dap_config_st_local = global_dap_config_class.getConfig();

  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);
  dap_calculationVariables_st.updateEndstops(stepper->getLimitMin(), stepper->getLimitMax());
  stepper->updatePedalMinMaxPos(dap_config_st_local.payLoadPedalConfig_.pedalStartPosition, dap_config_st_local.payLoadPedalConfig_.pedalEndPosition);
  dap_calculationVariables_st.updateStiffness();

  // tune the PID settings
  tunePidValues(dap_config_st_local);
}



/**********************************************************************************************/
/*                                                                                            */
/*                         Main function                                                      */
/*                                                                                            */
/**********************************************************************************************/
void loop() {
  //vTaskDelete(NULL);  // Kill the Arduino loop task

  delay(5000);   // wait some time
  profiler.report();  // print CPU usage per task
}




/**********************************************************************************************/
/*                                                                                            */
/*                         pedal update task                                                  */
/*                                                                                            */
/**********************************************************************************************/
void IRAM_ATTR pedalUpdateTask( void * pvParameters )
{

  static DRAM_ATTR DAP_state_extended_st dap_state_extended_st_lcl_pedalUpdateTask;
  FunctionProfiler profiler_pedalUpdateTask(nullptr, "pedalUpdateTask");
  registerProfiler(&profiler_pedalUpdateTask);   // hook profiler into task
  profiler_pedalUpdateTask.setName("PedalUpdate");
  float loadcellReading = 0.0f;
  float filteredReading_exp_filter = 0;
  unsigned long servoActionLast = millis();
  bool firstReadConfig=true;
  unsigned long debugMessageLast=0;

  uint32_t pos_printCount = 0;
  uint32_t controlTask_stackSizeIdx_u32 = 0;
  float previousLoadcellReadingInKg_fl32 = 0.0f;
 
  for(;;){

    // wait for the timer to fire
    // This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
    // if(handle_pedalUpdateTask != NULL)
    {
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

        // copy global struct to local for faster and safe executiion
        DAP_config_st dap_config_pedalUpdateTask_st = global_dap_config_class.getConfig();

        // activate profiler depending on pedal config
        if (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_pedalUpdateTask.activate( true );
        }
        else
        {
          profiler_pedalUpdateTask.activate( false );
        }


        // start profiler 0, overall function
        profiler_pedalUpdateTask.start(0);
        
        

        // system identification mode
        #ifdef ALLOW_SYSTEM_IDENTIFICATION
          if (systemIdentificationMode_b == true)
          {
            measureStepResponse(stepper, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, loadcell);
            systemIdentificationMode_b = false;
          }
        #endif
      

        // if a config update was received over serial, update the variables required for further computation
        if (configUpdateAvailable == true)
        {
            bool configWasUpdated_b = false;
            // Take the semaphore and just update the config file, then release the semaphore
            
            Serial.println("Updating pedal config");
            configUpdateAvailable = false;

            // update the calc params
            Serial.println("Updating the calc params");
            configWasUpdated_b = false;
            //Serial.print("save to eeprom tag:");
            //Serial.println(dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom);
            if(firstReadConfig)
            {
              firstReadConfig=false;
            }
            else
            {
              previewConfigGet_b = true;
              saveToEEPRomDuration = millis();
            }
            
            if (true == dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom)
            {
              dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
              uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_pedalUpdateTask_st.payLoadHeader_)), sizeof(dap_config_pedalUpdateTask_st.payLoadHeader_) + sizeof(dap_config_pedalUpdateTask_st.payLoadPedalConfig_));
              dap_config_pedalUpdateTask_st.payloadFooter_.checkSum = crc;

              global_dap_config_class.storeConfigToEprom();
              previewConfigGet_b = false;
              saveToEEPRomDuration = 0;
            }
            
            updatePedalCalcParameters(); // update the calc parameters
            moveSlowlyToPosition_b = true;
        }
        
        //#define RECALIBRATE_POSITION
        #ifdef RECALIBRATE_POSITION
          stepper->checkLimitsAndResetIfNecessary();
        #endif


        // start profiler 1, effects
        profiler_pedalUpdateTask.start(1);


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
          if(dap_calculationVariables_st.Rudder_status) 
          {
            _rudder.offset_calculate(&dap_calculationVariables_st);
            dap_calculationVariables_st.update_stepperMinpos(_rudder.offset_filter);
            _rudder_g_force.offset_calculate(&dap_calculationVariables_st);
            dap_calculationVariables_st.update_stepperMaxpos(_rudder_g_force.offset_filter);
          }
          if(dap_calculationVariables_st.helicopterRudderStatus) 
          {
            helicopterRudder_.offset_calculate(&dap_calculationVariables_st);
            dap_calculationVariables_st.update_stepperMinpos(helicopterRudder_.offset_filter);
          }
          #ifdef ESPNow_debug_rudder
            if(millis()-debugMessageLast>500)
            {
              debugMessageLast=millis();
              Serial.print("Center offset:");
              Serial.println(_rudder.offset_filter);
              Serial.print("min default:");
              Serial.println(dap_calculationVariables_st.stepperPosMin_default);
            }
          #endif

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
      
        // end profiler 1, effects
        profiler_pedalUpdateTask.end(1);

        // start profiler 2, loadcell reading
        profiler_pedalUpdateTask.start(2);

        // Get the loadcell reading
        if(semaphore_updateLoadcellReading != NULL)
        {
          if(xSemaphoreTake(semaphore_updateLoadcellReading, (TickType_t)0)==pdTRUE) {
            loadcellReading = loadcellReading_global_fl32;
            xSemaphoreGive(semaphore_updateLoadcellReading);
          }
        }




        // end profiler 2, loadcell reading
        profiler_pedalUpdateTask.end(2);

        // detect loadcell outlier
        float loadcellDifferenceToLastCycle_fl32 = loadcellReading - previousLoadcellReadingInKg_fl32;
        previousLoadcellReadingInKg_fl32 = loadcellReading;
        if(!dap_calculationVariables_st.Rudder_status && !dap_calculationVariables_st.helicopterRudderStatus)
        {
          //make the force reading skip only in pedal mode
          if (fabsf(loadcellDifferenceToLastCycle_fl32) > 5.0f)
          {
            dap_calculationVariables_st.StepperPos_setback();
            dap_calculationVariables_st.reset_maxforce();
            dap_calculationVariables_st.dynamic_update();
            dap_calculationVariables_st.updateStiffness();
            // reject update when loadcell reading likely outlier
            continue;
          }
        }

        



        uint16_t angleReading_ui16 = 0;
    #ifdef ANGLE_SENSOR_GPIO
          angleReading_ui16 = analogRead(ANGLE_SENSOR_GPIO);
          // if (pos_printCount >= 100)
          // {
          //   Serial.printf("Ang.: %f\n", angleReading_ui16);
          //   pos_printCount = 0;
          // }
          // pos_printCount++;
    #endif

        // Get the angle measurement reading
        // float angleReading = loadcell->getAngleMeasurement();

        
        // if (pos_printCount >= 100)
        // {
        //   Serial.printf("Ang.: %f\n", angleReading);
        //   pos_printCount = 0;
        // }
        // pos_printCount++;
      

        // start profiler 3, loadcell reading conversion
        profiler_pedalUpdateTask.start(3);

        // Invert the loadcell reading digitally if desired
        if (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.invertLoadcellReading_u8 == 1)
        {
          loadcellReading *= -1.0f;
        }

        // Convert loadcell reading to pedal force
        float sledPosition = sledPositionInMM(stepper, &dap_config_pedalUpdateTask_st, motorRevolutionsPerSteps_fl32);
        float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, &dap_config_pedalUpdateTask_st);
        float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
        float d_phi_d_x = convertToPedalForceGain(sledPosition, &dap_config_pedalUpdateTask_st);

        // compute gain for horizontal foot model
        float b = (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_b;
        float d = (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_d;
        float d_x_hor_d_phi = -(float)(b+d) * isin(pedalInclineAngleInDeg_fl32);
        d_x_hor_d_phi *= DEG_TO_RAD; // inner derivative

        // start profiler 3, loadcell reading conversion
        profiler_pedalUpdateTask.end(3);

        // start profiler 4, loadcell reading filtering
        profiler_pedalUpdateTask.start(4);
        
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
            filteredReading = pedalForce_fl32;
            break;
        }
        //write filter reading into calculation_st
        dap_calculationVariables_st.currentForceReading=filteredReading;


        // end profiler 4, loadcell reading filtering
        profiler_pedalUpdateTask.end(4);


       
        float FilterReadingJoystick=0.0f;
        if(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_Joystick_u8==1)
        {
          FilterReadingJoystick=kalman_joystick->filteredValue(filteredReading,0.0f,dap_config_pedalUpdateTask_st.payLoadPedalConfig_.kf_modelNoise_joystick);

        }
        else
        {
          FilterReadingJoystick=filteredReading;

        }


        //if filtered reading > min force, mark the servo was in aciton
        if(filteredReading > dap_config_pedalUpdateTask_st.payLoadPedalConfig_.preloadForce)
        {
          servoActionLast = millis();
        }

        // wakeup process
        if ((filteredReading > STEPPER_WAKEUP_FORCE) && (stepper->servoStatus == SERVO_IDLE_NOT_CONNECTED))
        {
          #ifdef USING_BUZZER
            Buzzer.single_beep_tone(770, 100);
            delay(300);
            Buzzer.single_beep_tone(770, 100);
          #endif
          Serial.println("Wake up servo, restart esp.");
          delay(1000);
          ESP.restart();
        }

        // pedal not in action, disable pedal power
        uint32_t pedalIdleTimout = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.servoIdleTimeout * 60 * 1000; // timeout in ms
        if ((stepper->servoStatus == SERVO_CONNECTED) && ((millis() - servoActionLast) > pedalIdleTimout) && (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.servoIdleTimeout != 0))
        {
          stepper->servoIdleAction();
          stepper->servoStatus = SERVO_IDLE_NOT_CONNECTED;
          #ifdef USING_BUZZER
            Buzzer.single_beep_tone(770, 100);
          #endif
          delay(300);
          #ifdef USING_LED
            pixels.setPixelColor(0, 0xff, 0x00, 0x00); // show red
            pixels.show();
          #endif
          #ifdef USING_BUZZER
            Buzzer.single_beep_tone(770, 100);
          #endif
          Serial.println("Servo idle timeout reached. To restart pedal, please apply pressure.");
        }
        //emergency button

        #ifdef EMERGENCY_PIN
          if ((stepper->servoStatus == SERVO_CONNECTED) && (stepper->servoStatus != SERVO_FORCE_STOP) && (digitalRead(EMERGENCY_PIN) == LOW))
          {
            stepper->servoIdleAction();
            stepper->servoStatus = SERVO_FORCE_STOP;
            #ifdef USING_BUZZER
              Buzzer.single_beep_tone(770, 100);
            #endif
            delay(300);
            #ifdef USING_LED
              pixels.setPixelColor(0, 0xff, 0x00, 0x00); // show red
              pixels.show();
            #endif
            #ifdef USING_BUZZER
              Buzzer.single_beep_tone(770, 100);
            #endif
            Serial.println("Servo force Stoped.");
          }
        #endif
        //float FilterReadingJoystick=averagefilter_joystick.process(filteredReading);


        // start profiler 4, movement strategy
        profiler_pedalUpdateTask.start(5);


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

        // end profiler 4, movement strategy
        profiler_pedalUpdateTask.end(5);

        // start profiler 6, ...
        profiler_pedalUpdateTask.start(6);

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
        dap_calculationVariables_st.current_pedal_position_ratio=((float)(dap_calculationVariables_st.current_pedal_position-dap_calculationVariables_st.stepperPosMin_default))/((float)dap_calculationVariables_st.stepperPosRange_default);
        //Rudder initialzing and de initializing
        #ifdef ESPNOW_Enable
          if(dap_calculationVariables_st.Rudder_status)
          {
            if(Rudder_initializing)
            {
              moveSlowlyToPosition_b=true;
              //Serial.println("moving to center");
            }
            if(Rudder_initializing && (Rudder_real_poisiton<52 && Rudder_real_poisiton>48))
            {
              if(Rudder_initialized_time==0)
              {
                Rudder_initialized_time=millis();
              }
              else
              {
                unsigned long Rudder_initialzing_time_Now = millis();
                //wait 3s for the initializing
                //Serial.print("Rudder initializing...");
                //Serial.println(Rudder_initialzing_time_Now-Rudder_initialized_time);
                if( (Rudder_initialzing_time_Now-Rudder_initialized_time)> Rudder_timeout )
                {
                  Rudder_initializing=false;
                  moveSlowlyToPosition_b=false;
                  Serial.println("Rudder initialized");
                  dap_calculationVariables_st.isRudderInitialized=true;
                  Rudder_initialized_time=0;
                  #ifdef USING_BUZZER
                    Buzzer.play_melody_tone(melody_Airship_theme, sizeof(melody_Airship_theme)/sizeof(melody_Airship_theme[0]),melody_Airship_theme_duration);
                  #endif
                }
              }
              

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
            Serial.println("Rudder deinitialized");
            dap_calculationVariables_st.isRudderInitialized=false;
          }
          //helicopter rudder initialzied
          if(dap_calculationVariables_st.helicopterRudderStatus)
          {
            if(HeliRudder_initializing)
            {
              moveSlowlyToPosition_b=true;
              //Serial.println("moving to center");
            }
            if(HeliRudder_initializing && (Rudder_real_poisiton<52 && Rudder_real_poisiton>48))
            {
              if(Rudder_initialized_time==0)
              {
                Rudder_initialized_time=millis();
              }
              else
              {
                unsigned long Rudder_initialzing_time_Now = millis();
                //wait 3s for the initializing
                //Serial.print("Rudder initializing...");
                //Serial.println(Rudder_initialzing_time_Now-Rudder_initialized_time);
                if( (Rudder_initialzing_time_Now-Rudder_initialized_time)> Rudder_timeout )
                {
                  HeliRudder_initializing=false;
                  moveSlowlyToPosition_b=false;
                  Serial.println("HeliRudder initialized");
                  dap_calculationVariables_st.isHelicopterRudderInitialized=true;
                  Rudder_initialized_time=0;
                  #ifdef USING_BUZZER
                    Buzzer.play_melody_tone(melodyAirwolfTheme, sizeof(melodyAirwolfTheme)/sizeof(melodyAirwolfTheme[0]),melodyAirwolfThemeDuration);
                  #endif
                }
              }
            }
          }
          if(HeliRudder_deinitializing)
          {
            moveSlowlyToPosition_b=true;
              //Serial.println("moving to min end stop");
          }
          if(HeliRudder_deinitializing && (Rudder_real_poisiton< 2 ))
          {
            HeliRudder_deinitializing=false;
            moveSlowlyToPosition_b=false;
            Serial.println("HeliRudder deinitialized");
            dap_calculationVariables_st.isHelicopterRudderInitialized=false;
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

        // set position command smoothing
        stepper->configSetPositionCommandSmoothingFactor(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.positionSmoothingFactor_u8);

        // reset all servo alarms
        if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS) )
        {
          Serial.println("Set clear alarm history flag");
          stepper->clearAllServoAlarms();
          delay(1000); // makes sure the routine has finished

          DAP_config_st tmp = global_dap_config_class.getConfig();
          tmp.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS); // clear the debug bit
          global_dap_config_class.setConfig(tmp);

        }

        // print all servo parameters for debug purposes
        if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS) )
        {
          DAP_config_st tmp = global_dap_config_class.getConfig();
          tmp.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS); // clear the debug bit
          global_dap_config_class.setConfig(tmp);

          delay(1000);  
          stepper->printAllServoParameters();
        }


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
          if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)0)==pdTRUE) {
            float joystickNormalizedToInt32_orig=0.0f;
            float joystickfrac =0.0f;
            float joystickNormalizedToInt32_eval=0.0f;
            if(dap_calculationVariables_st.Rudder_status&&dap_calculationVariables_st.rudder_brake_status)
            {
              if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
              {
                joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((Position_Next-dap_calculationVariables_st.stepperPosRange/2), dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMin+dap_calculationVariables_st.stepperPosRange/2.0f, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
                //joystickNormalizedToInt32 = constrain(joystickNormalizedToInt32,0,JOYSTICK_MAX_VALUE);
              }
              else
              {
                joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((FilterReadingJoystick/*filteredReading*/), dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
                //joystickNormalizedToInt32 = constrain(joystickNormalizedToInt32,0,JOYSTICK_MAX_VALUE);
              }
            }
            else
            {
              if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
              {
                joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
              }
              else
              {            
                joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(FilterReadingJoystick/*filteredReading*/, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
              }
            }
            joystickfrac=(float)joystickNormalizedToInt32_orig/(float)JOYSTICK_MAX_VALUE;
            joystickNormalizedToInt32_eval = forceCurve.EvalJoystickCubicSpline(&dap_config_pedalUpdateTask_st, &dap_calculationVariables_st, joystickfrac);
            joystickNormalizedToInt32 = joystickNormalizedToInt32_eval/100.0f* JOYSTICK_MAX_VALUE;
            joystickNormalizedToInt32 = constrain(joystickNormalizedToInt32,0,JOYSTICK_MAX_VALUE);
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

        // end profiler 6, ...
        profiler_pedalUpdateTask.end(6);


        // start profiler 6, struct exchange
        profiler_pedalUpdateTask.start(7);

        // update extended pedal structures
        dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0; // 170
        dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1; // 85

        dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame0_u8 =  EOF_BYTE_0; // 170
        dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame1_u8 =  EOF_BYTE_1; // 86

        // update extended struct 
        //dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.timeInMs_u32 = millis();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.timeInUs_u32 = micros();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.pedalForce_raw_fl32 =  loadcellReading;
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.pedalForce_filtered_fl32 =  filteredReading;
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.forceVel_est_fl32 =  changeVelocity;

        //dap_state_extended_st.payloadPedalState_Extended_.servoPosition_i16 = stepper->getServosInternalPosition();
        int32_t minPos = 0; //stepper->getMinPosition();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPosition_i16 = stepper->getServosInternalPositionCorrected() - minPos;
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_voltage_0p1V =  stepper->getServosVoltage();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_current_percent_i16 = stepper->getServosCurrent();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_position_error_i16 = stepper->getServosPosError();
        
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPositionEstimated_i16 = stepper->getEstimatedPosError();

        
        //dap_state_extended_st.payloadPedalState_Extended_.servoPositionTarget_i16 = stepper->getCurrentPositionFromMin();
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPositionTarget_i16 = stepper->getCurrentPosition() - minPos;
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.angleSensorOutput_ui16 = angleReading_ui16;
        dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.brakeResistorState_b = stepper->getBrakeResistorState();
        dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.PedalTag = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;
        dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_EXTENDED;
        dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.version = DAP_VERSION_CONFIG;
        //dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_)), sizeof(dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_) + sizeof(dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_));

        

        // end profiler 7, struct exchange
        profiler_pedalUpdateTask.end(7);

        // start profiler 8, struct exchange
        profiler_pedalUpdateTask.start(8);
        
        // update basic pedal state struct
        static DRAM_ATTR DAP_state_basic_st dap_state_basic_st_lcl_pedalUpdateTask;

        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalForce_u16 =  normalizedPedalReading_fl32 * 65535.0f;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalPosition_u16 = constrain(stepperPosFraction, 0.0f, 1.0f) * 65535.0f;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.joystickOutput_u16 = (float)joystickNormalizedToInt32 / 10000.0f * 32767.0f;//65535;
        //parse_version_fast(DAP_FIRMWARE_VERSION, &dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[0], &dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[1], &dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[2]);
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[0]=versionMajor;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[1]=versionMinor;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[2]=versionPatch;
        //error code
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.erroe_code_u8=0;
        
        //servo status update
        dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.servoStatus=stepper->servoStatus;
        
        #ifdef ESPNOW_Enable
          if(ESPNow_error_code!=0)
          {
            dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.erroe_code_u8=ESPNow_error_code;
            ESPNow_error_code=0;
          }
        #endif

        if( (stepper->getLifelineSignal()==false) && (stepper->servoStatus!=SERVO_IDLE_NOT_CONNECTED) )
        {
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.erroe_code_u8=12;
        }

        //fill the header
        dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
        dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
        dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;

        dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_BASIC;
        dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.version = DAP_VERSION_CONFIG;
        //dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_)), sizeof(dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_) + sizeof(dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_));
        dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.PedalTag = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;        
        
        

        // update pedal states
        if(semaphore_updatePedalStates!=NULL)
        {
          if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)0)==pdTRUE) 
          {
            
            // move local structure values to global structures
            dap_state_basic_st = dap_state_basic_st_lcl_pedalUpdateTask;
            dap_state_extended_st = dap_state_extended_st_lcl_pedalUpdateTask;

            // release semaphore
            xSemaphoreGive(semaphore_updatePedalStates);
          }
        }
        else
        {
          semaphore_updatePedalStates = xSemaphoreCreateMutex();
        }

        // start profiler 8, struct exchange
        profiler_pedalUpdateTask.end(8);

        

        

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



        profiler_pedalUpdateTask.end(0);

        // print profiler results
        profiler_pedalUpdateTask.report();
      
      }
    }

  }
}

  






/**********************************************************************************************/
/*                                                                                            */
/*                         joystick output task                                               */
/*                                                                                            */
/**********************************************************************************************/
void IRAM_ATTR joystickOutputTask( void * pvParameters )
{ 
  int32_t joystickNormalizedToInt32_local = 0;

  FunctionProfiler profiler_joystickOutputTask(nullptr, "joystickOutputTask");
  registerProfiler(&profiler_joystickOutputTask);   // hook profiler into task
  profiler_joystickOutputTask.setName("JoystickOutput");
  profiler_joystickOutputTask.setNumberOfCalls(500);


  for(;;){

    // wait for the timer to fire
    // This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
    // if(handle_joystickOutput != NULL)
    {
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

        // copy global struct to local for faster and safe executiion
        DAP_config_st jut_dap_config_st = global_dap_config_class.getConfig();

        // activate profiler depending on pedal config
        if (jut_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_joystickOutputTask.activate( true );
        }
        else
        {
          profiler_joystickOutputTask.activate( false );
        }

        // start profiler 0, overall function
        profiler_joystickOutputTask.start(0);


        // obtain joystick output level
        if(semaphore_updateJoystick!=NULL)
        {
          if(xSemaphoreTake(semaphore_updateJoystick, (TickType_t)1)==pdTRUE)
          {
            //Serial.print(" 3");
            joystickNormalizedToInt32_local = joystickNormalizedToInt32;
            xSemaphoreGive(semaphore_updateJoystick);
          }
        }


        // send joystick output
        #if defined(USB_JOYSTICK) || defined(BLUETOOTH_GAMEPAD)
          if (IsControllerReady()) 
          {
            if(dap_calculationVariables_st.Rudder_status==false)
            {
              //general output
              SetControllerOutputValue(joystickNormalizedToInt32_local);
              
              #ifdef USB_JOYSTICK
                // Restart HID output if faulty behavior was detected
                JoystickSendState();
                if(!GetJoystickStatus())
                {
                  RestartJoystick();
                  Serial.println("HID Error, Restart Joystick...");
                  //last_serial_joy_out=millis();
                }
              #endif

            }
          }
        #endif


        // start profiler 0, overall function
        profiler_joystickOutputTask.end(0);

        profiler_joystickOutputTask.end(0);

        // print profiler results
        profiler_joystickOutputTask.report();

      }
    }
  }
}

/**********************************************************************************************/
/*                                                                                            */
/*                         communication task                                                 */
/*                                                                                            */
/**********************************************************************************************/
uint32_t communicationTask_stackSizeIdx_u32 = 0;
void IRAM_ATTR serialCommunicationTask( void * pvParameters )
{ 
  int32_t joystickNormalizedToInt32_local = 0;
  FunctionProfiler profiler_serialCommunicationTask(nullptr, "serialCommunicationTask");
  registerProfiler(&profiler_serialCommunicationTask);   // hook profiler into task
  profiler_serialCommunicationTask.setName("SerialCommunication");
  profiler_serialCommunicationTask.setNumberOfCalls(500);

  unsigned long previousTimeInUsFromExtendedStruct_u32 = 0;
  unsigned long printCycleCounter = 0;

  static uint16_t timerTicks_serialCommunicationTask_u16 = REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US / BASE_TICK_US;
  static uint16_t timerTicks_serialCommunicationTask_prev_u16 = timerTicks_serialCommunicationTask_u16;

  static DAP_config_st sct_dap_config_received_st;
  static DAP_config_st sct_dap_config_st;
  for(;;){

    sct_dap_config_st = global_dap_config_class.getConfig();

    // when debug flag == DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT is set --> change the serial communication task time
    if ( (sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT) )
    {
      // fast execution
      timerTicks_serialCommunicationTask_u16 = REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_FAST_IN_US / BASE_TICK_US;
    }
    else
    {
      // slow execution
      timerTicks_serialCommunicationTask_u16 = REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US / BASE_TICK_US;   
    }


    // check if timer intervall needs to change
    if (timerTicks_serialCommunicationTask_prev_u16 != timerTicks_serialCommunicationTask_u16)
    {
      // // search for task and update interval
      // for (int i = 0; i < taskCount; i++) {

      //   // if (tasks[i].name == String("serialCommunicationTask")) {
      //   if (tasks[i].name && strcmp(tasks[i].name, "serialCommunicationTask") == 0) {

      //       // update previous value
      //       timerTicks_serialCommunicationTask_prev_u16 = timerTicks_serialCommunicationTask_u16;

      //       Serial.print("Setting serial communication task to: ");
      //       Serial.print(timerTicks_serialCommunicationTask_u16 * BASE_TICK_US);
      //       Serial.println("us");

      //       tasks[i].intervalTicks = timerTicks_serialCommunicationTask_u16;
      //       tasks[i].counter = 0;  // reset counter
      //       break;
      //   }
      // }


      // update previous value
      timerTicks_serialCommunicationTask_prev_u16 = timerTicks_serialCommunicationTask_u16;

      Serial.print("Setting serial communication task to: ");
      Serial.print(timerTicks_serialCommunicationTask_u16 * BASE_TICK_US);
      Serial.println("us");

      
      tasks[1].intervalTicks = timerTicks_serialCommunicationTask_u16;
      tasks[1].counter = 0;  // reset counter

    }

    



    // wait for the timer to fire
    // This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
    // if(handle_serialCommunication != NULL)
    {
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

        // activate profiler depending on pedal config
        if (sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_serialCommunicationTask.activate( true );
        }
        else
        {
          profiler_serialCommunicationTask.activate( false );
        }


        // start profiler 0, overall function
        profiler_serialCommunicationTask.start(0);

        // start profiler 1, serial read
        profiler_serialCommunicationTask.start(1);


        uint16_t crc;

      
        { 
          // read serial input 
          uint8_t n = Serial.available();

          bool structChecker = true;
          
          if (n > 0)
          {
            switch (n) {

              // likely config structure 
              case sizeof(DAP_config_st):

                DAP_config_st * dap_config_st_local_ptr;
                dap_config_st_local_ptr = &sct_dap_config_received_st;
                Serial.readBytes((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));

                // check if data is plausible
                if ( sct_dap_config_received_st.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
                  structChecker = false;
                  Serial.print("Payload type expected: ");
                  Serial.print(DAP_PAYLOAD_TYPE_CONFIG);
                  Serial.print(",   Payload type received: ");
                  Serial.println(sct_dap_config_received_st.payLoadHeader_.payloadType);
                  break; // Exit case early
                }

                if ( sct_dap_config_received_st.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
                  structChecker = false;
                  Serial.print("Config version expected: ");
                  Serial.print(DAP_VERSION_CONFIG);
                  Serial.print(",   Config version received: ");
                  Serial.println(sct_dap_config_received_st.payLoadHeader_.version);
                  break; // Exit case early
                }
                // checksum validation
                crc = checksumCalculator((uint8_t*)(&(sct_dap_config_received_st.payLoadHeader_)), sizeof(sct_dap_config_received_st.payLoadHeader_) + sizeof(sct_dap_config_received_st.payLoadPedalConfig_));
                if (crc != sct_dap_config_received_st.payloadFooter_.checkSum){ 
                  structChecker = false;
                  Serial.print("CRC expected: ");
                  Serial.print(crc);
                  Serial.print(",   CRC received: ");
                  Serial.println(sct_dap_config_received_st.payloadFooter_.checkSum);
                  
                  // No need to break here, as it's the last check before the final 'if'
                  Serial.print("Headersize: ");
                  Serial.print(sizeof(sct_dap_config_received_st.payLoadHeader_));
                  Serial.print(",    Configsize: ");
                  Serial.println(sizeof(sct_dap_config_received_st.payLoadPedalConfig_));
                }


                // if checks are successfull, overwrite global configuration struct
                if (structChecker == true)
                {
                  Serial.println("Updating pedal config");

                  global_dap_config_class.setConfig(sct_dap_config_received_st);
                  sct_dap_config_st = global_dap_config_class.getConfig();
                  configUpdateAvailable = true; 

                  #ifdef USING_BUZZER
                    if(sct_dap_config_st.payLoadHeader_.storeToEeprom==1)
                    {
                      Buzzer.single_beep_tone(700,100);
                    }     
                  #endif        
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
                  Serial.println(dap_actions_st.payLoadHeader_.payloadType);
                }
                if ( dap_actions_st.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
                  structChecker = false;
                  Serial.print("Config version expected: ");
                  Serial.print(DAP_VERSION_CONFIG);
                  Serial.print(",   Config version received: ");
                  Serial.println(dap_actions_st.payLoadHeader_.version);
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
                  if (dap_actions_st.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
                  {
                    char logString[200];
                    snprintf(logString, sizeof(logString),
                            "Pedal ID: %d\nBoard: %s\nLoadcell shift= %.3f kg\nLoadcell variance= %.3f kg\nPSU voltage:%.1f V\nMax endstop:%lu\nCurrentPos:%lu\n\0",
                            sct_dap_config_st.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, loadcell->getShiftingEstimate(), loadcell->getSTDEstimate(), ((float)stepper->getServosVoltage() / 10.0f), dap_calculationVariables_st.stepperPosMaxEndstop, dap_calculationVariables_st.current_pedal_position);
                    Serial.println(logString);
                  }

                  // trigger ABS effect
                  if (dap_actions_st.payloadPedalAction_.triggerAbs_u8>0)
                  {
                    absOscillation.trigger();
                    if(dap_actions_st.payloadPedalAction_.triggerAbs_u8>1)
                    {
                      dap_calculationVariables_st.TrackCondition=dap_actions_st.payloadPedalAction_.triggerAbs_u8-1;
                    }
                    else
                    {
                      dap_calculationVariables_st.TrackCondition=dap_actions_st.payloadPedalAction_.triggerAbs_u8=0;
                    }
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
                    dap_config_st_local_ptr = &sct_dap_config_st;
                    dap_config_st_local_ptr->payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
                    dap_config_st_local_ptr->payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
                    dap_config_st_local_ptr->payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
                    dap_config_st_local_ptr->payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
                    crc = checksumCalculator((uint8_t*)(&(sct_dap_config_st.payLoadHeader_)), sizeof(sct_dap_config_st.payLoadHeader_) + sizeof(sct_dap_config_st.payLoadPedalConfig_));
                    dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
                    Serial.write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
                    // Serial.print("\r\n");
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
              case sizeof(DAP_otaWifiInfo_st) : 
              Serial.println("get basic wifi info");
              Serial.readBytes((char*)&_dap_OtaWifiInfo_st, sizeof(DAP_otaWifiInfo_st));
              #ifdef OTA_update
                if(_dap_OtaWifiInfo_st.device_ID == sct_dap_config_st.payLoadPedalConfig_.pedal_type)
                {
                  SSID=new char[_dap_OtaWifiInfo_st.SSID_Length+1];
                  PASS=new char[_dap_OtaWifiInfo_st.PASS_Length+1];
                  memcpy(SSID,_dap_OtaWifiInfo_st.WIFI_SSID,_dap_OtaWifiInfo_st.SSID_Length);
                  memcpy(PASS,_dap_OtaWifiInfo_st.WIFI_PASS,_dap_OtaWifiInfo_st.PASS_Length);
                  SSID[_dap_OtaWifiInfo_st.SSID_Length]=0;
                  PASS[_dap_OtaWifiInfo_st.PASS_Length]=0;
                  OTA_enable_b=true;
                }
              #endif
              #ifdef OTA_update_ESP32
                Serial.println("Get OTA command");
                OTA_enable_b=true;
                //OTA_enable_start=true;
                ESPNow_OTA_enable=false;
                //Serial.println("get basic wifi info");
                //Serial.readBytes((char*)&_basic_wifi_info, sizeof(Basic_WIfi_info));
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


          // start profiler 1, serial read
          profiler_serialCommunicationTask.end(1);

          // start profiler 2, serial send
          profiler_serialCommunicationTask.start(2);


          // send pedal state structs
          // update pedal states
          printCycleCounter++;
          DAP_state_basic_st dap_state_basic_st_lcl;
          DAP_state_extended_st dap_state_extended_st_lcl;
            
          // initialize with zeros in case semaphore couldn't be aquired
          // memset(&dap_state_basic_st_lcl, 0, sizeof(dap_state_basic_st_lcl));
          // memset(&dap_state_extended_st_lcl, 0, sizeof(dap_state_extended_st_lcl));


          if(semaphore_updatePedalStates!=NULL)
          {
            
            if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)5)==pdTRUE) 
            {
            
              // UPDATE basic pedal state struct
              dap_state_basic_st_lcl = dap_state_basic_st;

              // UPDATE extended pedal state struct
              dap_state_extended_st_lcl = dap_state_extended_st;
                
              // release semaphore
              xSemaphoreGive(semaphore_updatePedalStates);

            }
          }

          
          // end profiler 2, serial send
          profiler_serialCommunicationTask.end(2);

          // end profiler 2, serial send
          profiler_serialCommunicationTask.start(3);


          // send the pedal state structs
          // send basic pedal state struct
          if ( !(sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_BASIC_INFO_STRUCT) )
          {
            if (printCycleCounter >= 2)
            {
              printCycleCounter = 0;

              // update CRC before transmission
              dap_state_basic_st_lcl.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl.payLoadHeader_)), sizeof(dap_state_basic_st_lcl.payLoadHeader_) + sizeof(dap_state_basic_st_lcl.payloadPedalState_Basic_));
          
              Serial.write((char*)&dap_state_basic_st_lcl, sizeof(DAP_state_basic_st));
              // Serial.flush();
          
              // Serial.print("\r\n");
            }
          }

          if ( (sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT) )
          {
            // only send, when extended struct contains updated values
            if( dap_state_extended_st_lcl.payloadPedalState_Extended_.timeInUs_u32 != previousTimeInUsFromExtendedStruct_u32)
            {
              previousTimeInUsFromExtendedStruct_u32 = dap_state_extended_st_lcl.payloadPedalState_Extended_.timeInUs_u32;

              //dap_state_extended_st_lcl.payloadPedalState_Extended_.timeInUsFromSerialTask_u32 = micros();

              // update CRC before transmission
              dap_state_extended_st_lcl.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_extended_st_lcl.payLoadHeader_)), sizeof(dap_state_extended_st_lcl.payLoadHeader_) + sizeof(dap_state_extended_st_lcl.payloadPedalState_Extended_));

              Serial.write((char*)&dap_state_extended_st_lcl, sizeof(DAP_state_extended_st));
              // Serial.print("\r\n");
              // Serial.flush();
            }
          }

        }


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

        // end profiler 3, serial send
        profiler_serialCommunicationTask.end(3);

        profiler_serialCommunicationTask.end(0);

        // print profiler results
        profiler_serialCommunicationTask.report();
      }
    }
  }
}



//OTA multitask
bool OTA_enable_start=false;
void OTATask( void * pvParameters )
{
  uint16_t OTA_count=0;
  bool message_out_b=false;
  int OTA_update_status=99;

  for(;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

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
            if(_dap_OtaWifiInfo_st.wifi_action==1)
            {
              const char* str ="0.0.0";
              version_tag=new char[strlen(str) + 1];
              strcpy(version_tag, str);
              Serial.println("Force update");
            }
            else
            {
              version_tag=new char[strlen(DAP_FIRMWARE_VERSION) + 1];
              strcpy(version_tag, DAP_FIRMWARE_VERSION);
              //version_tag=DAP_FIRMWARE_VERSION;
            }
            switch (_dap_OtaWifiInfo_st.mode_select)
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
              case 3:
                Serial.printf("Flashing to Daily build, checking %s to see if an update is available...\n", JSON_URL_dev);
                ret = ota.CheckForOTAUpdate(JSON_URL_daily, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                Serial.printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                OTA_update_status=ret;
                break;
              default:
              break;
              delete[] version_tag; 
            }
            #endif

            delay(3000);
          }

        }
      }
      
      #endif
    }
  }
}

#ifdef ESPNOW_Enable

void IRAM_ATTR ESPNOW_SyncTask( void * pvParameters )
{
  FunctionProfiler profiler_espNow(nullptr, "ESPNOW_SyncTask");
  registerProfiler(&profiler_espNow);   // hook profiler into task
  profiler_espNow.setName("EspNow");

  uint Pairing_timeout=20000;
  uint rudderPacketInterval=3;
  uint joystickPacketInterval=2;
  bool Pairing_timeout_status=false;
  bool building_dap_esppairing_lcl =false;
  unsigned long Pairing_state_start;
  unsigned long Pairing_state_last_sending;
  unsigned long Debug_rudder_last=0;
  unsigned long basic_state_update_last=0;
  unsigned long extend_state_update_last=0;
  unsigned long rudderPacketsUpdateLast=0;
  unsigned long joystickPacketsUpdateLast=0;
  uint32_t espNowTask_stackSizeIdx_u32 = 0;

  int error_count=0;
  int print_count=0;
  int ESPNow_no_device_count=0;
  bool basic_state_send_b=false;
  bool extend_state_send_b=false;
  uint8_t error_out;

  for(;;)
  {
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

        //restart from espnow
        if(ESPNow_restart)
        {
          Serial.println("ESP restart by ESP now request");
          ESP.restart();
        }

        
        //basic state sendout interval
        if(millis()-basic_state_update_last>3)
        {
          basic_state_send_b=true;
          basic_state_update_last=millis();
          
        }

        DAP_config_st espnow_dap_config_st = global_dap_config_class.getConfig();

        //entend state send out interval
        if((millis()-extend_state_update_last>10) && espnow_dap_config_st.payLoadPedalConfig_.debug_flags_0 == DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT)
        {
          extend_state_send_b=true;
          extend_state_update_last=millis();
          
        }

        // activate profiler depending on pedal config
        if (espnow_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_espNow.activate( true );
        }
        else
        {
          profiler_espNow.activate( false );
        }

        // start profiler 0, overall function
        profiler_espNow.start(0);

        
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
                dap_esppairing_lcl.payloadESPNowInfo_._deviceID = espnow_dap_config_st.payLoadPedalConfig_.pedal_type;
                dap_esppairing_lcl.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_ESPNOW_PAIRING;
                dap_esppairing_lcl.payLoadHeader_.PedalTag = espnow_dap_config_st.payLoadPedalConfig_.pedal_type;
                dap_esppairing_lcl.payLoadHeader_.version = DAP_VERSION_CONFIG;
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
                Serial.print(espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
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
                      if(espnow_dap_config_st.payLoadPedalConfig_.pedal_type==1)
                      {
                        Recv_mac=Gas_mac;
                      }
                      if(espnow_dap_config_st.payLoadPedalConfig_.pedal_type==2)
                      {
                        Recv_mac=Brk_mac;
                      }
                    }
                  }
                }
              }
            }
          #endif

          profiler_espNow.start(1);

          //joystick value broadcast
          if((joystickPacketsUpdateLast-millis())>joystickPacketInterval) 
          {
            ESPNow_Joystick_Broadcast(joystickNormalizedToInt32);
            joystickPacketsUpdateLast=millis();
          }
          
          profiler_espNow.end(1);

          profiler_espNow.start(2);

          if(basic_state_send_b)
          {
            // update pedal states
            DAP_state_basic_st dap_state_basic_st_lcl;       
            // initialize with zeros in case semaphore couldn't be aquired
            memset(&dap_state_basic_st_lcl, 0, sizeof(dap_state_basic_st_lcl));
            if(semaphore_updatePedalStates!=NULL)
            {  
              if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)5)==pdTRUE) 
              {
                // UPDATE basic pedal state struct
                dap_state_basic_st_lcl = dap_state_basic_st;

                // release semaphore
                xSemaphoreGive(semaphore_updatePedalStates);
                dap_state_basic_st_lcl.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_basic_st_lcl.payLoadHeader_)), sizeof(dap_state_basic_st_lcl.payLoadHeader_) + sizeof(dap_state_basic_st_lcl.payloadPedalState_Basic_));
              }
            }
            else
            {
              semaphore_updatePedalStates = xSemaphoreCreateMutex();
            }
            ESPNow.send_message(broadcast_mac,(uint8_t *) & dap_state_basic_st_lcl,sizeof(dap_state_basic_st_lcl));
            basic_state_send_b=false;
          }

          profiler_espNow.end(2);

          profiler_espNow.start(3);

          if(extend_state_send_b)
          {
            // update pedal states
            DAP_state_extended_st dap_state_extended_st_espNow; 
            // initialize with zeros in case semaphore couldn't be aquired
            memset(&dap_state_extended_st_espNow, 0, sizeof(dap_state_extended_st_espNow));
            if(semaphore_updatePedalStates!=NULL)
            {  
              if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)5)==pdTRUE) 
              {
                // UPDATE extended pedal state struct
                dap_state_extended_st_espNow = dap_state_extended_st; 
                // release semaphore
                xSemaphoreGive(semaphore_updatePedalStates);
                dap_state_extended_st_espNow.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(dap_state_extended_st_espNow.payLoadHeader_)), sizeof(dap_state_extended_st_espNow.payLoadHeader_) + sizeof(dap_state_extended_st_espNow.payloadPedalState_Extended_));
              }
            }
            else
            {
              semaphore_updatePedalStates = xSemaphoreCreateMutex();
            }

            ESPNow.send_message(broadcast_mac,(uint8_t *)&dap_state_extended_st_espNow, sizeof(dap_state_extended_st_espNow));
            extend_state_send_b=false;
          }

          profiler_espNow.end(3);


          if(ESPNow_config_request)
          {
            DAP_config_st * dap_config_st_local_ptr;
            dap_config_st_local_ptr = &espnow_dap_config_st;
            dap_config_st_local_ptr->payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
            dap_config_st_local_ptr->payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
            dap_config_st_local_ptr->payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
            dap_config_st_local_ptr->payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
            uint16_t crc=0;
            crc = checksumCalculator((uint8_t*)(&(espnow_dap_config_st.payLoadHeader_)), sizeof(espnow_dap_config_st.payLoadHeader_) + sizeof(espnow_dap_config_st.payLoadPedalConfig_));
            dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
            ESPNow.send_message(broadcast_mac,(uint8_t *) & espnow_dap_config_st, sizeof(espnow_dap_config_st));
            ESPNow_config_request=false;
          }
          if(Config_update_b)
          {
            Config_update_b=false;
            #ifdef USING_BUZZER
            if (espnow_dap_config_st.payLoadHeader_.storeToEeprom == 1)
            {
              Buzzer.single_beep_tone(700, 100);
            }          
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
            Serial.readBytes((char*)&_dap_OtaWifiInfo_st, sizeof(DAP_otaWifiInfo_st));
            #ifdef OTA_update
              if(_dap_OtaWifiInfo_st.device_ID == espnow_dap_config_st.payLoadPedalConfig_.pedal_type)
              {
                SSID=new char[_dap_OtaWifiInfo_st.SSID_Length+1];
                PASS=new char[_dap_OtaWifiInfo_st.PASS_Length+1];
                memcpy(SSID,_dap_OtaWifiInfo_st.WIFI_SSID,_dap_OtaWifiInfo_st.SSID_Length);
                memcpy(PASS,_dap_OtaWifiInfo_st.WIFI_PASS,_dap_OtaWifiInfo_st.PASS_Length);
                SSID[_dap_OtaWifiInfo_st.SSID_Length]=0;
                PASS[_dap_OtaWifiInfo_st.PASS_Length]=0;
                OTA_enable_b=true;
              }
              #endif

          }


          if(printPedalInfo_b)
          {
            printPedalInfo_b=false;
            /*
            char logString[200];
            snprintf(logString, sizeof(logString),
                    "Pedal ID: %d\nBoard: %s\nLoadcell shift= %.3f kg\nLoadcell variance= %.3f kg\nPSU voltage:%.1f V\nMax endstop:%lu\nCurrentPos:%d\0",
                    espnow_dap_config_st.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, loadcell->getShiftingEstimate(), loadcell->getSTDEstimate(), ((float)stepper->getServosVoltage()/10.0f),dap_calculationVariables_st.stepperPosMaxEndstop,dap_calculationVariables_st.current_pedal_position);
            Serial.println(logString);
            sendESPNOWLog(logString, strnlen(logString, sizeof(logString)));
            */
            pedalInfoBuilder.BuildString(espnow_dap_config_st.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, loadcell->getShiftingEstimate(), loadcell->getSTDEstimate(), ((float)stepper->getServosVoltage()/10.0f),dap_calculationVariables_st.stepperPosMaxEndstop,dap_calculationVariables_st.current_pedal_position);
            Serial.println(pedalInfoBuilder.logString);
            sendESPNOWLog(pedalInfoBuilder.logString, strnlen(pedalInfoBuilder.logString, sizeof(pedalInfoBuilder.logString)));
            pedalInfoBuilder.BuildESPNOWInfo(espnow_dap_config_st.payLoadPedalConfig_.pedal_type,rssi);
            Serial.println(pedalInfoBuilder.logESPNOWString);
            delay(3);
            sendESPNOWLog(pedalInfoBuilder.logESPNOWString, strnlen(pedalInfoBuilder.logESPNOWString, sizeof(pedalInfoBuilder.logESPNOWString)));

          }
          if(Get_Rudder_action_b)
          {
            Get_Rudder_action_b=false;
            #ifdef USING_BUZZER
            Buzzer.single_beep_tone(700,100);
            #endif
          }
          if(Get_HeliRudder_action_b)
          {
            Get_HeliRudder_action_b=false;
            #ifdef USING_BUZZER
            Buzzer.single_beep_tone(700,100);
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
          //send out rudder packet after rudder initialized
          if(rudderPacketsUpdateLast-millis()>rudderPacketInterval)
          {
            if((dap_calculationVariables_st.Rudder_status || dap_calculationVariables_st.helicopterRudderStatus) && (!Rudder_initializing && !HeliRudder_initializing))
            {              
              dap_rudder_sending.payloadRudderState_.pedal_position_ratio=dap_calculationVariables_st.current_pedal_position_ratio;
              dap_rudder_sending.payloadRudderState_.pedal_position=dap_calculationVariables_st.current_pedal_position;
              dap_rudder_sending.payLoadHeader_.payloadType=DAP_PAYLOAD_TYPE_ESPNOW_RUDDER;
              dap_rudder_sending.payLoadHeader_.PedalTag = espnow_dap_config_st.payLoadPedalConfig_.pedal_type;
              dap_rudder_sending.payLoadHeader_.version=DAP_VERSION_CONFIG;
              uint16_t crc=0;
              crc = checksumCalculator((uint8_t*)(&(dap_rudder_sending.payLoadHeader_)), sizeof(dap_rudder_sending.payLoadHeader_) + sizeof(dap_rudder_sending.payloadRudderState_));
              dap_rudder_sending.payloadFooter_.checkSum=crc;
              ESPNow.send_message(broadcast_mac,(uint8_t *) &dap_rudder_sending,sizeof(dap_rudder_sending));   
              //ESPNow_send=dap_calculationVariables_st.current_pedal_position; 
              //esp_err_t result =ESPNow.send_message(Recv_mac,(uint8_t *) &_ESPNow_Send,sizeof(_ESPNow_Send));                
              //if (result == ESP_OK) 
              //{
              //  Serial.println("Error sending the data");
              //}                
              if(ESPNow_Rudder_Update)
              {
                //dap_calculationVariables_st.sync_pedal_position=ESPNow_recieve;
                dap_calculationVariables_st.sync_pedal_position=dap_rudder_receiving.payloadRudderState_.pedal_position;
                dap_calculationVariables_st.Sync_pedal_position_ratio=dap_rudder_receiving.payloadRudderState_.pedal_position_ratio;
                ESPNow_Rudder_Update=false;
              }                
            }
            rudderPacketsUpdateLast=millis();
          }    
        }

        #ifdef ESPNow_debug_rudder
          if(print_count>1000)
          {
            if(dap_calculationVariables_st.Rudder_status)
            {
              Serial.print("Pedal:");
              Serial.print(espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
              Serial.print(", Send %: ");
              Serial.print(dap_rudder_sending.payloadRudderState_.pedal_position_ratio);
              Serial.print(", Recieve %:");
              Serial.print(dap_rudder_receiving.payloadRudderState_.pedal_position_ratio);
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

      profiler_espNow.end(0);

      // print profiler results
      profiler_espNow.report();

    }
  

}
#endif

#define CONFIG_PREVIEW_DURATION 180000// wait 3 mins then save config into eeprom
void miscTask( void * pvParameters )
{
  // for the task no need complete asap, ex buzzer, led 
  for(;;)
  {
    DAP_config_st misc_dap_config_st = global_dap_config_class.getConfig();
    if(previewConfigGet_b && ((millis()-saveToEEPRomDuration)>CONFIG_PREVIEW_DURATION))
    {
      
      Serial.println("Auto save config in pedal");
      /*
      Serial.print(millis());
      Serial.print(" Duration:");
      Serial.print(saveToEEPRomDuration);
      Serial.print(" flag:");
      Serial.println(previewConfigGet_b);
      */
      //saveToEEPRomDuration=0;
      global_dap_config_class.storeConfigToEprom();
      previewConfigGet_b=false;
      #ifdef USING_BUZZER
        Buzzer.single_beep_tone(700,50);
        delay(50);
        Buzzer.single_beep_tone(700,50);
      #endif 


    }
    delay(100);
  }
}
