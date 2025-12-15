
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779


#include "esp_timer.h" // Include the header for the high-resolution timer
#include "esp_partition.h"


#define ESTIMATE_LOADCELL_VARIANCE
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER 1
#define DEBUG_INFO_0_NET_RUNTIME 2
// #define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS 8
#define DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS 16
#define DEBUG_INFO_0_RESET_SERVO_TO_FACTORY 32
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT 64
#define DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS 128


#define EFFECT_SCALING_FACTOR_FL32 4.0f;
#define EFFECT_POSITION_SCALING_FACTOR_FL32 0.1f;

#define BAUD3M 3000000
#define DEFAULTBAUD 921600
#include "Arduino.h"
#include "Main.h"
#ifdef CONFIG_IDF_TARGET_ESP32S3
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"
#endif
Stream *ActiveSerial = nullptr;

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
void profilerTask( void * pvParameters );
void serialCommunicationTaskRx( void * pvParameters );
void serialCommunicationTaskTx( void * pvParameters );
void otaUpdateTask( void * pvParameters );
void espNowCommunicationTaskTx( void * pvParameters);
void miscTask( void * pvParameters);
void configUpdateTask( void * pvParameters );

#ifdef USB_JOYSTICK
  void joystickOutputTask( void * pvParameters );
#endif


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



#include "DiyActivePedal_types.h"

DAP_config_class global_dap_config_class;
DRAM_ATTR DAP_calculationVariables_st dap_calculationVariables_st;
DAP_state_basic_st dap_state_basic_st;
DAP_state_extended_st dap_state_extended_st;
DAP_ESPPairing_st dap_esppairing_st;//saving
DAP_ESPPairing_st dap_esppairing_lcl;//sending
DAP_action_ota_st dap_action_ota_st;//OTA command(do not check version)



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
  #include "rtc_wdt.h"
#endif





static SemaphoreHandle_t semaphore_updatePedalStates=NULL;



/**********************************************************************************************/
/*                                                                                            */
/*                         queue declarations                                                 */
/*                                                                                            */
/**********************************************************************************************/
// ADD THIS: The handle for our new FreeRTOS queue
static QueueHandle_t pedalStateQueue = NULL;
static QueueHandle_t joystickDataQueue = NULL;
static QueueHandle_t loadcellDataQueue = NULL;
static QueueHandle_t configUpdateAvailableQueue = NULL;
static QueueHandle_t configUpdateSendToPedalUpdateTaskQueue = NULL;
static QueueHandle_t configUpdateSendToLoadcellTaskQueue = NULL;
// static QueueHandle_t configUpdateSendToJoystickTaskQueue = NULL;
static QueueHandle_t configUpdateSendToSerialRXTaskQueue = NULL;



// ADD THIS: New data structure to send both states together in one package
typedef struct {
    DAP_state_basic_st    basic_st;
    DAP_state_extended_st extended_st;
    bool sendBasicFlag_b;
    bool sendExtendedFlag_b;
} PedalStatePackage_t;

typedef struct {
    uint16_t joystickNormalizedToUInt16;
    bool sendJoystickFlag_b;
} joystickDataPackage_t;

typedef struct {
    float loadcellReadingInKg_fl32;
} loadcellDataPackage_t;

typedef struct {
    DAP_config_st config_st;
} configDataPackage_t;



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

#include "SignalFilter_1st_order.h"
KalmanFilter_1st_order* kalman = NULL;
KalmanFilter_1st_order* kalman_joystick = NULL;

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
#include "StepperMovementStrategy_MPC.h"
#include "ChatterReduction.h"
bool moveSlowlyToPosition_b = true;
/**********************************************************************************************/
/*                                                                                            */
/*                         OTA                                                                */
/*                                                                                            */
/**********************************************************************************************/
//OTA update
#ifdef OTA_update
//#include "ota.h"
#include "OTA_Pull.h"
#include "OTA_ArduinoOTA.h"
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


#include "Buzzer.h"
Simple_Buzzer Buzzer;
bool buzzerBeepAction_b = false;
#include <cstring>


/**********************************************************************************************/
/*                                                                                            */
/*                         profiler setup                                                     */
/*                                                                                            */
/**********************************************************************************************/
#include "FunctionProfiler.h"





/**********************************************************************************************/
/*                                                                                            */
/*                         config reading                                                     */
/*                                                                                            */
/**********************************************************************************************/
void IRAM_ATTR_FLAG configHandlingTask( void * pvParameters )
{
  DAP_config_st dap_config_st_local;
  configDataPackage_t configPackage_st;

  for(;;){

    // check if config update is available
    if (xQueueReceive(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY) == pdPASS) {
      global_dap_config_class.setConfig(configPackage_st.config_st);

      ActiveSerial->println("Config update received: config handling task");

      // send queues to other tasks

      // Send the package to the queue. Use a timeout of 0 (non-blocking).
      // If the queue is full, the data is simply dropped. This prevents this
      // high-priority control task from ever blocking on a full serial buffer.
      xQueueSend(configUpdateSendToPedalUpdateTaskQueue, &configPackage_st, portMAX_DELAY);
      xQueueSend(configUpdateSendToLoadcellTaskQueue, &configPackage_st, portMAX_DELAY);
      // xQueueSend(configUpdateSendToJoystickTaskQueue, &configPackage_st, portMAX_DELAY);
      xQueueSend(configUpdateSendToSerialRXTaskQueue, &configPackage_st, portMAX_DELAY);

    }
  }
}



/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell reading                                                   */
/*                                                                                            */
/**********************************************************************************************/
void IRAM_ATTR_FLAG loadcellReadingTask( void * pvParameters )
{

  static FunctionProfiler profiler_loadcellReading;
  profiler_loadcellReading.setName("loadcellReading");
  profiler_loadcellReading.setNumberOfCalls(3000);

  static float loadcellReading_fl32 = 0.0f;
  static DAP_config_st loadcellTask_dap_config_st;
  configDataPackage_t configPackage_st;

  static float previousLoadcellReadingInKg_fl32 = 0.0f;

  for(;;){

    if (loadcell != NULL)
    {

      // if new data package is available, update the local config
      if (xQueueReceive(configUpdateSendToLoadcellTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS) {
        loadcellTask_dap_config_st = configPackage_st.config_st;

        // activate profiler depending on pedal config
        if (loadcellTask_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_loadcellReading.activate( true );
        }
        else
        {
          profiler_loadcellReading.activate( false );
        }

        ActiveSerial->println("Update config: loadcell task");
      }     

      // start profiler 0, overall function
      profiler_loadcellReading.start(0);

      // no need for delay, since getReadingKg will block until DRDY edge down is detected
      loadcellReading_fl32 = loadcell->getReadingKg();

      // Invert the loadcell reading digitally if desired
      if (loadcellTask_dap_config_st.payLoadPedalConfig_.invertLoadcellReading_u8 == 1)
      {
        loadcellReading_fl32 *= -1.0f;
      }

      // detect loadcell outlier
      float loadcellDifferenceToLastCycle_fl32 = loadcellReading_fl32 - previousLoadcellReadingInKg_fl32;
      previousLoadcellReadingInKg_fl32 = loadcellReading_fl32;
      
      if (fabsf(loadcellDifferenceToLastCycle_fl32) < 5.0f)
      {
        // reject update when loadcell reading likely outlier
          
        // send joystick data to queue
        if (loadcellDataQueue != NULL) {

          // Package the new state data into a single struct
          loadcellDataPackage_t newLoadcellPackage;
          newLoadcellPackage.loadcellReadingInKg_fl32 = loadcellReading_fl32;

            // Send the package to the queue. Use a timeout of 0 (non-blocking).
            // If the queue is full, the data is simply dropped. This prevents this
            // high-priority control task from ever blocking on a full serial buffer.
          xQueueSend(loadcellDataQueue, &newLoadcellPackage, (TickType_t)0);
        }
      }
      
      
      


      profiler_loadcellReading.end(0);

      // print profiler results
      // profiler_loadcellReading.report();


    }

    // force a context switch
		taskYIELD();
  }
}



// === Scheduler config ===
#define BASE_TICK_US 300   // base tick in microseconds
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
DRAM_ATTR SchedTask tasks[MAX_TASKS];
uint8_t taskCount = 0;

// Timer handle
hw_timer_t *timer0 = NULL;

// === Scheduler ISR ===
void IRAM_ATTR_FLAG onTimer(void* arg) {
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
  if (xHigherPriorityWoken) 
  {
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
  tasks[taskCount].name = name;
  taskCount++;
}




TaskHandle_t handle_pedalUpdateTask = NULL;
TaskHandle_t handle_joystickOutput = NULL;
TaskHandle_t handle_loadcellReadingTask = NULL;
TaskHandle_t handle_profilerTask = NULL;
TaskHandle_t handle_serialCommunicationRx = NULL; 
TaskHandle_t handle_serialCommunicationTx = NULL; 
TaskHandle_t handle_miscTask = NULL; 
TaskHandle_t handle_otaTask = NULL;
TaskHandle_t handle_espnowTask = NULL;
TaskHandle_t handle_configHandlingTask = NULL;

#define COUNTER_SIZE 4u
uint16_t tickCount_au16[COUNTER_SIZE] = {0};


static uint16_t timerTicks_espNowTask_u16 = REPETITION_INTERVAL_ESPNOW_TASK_IN_US / BASE_TICK_US;




/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
 // #define SERIAL_PATTERN_DETECTOR
#ifdef SERIAL_PATTERN_DETECTOR

#include "driver/uart.h"

// Structure to hold a complete UART packet
#define UART_RX_BUF_SIZE sizeof(DAP_config_st)
typedef struct {
    uint8_t data[UART_RX_BUF_SIZE];
    size_t len;
} UartPacket_t;

// Queue to pass packets from the UART event task to the processing task
static QueueHandle_t serial_packet_queue;

// Queue to handle UART events
static QueueHandle_t uart_queue;

// --- ADD THIS LINE ---
#define TEMP_BUFFER_SIZE (UART_RX_BUF_SIZE * 2)

/**
 * @brief Task to handle UART events with persistent buffering.
 *
 * This task accumulates data in a static buffer. After new data arrives,
 * it scans the buffer for one or more complete packets ending in the
 * {EOF_BYTE_0, EOF_BYTE_1} sequence. Valid packets are extracted, queued
 * for processing, and removed from the buffer.
 */
static void uart_event_task(void *pvParameters) {
    uart_event_t event;
    
    // Persistent buffer to accumulate fragmented data
    static uint8_t temp_buffer[TEMP_BUFFER_SIZE];
    static size_t temp_buffer_len = 0;

    for (;;) {
        // Wait for a UART event
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            
            ActiveSerial->println("UART event triggered");

            switch (event.type) {
                case UART_PATTERN_DET: {

                    ActiveSerial->println("EOF1 detected");

                    // // Read all available new data from the hardware buffer
                    // uint8_t incoming_data[UART_RX_BUF_SIZE];
                    // size_t buffered_size;
                    // uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
                    // int read_len = uart_read_bytes(UART_NUM_0, incoming_data, buffered_size, pdMS_TO_TICKS(100));

                    // if (read_len > 0) {
                    //     // --- 1. Append new data, checking for overflow ---
                    //     if (temp_buffer_len + read_len > TEMP_BUFFER_SIZE) {
                    //         ActiveSerial->println("ERROR: UART temporary buffer overflow. Discarding all data.");
                    //         temp_buffer_len = 0; // Reset the buffer
                    //         break; 
                    //     }
                    //     memcpy(&temp_buffer[temp_buffer_len], incoming_data, read_len);
                    //     temp_buffer_len += read_len;

                    //     // --- 2. Scan buffer for complete packets and process them ---
                    //     if (temp_buffer[temp_buffer_len-2] == EOF_BYTE_0 && temp_buffer[temp_buffer_len-1] == EOF_BYTE_1) {
                            

                    //         // --- 3. Extract the packet and send it to the queue ---
                    //         UartPacket_t packet_to_send;
                    //         packet_to_send.len = temp_buffer_len;
                    //         memcpy(packet_to_send.data, temp_buffer, temp_buffer_len);
                    //         xQueueSend(serial_packet_queue, &packet_to_send, (TickType_t)0);

                    //         // --- 4. Remove the processed packet by shifting the buffer ---
                    //         size_t remaining_len = 0;//temp_buffer_len - packet_len;
                    //         temp_buffer_len = remaining_len;
                    //     }
                    //   }
                    }
                    break;
                

                // --- Error handling cases remain the same ---
                case UART_FIFO_OVF:
                    ActiveSerial->println("Hardware FIFO overflow");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart_queue);
                    temp_buffer_len = 0; // Also clear our temp buffer
                    break;

                case UART_BUFFER_FULL:
                    ActiveSerial->println("Ring buffer full");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart_queue);
                    temp_buffer_len = 0; // Also clear our temp buffer
                    break;
                
                default:
                    uart_flush_input(UART_NUM_0);
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}

#endif




void setup()
{
  DAP_config_st dap_config_st_local;
  DAP_config_st dap_config_st_eeprom;

  // setup serial
  // #define USE_CDC_INSTEAD_OF_UART
  #ifdef USE_CDC_INSTEAD_OF_UART
    Serial.begin(DEFAULTBAUD);
    //Serial.enableReboot(false);
    
    Serial.setTxTimeoutMs(100);
    ActiveSerial = &Serial;
    #ifdef USB_JOYSTICK
      ActiveSerial->println("Setup Controller");
      SetupController();
    #endif
  #elif CONFIG_IDF_TARGET_ESP32S3
    Serial1.begin(BAUD3M, SERIAL_8N1, 44, 43);
    // Serial.begin(BAUD3M, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial1;
  #elif CONFIG_IDF_TARGET_ESP32
    Serial.begin(DEFAULTBAUD);
    // Serial.begin(BAUD3M, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial;
  #endif





  // ADD THIS: Create the queue before creating the tasks that use it.
  // The queue can hold up to N state packages.
  pedalStateQueue = xQueueCreate(10, sizeof(PedalStatePackage_t));
  if (pedalStateQueue == NULL) {
      ActiveSerial->println("Error creating the pedal state queue!");
  }
  joystickDataQueue = xQueueCreate(1, sizeof(joystickDataPackage_t));
  if (joystickDataQueue == NULL) {
      ActiveSerial->println("Error creating the joystick data queue!");
  }
  loadcellDataQueue = xQueueCreate(1, sizeof(loadcellDataPackage_t));
  if (loadcellDataQueue == NULL) {
      ActiveSerial->println("Error creating the joystick data queue!");
  }
  configUpdateAvailableQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  if (configUpdateAvailableQueue == NULL) {
      ActiveSerial->println("Error creating the config data queue!");
  }
  configUpdateSendToPedalUpdateTaskQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  if (configUpdateSendToPedalUpdateTaskQueue == NULL) {
      ActiveSerial->println("Error creating the config data queue!");
  }
  configUpdateSendToLoadcellTaskQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  if (configUpdateSendToLoadcellTaskQueue == NULL) {
      ActiveSerial->println("Error creating the config data queue!");
  }
  // configUpdateSendToJoystickTaskQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  // if (configUpdateSendToJoystickTaskQueue == NULL) {
  //     ActiveSerial->println("Error creating the config data queue!");
  // }
  configUpdateSendToSerialRXTaskQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  if (configUpdateSendToSerialRXTaskQueue == NULL) {
      ActiveSerial->println("Error creating the config data queue!");
  }



  xTaskCreatePinnedToCore(
                    configHandlingTask,   /* Task function. */
                    "configHandlingTask",     /* name of task. */
                    3000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    TASK_PRIORITY_CONFIG_HANDLING_TASK,           /* priority of the task */
                    &handle_configHandlingTask,      /* Task handle to keep track of created task */
                    CORE_ID_CONFIG_HANDLING_TASK);          /* pin task to core 1 */  

  

  

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
  

  #ifdef USING_LED
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0xff,0xff);
    pixels.show(); 
  #endif

  Buzzer.initialized(BuzzerPin, 1);
  Buzzer.single_beep_tone(770, 100);


  parse_version(DAP_FIRMWARE_VERSION, &versionMajor, &versionMinor, &versionPatch);
  ActiveSerial->println(" ");
  ActiveSerial->println(" ");
  ActiveSerial->println(" ");
  //delay(3000);
  ActiveSerial->println("This work is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.");
  ActiveSerial->println("Please check github repo for more detail: https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal");
  //printout the github releasing version
  //#ifdef OTA_update
  ActiveSerial->print("Board: ");
  ActiveSerial->println(CONTROL_BOARD);
  ActiveSerial->print("Firmware Version:");
  ActiveSerial->println(DAP_FIRMWARE_VERSION);
  //#endif
  #ifdef PRINT_PARTITION_TABLE
    ActiveSerial->printf("========== Partition Table ==========\n");
    ActiveSerial->printf("| %-10s | %-4s | %-7s | %-8s | %-8s | %-5s |\n", "Name", "Type", "SubType", "Offset", "Size", "Encrypted");
    ActiveSerial->printf("--------------------------------------------------------------------------------\n");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it != NULL) {
        const esp_partition_t *part = esp_partition_get(it);
        ActiveSerial->printf("| %-10s | 0x%02x | 0x%02x    | 0x%08x | 0x%08x | %-5s |\n",
               part->label,      
               part->type,       
               part->subtype,    
               part->address,    
               part->size,       
               part->encrypted ? "true" : "false");
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    ActiveSerial->printf("=====================================\n");
  #endif
  
	#ifdef Hardware_Pairing_button
    pinMode(Pairing_GPIO, INPUT_PULLUP);
  #endif

  #ifdef USING_LED
    pixels.setPixelColor(0,0xff,0x00,0x00);
    pixels.show(); 
  #endif


  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(2048);
  global_dap_config_class.loadConfigFromEprom();
  global_dap_config_class.getConfig(&dap_config_st_eeprom, 500);


  // check validity of data from EEPROM  
  bool structChecker = true;
  uint16_t crc;
  if ( dap_config_st_eeprom.payLoadHeader_.payloadType != DAP_PAYLOAD_TYPE_CONFIG ){ 
    structChecker = false;
    /*ActiveSerial->print("Payload type expected: ");
    ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG);
    ActiveSerial->print(",   Payload type received: ");
    ActiveSerial->println(dap_config_st_local.payLoadHeader_.payloadType);*/
  }
  if ( dap_config_st_eeprom.payLoadHeader_.version != DAP_VERSION_CONFIG ){ 
    structChecker = false;
    /*ActiveSerial->print("Config version expected: ");
    ActiveSerial->print(DAP_VERSION_CONFIG);
    ActiveSerial->print(",   Config version received: ");
    ActiveSerial->println(dap_config_st_local.payLoadHeader_.version);*/
  }
  // checksum validation
  crc = checksumCalculator((uint8_t*)(&(dap_config_st_eeprom.payLoadHeader_)), sizeof(dap_config_st_eeprom.payLoadHeader_) + sizeof(dap_config_st_eeprom.payLoadPedalConfig_));
  if (crc != dap_config_st_eeprom.payloadFooter_.checkSum){ 
    structChecker = false;
    /*ActiveSerial->print("CRC expected: ");
    ActiveSerial->print(crc);
    ActiveSerial->print(",   CRC received: ");
    ActiveSerial->println(dap_config_st_local.payloadFooter_.checkSum);*/
  }






  // if checks are successfull, overwrite global configuration struct
  if (structChecker == true)
  {
    ActiveSerial->println("Updating pedal config from EEPROM");
    //global_dap_config_class.setConfig(dap_config_st_local);
    dap_config_st_local = dap_config_st_eeprom;
    configDataPackage_t configPackage_st;
    configPackage_st.config_st = dap_config_st_local;
    // xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);

  }
  else
  {

    ActiveSerial->println("Couldn't load config from EPROM due to mismatch: ");

    ActiveSerial->print("Payload type expected: ");
    ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG);
    ActiveSerial->print(",   Payload type received: ");
    ActiveSerial->println(dap_config_st_local.payLoadHeader_.payloadType);

    ActiveSerial->print("Target version: ");
    ActiveSerial->print(DAP_VERSION_CONFIG);
    ActiveSerial->print(",    Source version: ");
    ActiveSerial->println(dap_config_st_local.payLoadHeader_.version);

    ActiveSerial->print("CRC expected: ");
    ActiveSerial->print(crc);
    ActiveSerial->print(",   CRC received: ");
    ActiveSerial->println(dap_config_st_local.payloadFooter_.checkSum);
    //if the config check all failed, reinitialzie _config_st
    ActiveSerial->println("initialized config");
    global_dap_config_class.initializedConfig();
    global_dap_config_class.getConfig(&dap_config_st_local, 500);
  }

  ActiveSerial->println("Config sent successfully");
  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);

  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x5f,0x5f,0x00);//yellow
      pixels.show(); 
      //delay(3000);
  #endif

  bool invMotorDir = dap_config_st_local.payLoadPedalConfig_.invertMotorDirection_u8 > 0;
  stepper = new StepperWithLimits(stepPinStepper, dirPinStepper, invMotorDir, dap_calculationVariables_st.stepsPerMotorRevolution, dap_config_st_local.payLoadPedalConfig_.servoRatioOfInertia_u8); 

  motorRevolutionsPerSteps_fl32 = 1.0f / ( (float)dap_calculationVariables_st.stepsPerMotorRevolution );
  // ActiveSerial->printf("Steps per motor revolution: %d\n", dap_calculationVariables_st.stepsPerMotorRevolution);

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
	ActiveSerial->println("Start homing");
	stepper->findMinMaxSensorless(dap_config_st_local);
  ActiveSerial->print("Min Position is "); ActiveSerial->println(stepper->getLimitMin());
  ActiveSerial->print("Max Position is "); ActiveSerial->println(stepper->getLimitMax());


  // setup Kalman filters
  // ActiveSerial->print("Given loadcell variance: ");
  // ActiveSerial->println(loadcell->getVarianceEstimate(), 5);
  kalman = new KalmanFilter_1st_order(loadcell->getVarianceEstimate());
  kalman_joystick =new KalmanFilter_1st_order(0.1f);
  kalman_2nd_order = new KalmanFilter_2nd_order(loadcell->getVarianceEstimate());


  // LED signal 
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0, 0x80, 0x00, 0x80);//purple
      pixels.show(); 
      //delay(3000);
  #endif

  

  // equalize pedal config for both tasks
  global_dap_config_class.getConfig(&dap_config_st_local, 500);

  // send to config handling task
  xQueueSend(configUpdateAvailableQueue, &dap_config_st_local, portMAX_DELAY);


  // setup multi tasking
  semaphore_updatePedalStates = xSemaphoreCreateMutex();



  delay(10);

  // disableCore0WDT();
  // disableCore1WDT();

  ActiveSerial->println("Starting other tasks");

  // Register tasks
  addScheduledTask(pedalUpdateTask, "pedalUpdateTask", REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US, TASK_PRIORITY_PEDAL_UPDATE_TASK, CORE_ID_PEDAL_UPDATE_TASK, 7000);
  addScheduledTask(serialCommunicationTaskRx, "serComRx", REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US, TASK_PRIORITY_SERIALCOMMUNICATION_TASK, CORE_ID_SERIAL_COMMUNICATION_TASK, 6000);

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


	// the serialCommunicationTaskTx does not need a dedicated timer, since it triggered by queue 
  xTaskCreatePinnedToCore(
      serialCommunicationTaskTx,      /* Task function. */
      "serComTx",    /* name of task. */
      2000,                           /* Stack size of task */
      NULL,                           /* parameter of the task */
      TASK_PRIORITY_SERIALCOMMUNICATION_TX_TASK,                              /* priority of the task (e.g., 2, slightly higher than producer) */
      &handle_serialCommunicationTx,  /* Task handle */
      CORE_ID_SERIAL_COMMUNICATION_TASK); /* pin task to core */

  // the joystickOutputTask does not need a dedicated timer, since it triggered by queue 
#ifdef USB_JOYSTICK
  xTaskCreatePinnedToCore(
      joystickOutputTask,      /* Task function. */
      "joystickOutputTask",    /* name of task. */
      4000,                           /* Stack size of task */
      NULL,                           /* parameter of the task */
      TASK_PRIORITY_JOYSTICKOUTPUT_TASK,                              /* priority of the task (e.g., 2, slightly higher than producer) */
      &handle_joystickOutput,  /* Task handle */
      CORE_ID_JOYSTICK_TASK); /* pin task to core */
#endif
  // the loadcell task does not need a dedicated timer, since it blocks by DRDY ready ISR
  xTaskCreatePinnedToCore(
                    loadcellReadingTask,   /* Task function. */
                    "loadcellReadingTask",     /* name of task. */
                    1500,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    TASK_PRIORITY_LOADCELL_READING_TASK,           /* priority of the task */
                    &handle_loadcellReadingTask,      /* Task handle to keep track of created task */
                    CORE_ID_LOADCELLREADING_TASK);          /* pin task to core 1 */  

  // xTaskCreatePinnedToCore(
  //                   serialCommunicationTaskRx,   /* Task function. */
  //                   "serialCommunicationTaskRx",          /* name of task. */
  //                   5000,                      /* Stack size of task */
  //                   NULL,                      /* parameter of the task */
  //                   2,                         /* priority of the task */
  //                   &handle_serialCommunicationRx, /* Task handle to keep track of created task */
  //                   CORE_ID_SERIAL_COMMUNICATION_TASK); /* pin task to core */
	
  


xTaskCreatePinnedToCore(
                    profilerTask,   /* Task function. */
                    "profilerTask",     /* name of task. */
                    3000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    TASK_PRIORITY_PROFILER_TASK,           /* priority of the task */
                    &handle_profilerTask,      /* Task handle to keep track of created task */
                    CORE_ID_PROFILER_TASK);          /* pin task to core 1 */

  xTaskCreatePinnedToCore(
                    miscTask,   
                    "miscTask", 
                    2000,  
                    NULL,      
                    TASK_PRIORITY_MISC_TASK,         
                    &handle_miscTask,    
                    CORE_ID_MISC_TASK);     


                    
 
  #ifdef SERIAL_PATTERN_DETECTOR

    // --- ADD: Create the serialCommunicationTask as a standalone task ---
  // Create the queue to hold incoming serial packets
  serial_packet_queue = xQueueCreate(10, sizeof(UartPacket_t)); // Queue can hold 10 packets


  // This prevents the "UART driver already installed" error.
  uart_driver_delete(UART_NUM_0);
  
  // --- MODIFIED: Install driver over the existing UART0 ---
  // Note: This will reconfigure the port used by the Arduino `Serial` object.
  esp_err_t err = uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE * 2, 0, 20, &uart_queue, 0);
  if (err != ESP_OK) {
    ActiveSerial->printf("Failed to install UART driver: %d\n", err);
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
  #define SERIAL_PATTERN_DETECTION_TIMEOUT_IN_US 100
  uart_enable_pattern_det_baud_intr(UART_NUM_0, EOF_BYTE_1, 1, SERIAL_PATTERN_DETECTION_TIMEOUT_IN_US, 0, 0);

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
    addScheduledTask(otaUpdateTask, "OTATask", REPETITION_INTERVAL_OTA_TASK_IN_US, TASK_PRIORITY_OTA_TASK, CORE_ID_OTA_TASK, 16000);
    delay(200);
  #endif
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
      pixels.show(); 
      //delay(3000);
  #endif

  //print pedal role assignment
  if(dap_config_st_local.payLoadPedalConfig_.pedal_type!=PEDAL_ID_UNKNOWN)
  {
    ActiveSerial->print("Pedal Assignment: ");
    ActiveSerial->println(dap_config_st_local.payLoadPedalConfig_.pedal_type);
  }
  else
  {
    #ifdef PEDAL_HARDWARE_ASSIGNMENT
      ActiveSerial->println("Pedal Role Assignment:4, reading from CFG pins....");
    #endif
  }
  //enable software assignment reading
  #if defined(PEDAL_SOFTWARE_ASSIGNMENT) && defined(ESPNOW_Enable)
    ActiveSerial->println("Starting software assignment");
    softwareAssignmentInitialize();
    //overwrite the pedal type with device ID
    if (deviceIdStructChecker)
    {
      dap_config_st_local.payLoadPedalConfig_.pedal_type = dap_assignement_reg.deviceID;
    }
    else
    {
      dap_config_st_local.payLoadPedalConfig_.pedal_type = PEDAL_ID_UNKNOWN;
    }
  #endif
  #ifdef PEDAL_HARDWARE_ASSIGNMENT
    pinMode(CFG1, INPUT_PULLUP);
    pinMode(CFG2, INPUT_PULLUP);
    delay(50); // give the pin time to settle
    ActiveSerial->println("Overriding Pedal Role Assignment from Hardware switch......");
    uint8_t CFG1_reading = digitalRead(CFG1);
    uint8_t CFG2_reading = digitalRead(CFG2);
    uint8_t Pedal_assignment = CFG1_reading * 2 + CFG2_reading * 1; // 00=clutch 01=brk  02=gas
    if (Pedal_assignment == PEDAL_ID_ASSIGNMENT_ERROR)
    {
      ActiveSerial->println("Pedal Type:3, assignment error, please adjust dip switch on control board to finish role assignment.");
    }
    else
    {
      if (Pedal_assignment != PEDAL_ID_UNKNOWN)
      {
        // ActiveSerial->print("Pedal Type");
        // ActiveSerial->println(Pedal_assignment);
        if (Pedal_assignment == PEDAL_ID_CLUTCH)
          ActiveSerial->println("Overriding Pedal as Clutch.");
        if (Pedal_assignment == PEDAL_ID_BRAKE)
          ActiveSerial->println("Overriding Pedal as Brake.");
        if (Pedal_assignment == PEDAL_ID_THROTTLE)
          ActiveSerial->println("Overriding Pedal as Throttle.");
        DAP_config_st tmp;
        global_dap_config_class.getConfig(&tmp, 500);
        tmp.payLoadPedalConfig_.pedal_type = Pedal_assignment;
        dap_config_st_local.payLoadPedalConfig_.pedal_type = Pedal_assignment;
        // global_dap_config_class.setConfig(tmp);

        configDataPackage_t configPackage_st;
        configPackage_st.config_st = tmp;
        xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
        delay(1000); // delay for writting config into global
      }
      else
      {
        ActiveSerial->println("Asssignment error, defective pin connection, pelase connect USB and send a config to finish assignment");
      }
    }

  #endif

    //enable ESP-NOW
  #ifdef ESPNOW_Enable
    dap_calculationVariables_st.rudder_brake_status=false;
    ActiveSerial->println("Starting ESP now tasks");
    ESPNow_initialize();
    ActiveSerial->println("ESPNOW initialized, add task in");
    addScheduledTask(espNowCommunicationTaskTx, "ESPNOW_update_Task", REPETITION_INTERVAL_ESPNOW_TASK_IN_US, TASK_PRIORITY_ESPNOW_TASK, CORE_ID_ESPNOW_TASK, 10000);
    ActiveSerial->println("ESPNOW task added");
    delay(500);
  #endif

  #if defined(CONTROLLER_SPECIFIC_VIDPID) && defined(USB_JOYSTICK) && !defined(USE_CDC_INSTEAD_OF_UART)
      ActiveSerial->println("Setup Controller");
      SetupController_USB(dap_config_st_local.payLoadPedalConfig_.pedal_type);
      delay(500);
  #endif
  #ifdef ESPNOW_Enable
      //print out basic pedal info via espnow
      sendESPNOWLog("Pedal:%d DAP version: %d", dap_config_st_local.payLoadPedalConfig_.pedal_type, DAP_VERSION_CONFIG);
      delay(2);
      sendESPNOWLog("Pedal:%d Control Board: %s, Firmware: %s", dap_config_st_local.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, DAP_FIRMWARE_VERSION);
      delay(2);
      sendESPNOWLog("Pedal:%d Servo Voltage: %.0f V, Rail pitch set to %d mm.",dap_config_st_local.payLoadPedalConfig_.pedal_type, (float)stepper->getServosVoltage()/10.0f , dap_config_st_local.payLoadPedalConfig_.spindlePitch_mmPerRev_u8);
      delay(2);
      sendESPNOWLog("Pedal:%d Loadcell shifting: %.3f kg, Stdev: %.4f",dap_config_st_local.payLoadPedalConfig_.pedal_type, loadcell->getShiftingEstimate(),loadcell->getSTDEstimate());
      delay(2);
      sendESPNOWLog("Pedal:%d Min pos: %d, Max pos: %d, Current pos: %d",dap_config_st_local.payLoadPedalConfig_.pedal_type, stepper->getLimitMin(), stepper->getLimitMax(), stepper->getCurrentPosition());
      delay(2);
      sendESPNOWLog("Pedal:%d Setup end.",dap_config_st_local.payLoadPedalConfig_.pedal_type);
  #endif
  ActiveSerial->println("Setup end");
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0xff,0x00);//Green
      pixels.show(); 
      //delay(3000);
  #endif
  
  Buzzer.InitializedSound((int)dap_config_st_local.payLoadPedalConfig_.pedal_type);

}




/**********************************************************************************************/
/*                                                                                            */
/*                         Calc update function                                               */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters()
{
  DAP_config_st dap_config_st_local;  
  global_dap_config_class.getConfig(&dap_config_st_local, 500);

  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);
  dap_calculationVariables_st.updateEndstops(stepper->getLimitMin(), stepper->getLimitMax());
  stepper->updatePedalMinMaxPos(dap_config_st_local.payLoadPedalConfig_.pedalStartPosition, dap_config_st_local.payLoadPedalConfig_.pedalEndPosition);
  dap_calculationVariables_st.updateStiffness();

  // tune the PID settings
  // tunePidValues(dap_config_st_local);
}



/**********************************************************************************************/
/*                                                                                            */
/*                         Main function                                                      */
/*                                                                                            */
/**********************************************************************************************/

void printTaskStats() {
    // Static variables to persist between calls
    static TaskStatus_t *pxPreviousTaskArray = NULL;
    static uint32_t ulPreviousTotalRunTime = 0;
    static UBaseType_t uxPreviousArraySize = 0;

    TaskStatus_t *pxCurrentTaskArray;
    volatile UBaseType_t uxCurrentArraySize;
    uint32_t ulCurrentTotalRunTime;

    // Allocate memory for the current snapshot
    uxCurrentArraySize = uxTaskGetNumberOfTasks();
    pxCurrentTaskArray = (TaskStatus_t *)pvPortMalloc(uxCurrentArraySize * sizeof(TaskStatus_t));

    // Get the current system state
    if (pxCurrentTaskArray != NULL) {
        uxCurrentArraySize = uxTaskGetSystemState(pxCurrentTaskArray, uxCurrentArraySize, &ulCurrentTotalRunTime);

        // Check if this is the first run
        if (pxPreviousTaskArray != NULL) {
            // Calculate the time difference over the last second
            uint32_t ulTotalRunTimeDelta = ulCurrentTotalRunTime - ulPreviousTotalRunTime;

            if (ulTotalRunTimeDelta > 0) {
                ActiveSerial->println("\n--- Task CPU Usage (Last Second) ---");
                ActiveSerial->printf("%-25s %10s %15s %14s %30s\n", "Task", "Core ID", "Runtime [us]", "CPU %", "Free stack space [byte]");

                for (uint8_t coreIdx = 0; coreIdx < 2; coreIdx++)
                {

                  for (UBaseType_t i = 0; i < uxCurrentArraySize; i++) {

                      if (pxCurrentTaskArray[i].xCoreID == coreIdx)
                      {
                        // Find the matching task in the previous snapshot
                        for (UBaseType_t j = 0; j < uxPreviousArraySize; j++) {
                            if (pxCurrentTaskArray[i].xHandle == pxPreviousTaskArray[j].xHandle) {
                                uint32_t ulRunTimeDelta = pxCurrentTaskArray[i].ulRunTimeCounter - pxPreviousTaskArray[j].ulRunTimeCounter;
                                float cpuPercent = (100.0f * (float)ulRunTimeDelta) / (float)ulTotalRunTimeDelta;

                                ActiveSerial->printf("%-25s %10lu %15lu %14.2f %30lu\n",
                                  pxCurrentTaskArray[i].pcTaskName,
                                  pxCurrentTaskArray[i].xCoreID,
                                  (unsigned long)ulRunTimeDelta,
                                  cpuPercent,
                                  pxCurrentTaskArray[i].usStackHighWaterMark);
                                break;
                            }
                        }
                      }
                  }


                }

                
                ActiveSerial->println("-----------------------\n");
            }
        }

        // Free the previous snapshot and save the current one for the next cycle
        if (pxPreviousTaskArray != NULL) {
            vPortFree(pxPreviousTaskArray);
        }
        pxPreviousTaskArray = pxCurrentTaskArray;
        ulPreviousTotalRunTime = ulCurrentTotalRunTime;
        uxPreviousArraySize = uxCurrentArraySize;
    } else {
        ActiveSerial->println("Failed to allocate memory for task stats.");
    }
}

void profilerTask( void * pvParameters )
{
  for(;;){
    // copy global struct to local for faster and safe executiion
    DAP_config_st dap_config_profilerTask_st;
    global_dap_config_class.getConfig(&dap_config_profilerTask_st, 500);

    // activate profiler depending on pedal config
    if (dap_config_profilerTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_NET_RUNTIME) 
    {
      printTaskStats();
    }


    delay(5000);
    taskYIELD();
  }
}


void loop() {
  // vTaskDelete(NULL);  // Kill the Arduino loop task

  delay(5000);
  taskYIELD();
}




/**********************************************************************************************/
/*                                                                                            */
/*                         pedal update task                                                  */
/*                                                                                            */
/**********************************************************************************************/
//#define ESPNow_debug_rudder

#ifdef ESPNow_debug_rudder
  unsigned long debugMessageLast=0;
#endif 

void IRAM_ATTR_FLAG pedalUpdateTask( void * pvParameters )
{

  static DRAM_ATTR DAP_state_extended_st dap_state_extended_st_lcl_pedalUpdateTask;
  static DRAM_ATTR DAP_state_basic_st dap_state_basic_st_lcl_pedalUpdateTask;
  static DRAM_ATTR DAP_config_st dap_config_pedalUpdateTask_st;

  static loadcellDataPackage_t loadcellDataReceived_st;
  static configDataPackage_t configPackage_st;

  FunctionProfiler profiler_pedalUpdateTask;
  profiler_pedalUpdateTask.setName("PedalUpdate");

  static DRAM_ATTR float loadcellReading = 0.0f;
  float filteredReading_exp_filter = 0.0f;
  static DRAM_ATTR float filteredReading = 0.0f;

  unsigned long servoActionLast = millis();
  

  uint32_t controlTask_stackSizeIdx_u32 = 0;
  float previousLoadcellReadingInKg_fl32 = 0.0f;


  float effect_force_fl32;
  float effect_pos_fl32;
  int32_t Position_effect;
  int32_t BP_trigger_value;
  int32_t BP_trigger_min;
  int32_t BP_trigger_max;
  int32_t Position_check;
  int32_t Rudder_real_poisiton;
  float joystickNormalizedToInt32_orig;
  float joystickfrac;
  float joystickNormalizedToInt32_eval;
  uint16_t joystickNormalizedToUInt16 = 0;
  int32_t ABS_trigger_value;

  uint8_t sendPedalStructsViaSerialCounter_u8 = 0;
  uint8_t sendJoystickDataCounter_u8 = 0;

  
  global_dap_config_class.getConfig(&dap_config_pedalUpdateTask_st, 500);

  static const uint8_t joystickSendCounterMax_u8 = (REPETITION_INTERVAL_JOYSTICKOUTPUT_TASK_IN_US) / (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US) ;
  static const uint8_t serialSendCounterMax_u8 = (REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US) / (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US) ;

  
  

  
  static float changeVelocity = 0.0f;
  static float normalizedPedalReading_fl32 = 0.0f;
  static float stepperPosFraction_fl32 = 0.0f;
  static uint16_t angleReading_ui16 = 0;
  static bool sendBasicFlag_b = false;
  static bool sendExtendedFlag_b = false;
  static int32_t stepperPosCurrent_i32;
  static uint32_t cycleCount_u32 = 0;

  for(;;){


    // trigger task 
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

      // if new data package is available, update the local config
      if (xQueueReceive(configUpdateSendToPedalUpdateTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS) {
        dap_config_pedalUpdateTask_st = configPackage_st.config_st;

        // activate profiler depending on pedal config
        if (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        {
          profiler_pedalUpdateTask.activate( true );
        }
        else
        {
          profiler_pedalUpdateTask.activate( false );
        }

        ActiveSerial->println("Update config: pedal update task");



        // update the calc params
        ActiveSerial->println("Updating the calc params");
        //ActiveSerial->print("save to eeprom tag:");
        //ActiveSerial->println(dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom);
        saveToEEPRomDuration = millis();
        
        if (true == dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom)
        {
          dap_config_pedalUpdateTask_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
          uint16_t crc = checksumCalculator((uint8_t*)(&(dap_config_pedalUpdateTask_st.payLoadHeader_)), sizeof(dap_config_pedalUpdateTask_st.payLoadHeader_) + sizeof(dap_config_pedalUpdateTask_st.payLoadPedalConfig_));
          dap_config_pedalUpdateTask_st.payloadFooter_.checkSum = crc;
          global_dap_config_class.setConfig(dap_config_pedalUpdateTask_st);
          ActiveSerial->println("Saving into EEPROM");
          global_dap_config_class.storeConfigToEprom();
          
          saveToEEPRomDuration = 0;
        }
        
        updatePedalCalcParameters(); // update the calc parameters
        moveSlowlyToPosition_b = true;

        // enable/disable step loss recovery and crash
        stepper->configSteplossRecovAndCrashDetection(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.stepLossFunctionFlags_u8);

        // set position command smoothing
        stepper->configSetPositionCommandSmoothingFactor(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.positionSmoothingFactor_u8);
        stepper->configSetProfilingFlag( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) );
        stepper->configSetRatioOfInertia(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.servoRatioOfInertia_u8);

        // reset all servo alarms
        if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS) )
        {
          ActiveSerial->println("Set clear alarm history flag");
          stepper->clearAllServoAlarms();
          delay(1000); // makes sure the routine has finished

          DAP_config_st tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS); // clear the debug bit
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
        }

        // reset all servo alarms
        if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_RESET_SERVO_TO_FACTORY) )
        {
          DAP_config_st tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_RESET_SERVO_TO_FACTORY); // clear the debug bit
          tmp.payLoadHeader_.storeToEeprom = 1;
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);

          delay(500);

          global_dap_config_class.storeConfigToEprom();

          ActiveSerial->println("Resetting servo parameters to factory values");
          stepper->resetServoParametersToFactoryValues();
        }

        

        // print all servo parameters for debug purposes
        if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS) )
        {
          DAP_config_st tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payLoadPedalConfig_.debug_flags_0 &= ( ~(uint8_t)DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS); // clear the debug bit
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);

          delay(1000);  
          stepper->printAllServoParameters();
        }

        // ActiveSerial->printf("Abs ampl.: %0.3f\n", (float)dap_calculationVariables_st.absAmplitude);
        // ActiveSerial->printf("force range.: %0.3f\n", (float)dap_calculationVariables_st.Force_Range);
        // ActiveSerial->printf("pos range: %0.3f\n", (float)dap_calculationVariables_st.stepperPosRange);
        
        //bitepoint trigger
        BP_trigger_value = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.BP_trigger_value;
        BP_trigger_min = (BP_trigger_value-4);
        BP_trigger_max = (BP_trigger_value+4);

      }


      // start profiler 0, overall function
      profiler_pedalUpdateTask.start(0);
      


      cycleCount_u32++;

      // get current position
      stepperPosFraction_fl32 = stepper->getCurrentPositionFraction();
      stepperPosCurrent_i32 = stepper->getCurrentPosition();

      // system identification mode
      #ifdef ALLOW_SYSTEM_IDENTIFICATION
        if (systemIdentificationMode_b == true)
        {
          measureStepResponse(stepper, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, loadcell);
          systemIdentificationMode_b = false;
        }
      #endif
    

      
      //#define RECALIBRATE_POSITION
      #ifdef RECALIBRATE_POSITION
        stepper->checkLimitsAndResetIfNecessary();
      #endif



      // start profiler 1, effects
      profiler_pedalUpdateTask.start(1);

      // bitepoint helper
      Position_check = (int32_t)(stepperPosFraction_fl32 * 100.0f);

      // compute pedal oscillation, when ABS is active
      dap_calculationVariables_st.Default_pos();

      // trigger and compute effects
      absOscillation.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.absPattern, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.absForceOrTarvelBit);
      _RPMOscillation.trigger();
      _RPMOscillation.forceOffset(&dap_calculationVariables_st);
      _BitePointOscillation.forceOffset(&dap_calculationVariables_st);
      _G_force_effect.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.G_multi);
      _WSOscillation.forceOffset(&dap_calculationVariables_st);
      _Road_impact_effect.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Road_multi);
      CV1.forceOffset(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_freq_1,dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_amp_1,dap_calculationVariables_st.stepperPosRange);
      CV2.forceOffset(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_freq_2,dap_config_pedalUpdateTask_st.payLoadPedalConfig_.CV_amp_2,dap_calculationVariables_st.stepperPosRange);
      
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
      

      if(dap_calculationVariables_st.Rudder_status) 
      {
        _rudder_g_force.offset_calculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.update_stepperMaxpos(_rudder_g_force.offset_filter);
        _rudder.offset_calculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.update_stepperMinpos(_rudder.offset_filter);

      }
      if(dap_calculationVariables_st.helicopterRudderStatus) 
      {
        helicopterRudder_.offset_calculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.update_stepperMinpos(helicopterRudder_.offset_filter);
      }
      #ifdef ESPNow_debug_rudder
      if(dap_calculationVariables_st.Rudder_status || dap_calculationVariables_st.helicopterRudderStatus)
      {
        if(millis()-debugMessageLast>500 )
        {
          debugMessageLast=millis();
          ActiveSerial->print("Center offset:");
          ActiveSerial->println(_rudder.offset_filter);
          ActiveSerial->print("pos min:");
          ActiveSerial->println(dap_calculationVariables_st.stepperPosMin);
          ActiveSerial->print("pos max:");
          ActiveSerial->println(dap_calculationVariables_st.stepperPosMax);
          ActiveSerial->print("max Force:");
          ActiveSerial->print("max Force:");
          ActiveSerial->print(dap_calculationVariables_st.Force_Max);
          ActiveSerial->print(" min Force:");
          ActiveSerial->println(dap_calculationVariables_st.Force_Min);
          ActiveSerial->print("Force:");
          ActiveSerial->println(filteredReading);
        }
      }
      #endif

      //_rudder.force_offset_calculate(&dap_calculationVariables_st);


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

      // read loadcell data, when available
      profiler_pedalUpdateTask.start(2);
      if (xQueueReceive(loadcellDataQueue, &loadcellDataReceived_st, (TickType_t)0) == pdPASS) {
        loadcellReading = loadcellDataReceived_st.loadcellReadingInKg_fl32;
      }
      profiler_pedalUpdateTask.end(2);



      
  #ifdef ANGLE_SENSOR_GPIO
        angleReading_ui16 = analogRead(ANGLE_SENSOR_GPIO);
  #endif

      // Get the angle measurement reading
      // float angleReading = loadcell->getAngleMeasurement();
    

      // start profiler 3, loadcell reading conversion
      profiler_pedalUpdateTask.start(3);

      // Convert loadcell reading to pedal force
      float sledPosition = sledPositionInMM(stepper, &dap_config_pedalUpdateTask_st, motorRevolutionsPerSteps_fl32);
      float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, &dap_config_pedalUpdateTask_st);
      float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
      float d_phi_d_x = convertToPedalForceGain(sledPosition, &dap_config_pedalUpdateTask_st);

      // compute gain for horizontal foot model
      float b = (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_b;
      float d = (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.lengthPedal_d;
      float d_x_hor_d_phi = -(float)(b+d) * isin(pedalInclineAngleInDeg_fl32);
      d_x_hor_d_phi *= DEG_TO_RAD_FL32; // inner derivative

      // start profiler 3, loadcell reading conversion
      profiler_pedalUpdateTask.end(3);

      // start profiler 4, loadcell reading filtering
      profiler_pedalUpdateTask.start(4);
      
      // Do the loadcell signal filtering
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
        Buzzer.single_beep_tone(770, 100);
        delay(300);
        Buzzer.single_beep_tone(770, 100);
        ActiveSerial->println("Wake up servo, restart esp.");
        delay(1000);
        ESP.restart();
      }

      // pedal not in action, disable pedal power
      uint32_t pedalIdleTimout = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.servoIdleTimeout * 60 * 1000; // timeout in ms
      if ((stepper->servoStatus == SERVO_CONNECTED) && ((millis() - servoActionLast) > pedalIdleTimout) && (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.servoIdleTimeout != 0))
      {
        stepper->servoIdleAction();
        stepper->servoStatus = SERVO_IDLE_NOT_CONNECTED;
        Buzzer.single_beep_tone(770, 100);
        delay(300);
        #ifdef USING_LED
          pixels.setPixelColor(0, 0xff, 0x00, 0x00); // show red
          pixels.show();
        #endif
        Buzzer.single_beep_tone(770, 100);
        ActiveSerial->println("Servo idle timeout reached. To restart pedal, please apply pressure.");
      }
      //emergency button

      #ifdef EMERGENCY_PIN
        if ((stepper->servoStatus == SERVO_CONNECTED) && (stepper->servoStatus != SERVO_FORCE_STOP) && (digitalRead(EMERGENCY_PIN) == LOW))
        {
          stepper->servoIdleAction();
          stepper->servoStatus = SERVO_FORCE_STOP;
          Buzzer.single_beep_tone(770, 100);
          delay(300);
          #ifdef USING_LED
            pixels.setPixelColor(0, 0xff, 0x00, 0x00); // show red
            pixels.show();
          #endif
          Buzzer.single_beep_tone(770, 100);
          ActiveSerial->println("Servo force Stoped.");
        }
      #endif
      //float FilterReadingJoystick=averagefilter_joystick.process(filteredReading);


      // start profiler 4, movement strategy
      profiler_pedalUpdateTask.start(5);

      

      int32_t Position_Next = 0;

      // Adding effects
      effect_pos_fl32 = 0.0f;
      effect_force_fl32 = 0.0f;
      bool effectsCalculated_b=false;
      // only add force when not at min position 
      if(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.minForceForEffects_u8!=0)
      {
        if(filteredReading >= (float)dap_config_pedalUpdateTask_st.payLoadPedalConfig_.minForceForEffects_u8)
        {
          effectsCalculated_b = true;
        }
      }
      else
      {
        // accumulate force offsets
        effectsCalculated_b=true;
        //effect_pos_fl32 += _RPMOscillation.RPM_position_offset;
      }
      if(effectsCalculated_b)
      {
        effect_force_fl32 += absOscillation.absOscillation_Force_offset;
        // accumulate position offsets
        effect_pos_fl32 += absOscillation.absOscillation_Position_offset;
        effect_pos_fl32 += _WSOscillation.WheelSlipOffset;
        effect_pos_fl32 += _BitePointOscillation.BitePointOffset; 
        effect_pos_fl32 += CV1.CustomVibrationOffset;
        effect_pos_fl32 += CV2.CustomVibrationOffset;
        effect_pos_fl32 += _RPMOscillation.RPM_position_offset;
      }

      // add dampening
      if (dap_calculationVariables_st.dampingPress  > 0.0001f)
      {
        // dampening is proportional to velocity --> D-gain for stability
        effect_pos_fl32 -= dap_calculationVariables_st.dampingPress * changeVelocity * dap_calculationVariables_st.springStiffnesssInv;
      }



      // Due to closed loop frequiency response, the servo might travel less than the input amplitude. 
      // To compensate the attenuation, the input amplitude is scaled by a constant factor, which was identified for 15Hz input frequency and roughly resulted in plausible amplitude range. 
      effect_force_fl32 *= EFFECT_SCALING_FACTOR_FL32;
      effect_pos_fl32 *= EFFECT_POSITION_SCALING_FACTOR_FL32;
      


      // chatter reduction gain, reduce the gain when chatter happened
      /*
      if (chatterReduction.checkForChatter(stepperPosCurrent_i32, esp_timer_get_time(), false))
      {
        effect_pos_fl32 *= chatterReduction.DynamicEffectGain();
      }
      */

      
      // compute next position with PID strategy
      // MPC control strategy for rudder
      int32_t positionWithoutEffect=0;
      if(dap_calculationVariables_st.Rudder_status || dap_calculationVariables_st.helicopterRudderStatus)
      {
        // Rudder only
        Position_Next = MoveByForceTargetingStrategy(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0.0f/*effect_force*/, changeVelocity, d_phi_d_x, d_x_hor_d_phi);
        positionWithoutEffect=Position_Next;//send the value without rpm effect
        if(effectsCalculated_b)
        {
          //float effectsOffsetFiltered= effectOffsetPID.computeEffectOffset(effect_pos_fl32, &dap_calculationVariables_st);
          //Position_Next -= effectsOffsetFiltered;
          Position_Next -= effect_pos_fl32;
        } 
        //if(effectsCalculated_b) Position_Next -= _RPMOscillation.RPM_position_offset;
      }
      else 
      {
        // Pedal control
        Position_Next = MoveByPidStrategy(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, effect_force_fl32, 0);
        if(effectsCalculated_b)
        {
          Position_Next -= effect_pos_fl32;
        } 
      }
      // end profiler 4, movement strategy
      profiler_pedalUpdateTask.end(5);

      // start profiler 6, ...
      profiler_pedalUpdateTask.start(6);

      
      // clip target position to configured target interval with RPM effect movement in the endstop
      Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMinEndstop, dap_calculationVariables_st.stepperPosMaxEndstop);
      //Position_Next = (int32_t)constrain(Position_Next, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax);

      

      // rudder helper
      Rudder_real_poisiton= 100*((positionWithoutEffect-dap_calculationVariables_st.stepperPosMin_default) / dap_calculationVariables_st.stepperPosRange_default);

      dap_calculationVariables_st.current_pedal_position = positionWithoutEffect;
      dap_calculationVariables_st.current_pedal_position_ratio=((float)(dap_calculationVariables_st.current_pedal_position-dap_calculationVariables_st.stepperPosMin_default))/((float)dap_calculationVariables_st.stepperPosRange_default);
      //Rudder initialzing and de initializing
      #ifdef ESPNOW_Enable
        if(dap_calculationVariables_st.Rudder_status)
        {
          if(Rudder_initializing)
          {
            moveSlowlyToPosition_b=true;
            //ActiveSerial->println("moving to center");
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
              //ActiveSerial->print("Rudder initializing...");
              //ActiveSerial->println(Rudder_initialzing_time_Now-Rudder_initialized_time);
              if( (Rudder_initialzing_time_Now-Rudder_initialized_time)> Rudder_timeout )
              {
                Rudder_initializing=false;
                moveSlowlyToPosition_b=false;
                ActiveSerial->println("Rudder initialized");
                dap_calculationVariables_st.isRudderInitialized=true;
                Rudder_initialized_time=0;
                Buzzer.play_melody_tone(melody_Airship_theme, sizeof(melody_Airship_theme)/sizeof(melody_Airship_theme[0]),melody_Airship_theme_duration);
              }
            }
            

          }
        }
        if(Rudder_deinitializing)
        {
          moveSlowlyToPosition_b=true;
          //ActiveSerial->println("moving to min end stop");
        }
        if(Rudder_deinitializing && (Rudder_real_poisiton< 2 ))
        {
          Rudder_deinitializing=false;
          moveSlowlyToPosition_b=false;
          ActiveSerial->println("Rudder deinitialized");
          dap_calculationVariables_st.isRudderInitialized=false;
        }
        //helicopter rudder initialzied
        if(dap_calculationVariables_st.helicopterRudderStatus)
        {
          if(HeliRudder_initializing)
          {
            moveSlowlyToPosition_b=true;
            //ActiveSerial->println("moving to center");
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
              //ActiveSerial->print("Rudder initializing...");
              //ActiveSerial->println(Rudder_initialzing_time_Now-Rudder_initialized_time);
              if( (Rudder_initialzing_time_Now-Rudder_initialized_time)> Rudder_timeout )
              {
                HeliRudder_initializing=false;
                moveSlowlyToPosition_b=false;
                ActiveSerial->println("HeliRudder initialized");
                dap_calculationVariables_st.isHelicopterRudderInitialized=true;
                Rudder_initialized_time=0;
                Buzzer.play_melody_tone(melodyAirwolfTheme, sizeof(melodyAirwolfTheme)/sizeof(melodyAirwolfTheme[0]),melodyAirwolfThemeDuration);
              }
            }
          }
        }
        if(HeliRudder_deinitializing)
        {
          moveSlowlyToPosition_b=true;
            //ActiveSerial->println("moving to min end stop");
        }
        if(HeliRudder_deinitializing && (Rudder_real_poisiton< 2 ))
        {
          HeliRudder_deinitializing=false;
          moveSlowlyToPosition_b=false;
          ActiveSerial->println("HeliRudder deinitialized");
          dap_calculationVariables_st.isHelicopterRudderInitialized=false;
        }
      #endif

      

      // if pedal in min position, recalibrate position --> automatic step loss compensation
      // stepper->configSteplossRecovAndCrashDetection(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.stepLossFunctionFlags_u8);
      if( (stepper->getLifelineSignal()==true) && (stepper->servoStatus==SERVO_CONNECTED) )
      {
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
      }
      

      


      // stop movement, when OTA is in progres
      bool doMovement_b = true;
      #if defined(OTA_update_ESP32) || defined(OTA_update)
        if(OTA_status==true)
        {
          doMovement_b = false;
        } 
      #endif

      // Move to new position
      if (doMovement_b)
      {
        if (!moveSlowlyToPosition_b)
        {
          stepper->moveTo(Position_Next, false);
        }
        else
        {
          moveSlowlyToPosition_b = false;
          stepper->moveSlowlyToPos(Position_Next);
        }
      }
    
      // compute controller output
      dap_calculationVariables_st.StepperPos_setback();
      dap_calculationVariables_st.reset_maxforce();
      dap_calculationVariables_st.dynamic_update();
      dap_calculationVariables_st.updateStiffness();
      

      // compute joystick value
      if(dap_calculationVariables_st.Rudder_status&&dap_calculationVariables_st.rudder_brake_status)
      {
        if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((stepperPosCurrent_i32-dap_calculationVariables_st.stepperPosRange/2), dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMin+dap_calculationVariables_st.stepperPosRange/2.0f, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
        }
        else
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((FilterReadingJoystick/*filteredReading*/), dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
        }
      }
      else
      {
        if (1 == dap_config_pedalUpdateTask_st.payLoadPedalConfig_.travelAsJoystickOutput_u8)
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(stepperPosCurrent_i32, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
        }
        else
        {            
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(FilterReadingJoystick/*filteredReading*/, dap_calculationVariables_st.Force_Min, dap_calculationVariables_st.Force_Max, dap_config_pedalUpdateTask_st.payLoadPedalConfig_.maxGameOutput);
        }
      }
      joystickfrac=(float)joystickNormalizedToInt32_orig/(float)JOYSTICK_MAX_VALUE;
      joystickNormalizedToInt32_eval = forceCurve.EvalJoystickCubicSpline(&dap_config_pedalUpdateTask_st, &dap_calculationVariables_st, joystickfrac);
      
      joystickNormalizedToUInt16 = joystickNormalizedToInt32_eval/100.0f* JOYSTICK_MAX_VALUE;
      joystickNormalizedToUInt16 = constrain(joystickNormalizedToUInt16, JOYSTICK_MIN_VALUE, JOYSTICK_MAX_VALUE);      


      // send joystick data to queue
      if (joystickDataQueue != NULL) {

        // send data every N-th frame
        sendJoystickDataCounter_u8++;
        sendJoystickDataCounter_u8 %= joystickSendCounterMax_u8;

        if (sendJoystickDataCounter_u8 == 0)
        {
          // Package the new state data into a single struct
          joystickDataPackage_t newJoystickPackage;
          newJoystickPackage.sendJoystickFlag_b = true;
          newJoystickPackage.joystickNormalizedToUInt16 = joystickNormalizedToUInt16;

          // Send the package to the queue. Use a timeout of 0 (non-blocking).
          // If the queue is full, the data is simply dropped. This prevents this
          // high-priority control task from ever blocking on a full serial buffer.
          xQueueSend(joystickDataQueue, &newJoystickPackage, (TickType_t)0);
        }
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

      
      
      if ( fabs(dap_calculationVariables_st.Force_Range) > 0.01f)
      {
          normalizedPedalReading_fl32 = constrain((filteredReading - dap_calculationVariables_st.Force_Min) / dap_calculationVariables_st.Force_Range, 0.0f, 1.0f);
      }
      
      // simulate ABS trigger 
      if(dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Simulate_ABS_trigger==1)
      {
        ABS_trigger_value=dap_config_pedalUpdateTask_st.payLoadPedalConfig_.Simulate_ABS_value;
        if( (normalizedPedalReading_fl32*100.0f) > ABS_trigger_value)
        {
          absOscillation.trigger();
        }
      }

      // end profiler 6, ...
      profiler_pedalUpdateTask.end(6);

      // start profiler 8, struct exchange
      profiler_pedalUpdateTask.start(7);
      

  
      
      // update pedal states
      // check if data needs to be send
      if ( (dap_config_pedalUpdateTask_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT) )
      {
        // send data every frame
        sendPedalStructsViaSerialCounter_u8 = 0;
        sendBasicFlag_b = true;
        sendExtendedFlag_b = true;
      }
      else
      {
        // send data every N-th frame
        sendPedalStructsViaSerialCounter_u8++;
        sendPedalStructsViaSerialCounter_u8 %= serialSendCounterMax_u8;
        sendBasicFlag_b = true;
        sendExtendedFlag_b = false;
      }

      if (pedalStateQueue != NULL) {


        if (sendBasicFlag_b)
        {
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalForce_u16 =  normalizedPedalReading_fl32 * 65535.0f;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalPosition_u16 = constrain(stepperPosFraction_fl32, 0.0f, 1.0f) * 65535.0f;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.joystickOutput_u16 = joystickNormalizedToUInt16;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[0] = versionMajor;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[1] = versionMinor;  
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalFirmwareVersion_u8[2] = versionPatch;
          //error code
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.error_code_u8 = 0;
          //pedal status update
          if(dap_calculationVariables_st.Rudder_status)
          {
            if(dap_calculationVariables_st.rudder_brake_status) dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalStatus=PEDAL_STATUS_RUDDERBRAKE;
            else dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalStatus=PEDAL_STATUS_RUDDER;
          }
          else dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.pedalStatus=PEDAL_STATUS_NORMAL;
          //servo status update
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.servoStatus=stepper->servoStatus;

          #ifdef ESPNOW_Enable
            if(ESPNow_error_code!=0)
            {
              dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.error_code_u8=ESPNow_error_code;
              ESPNow_error_code=0;
            }
          #endif

          if( (stepper->getLifelineSignal()==false) && (stepper->servoStatus!=SERVO_IDLE_NOT_CONNECTED) )
          {
            dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalState_Basic_.error_code_u8=12;
          }
          
          //fill the header
          dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
          dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;

          dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_BASIC;
          dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.version = DAP_VERSION_CONFIG;
          dap_state_basic_st_lcl_pedalUpdateTask.payLoadHeader_.PedalTag = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;        
          
        }

        if (sendExtendedFlag_b)
        {
          // update extended pedal structures
          dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0; // 170
          dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1; // 85

          dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame0_u8 =  EOF_BYTE_0; // 170
          dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_.enfOfFrame1_u8 =  EOF_BYTE_1; // 86

          // update extended struct 
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.timeInUs_u32 = micros();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.cycleCount_u32 = cycleCount_u32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.pedalForce_raw_fl32 =  loadcellReading;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.pedalForce_filtered_fl32 =  filteredReading;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.forceVel_est_fl32 =  changeVelocity;

          int32_t minPos = 0; //stepper->getMinPosition();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPosition_i32 = stepper->getServosInternalPositionCorrected() - minPos;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_voltage_0p1V =  stepper->getServosVoltage();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_current_percent_i16 = stepper->getServosCurrent();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servo_position_error_i16 = stepper->getServosPosError();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPositionEstimated_i16 = stepper->getEstimatedPosError();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.targetPosition_i32 = Position_Next - minPos;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.servoPositionTarget_i32 = stepper->getCurrentPosition() - minPos;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.currentSpeedInMilliHz_i32 = stepper->getCurrentSpeedInMilliHz();

          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.angleSensorOutput_ui16 = angleReading_ui16;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalState_Extended_.brakeResistorState_b = stepper->getBrakeResistorState();
          dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.PedalTag = dap_config_pedalUpdateTask_st.payLoadPedalConfig_.pedal_type;
          dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_STATE_EXTENDED;
          dap_state_extended_st_lcl_pedalUpdateTask.payLoadHeader_.version = DAP_VERSION_CONFIG;
        }



        if (sendPedalStructsViaSerialCounter_u8 == 0)
        {
          // Package the new state data into a single struct
          PedalStatePackage_t newStatePackage;
          newStatePackage.basic_st = dap_state_basic_st_lcl_pedalUpdateTask;
          newStatePackage.extended_st = dap_state_extended_st_lcl_pedalUpdateTask;
          newStatePackage.sendBasicFlag_b = sendBasicFlag_b;
          newStatePackage.sendExtendedFlag_b = sendExtendedFlag_b;

          // Send the package to the queue. Use a timeout of 0 (non-blocking).
          // If the queue is full, the data is simply dropped. This prevents this
          // high-priority control task from ever blocking on a full serial buffer.
          xQueueSend(pedalStateQueue, &newStatePackage, (TickType_t)0);
        }
      }

      profiler_pedalUpdateTask.end(7);
      profiler_pedalUpdateTask.end(0);
    }
  }
}

  






/**********************************************************************************************/
/*                                                                                            */
/*                         joystick output task                                               */
/*                                                                                            */
/**********************************************************************************************/

#ifdef USB_JOYSTICK
void IRAM_ATTR_FLAG joystickOutputTask( void * pvParameters )
{ 

  // FunctionProfiler profiler_joystickOutputTask;
  // profiler_joystickOutputTask.setName("JoystickOutput");
  // profiler_joystickOutputTask.setNumberOfCalls(500);
  // configDataPackage_t configPackage_st;
  // static DAP_config_st jut_dap_config_st;
  

  // This task now waits for a complete package of data from the queue.
  joystickDataPackage_t receivedJoystickData;

  
  for(;;){

    // if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {
    if (xQueueReceive(joystickDataQueue, &receivedJoystickData, portMAX_DELAY) == pdPASS) {


      // start profiler 0, overall function
      // profiler_joystickOutputTask.start(0);
      // // if new data package is available, update the local config
      // if (xQueueReceive(configUpdateSendToJoystickTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS) {
      //   jut_dap_config_st = configPackage_st.config_st;
      //   // activate profiler depending on pedal config
      //   if (jut_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
      //   {
      //     profiler_joystickOutputTask.activate( true );
      //   }
      //   else
      //   {
      //     profiler_joystickOutputTask.activate( false );
      //   }

      //   ActiveSerial->println("Update config: joystick task");
      // } 


      
      // profiler_joystickOutputTask.start(1);

      uint16_t joystickData_u16 = receivedJoystickData.joystickNormalizedToUInt16;
      bool sendFlag_b = receivedJoystickData.sendJoystickFlag_b;

      if (sendFlag_b)
      {
        // send joystick output
        if (IsControllerReady()) 
        {
          if(dap_calculationVariables_st.Rudder_status==false)
          {
            //general output
            // ActiveSerial->printf("joystick: %lu\n", joystickData_u16);
            SetControllerOutputValue(joystickData_u16);
          }
        }
      }

      // profiler_joystickOutputTask.end(1);
      // profiler_joystickOutputTask.end(0);

      // // print profiler results
      // profiler_joystickOutputTask.report();

    }
  }
}
#endif
/**********************************************************************************************/
/*                                                                                            */
/*                         communication task                                                 */
/*                                                                                            */
/**********************************************************************************************/

typedef struct {
    uint16_t startBytePos_u16;
    uint16_t endBytePos_u16;
    uint16_t payloadType_u16;
    bool validFlag_b;
} structChecker_st;

// Helper function to determine expected packet size from payload type
// Returns 0 if the payload type is unknown.
static inline size_t getExpectedPacketSize(uint8_t payloadType) {
    switch (payloadType) {
        case DAP_PAYLOAD_TYPE_CONFIG:
            return sizeof(DAP_config_st);
        case DAP_PAYLOAD_TYPE_ACTION:
            return sizeof(DAP_actions_st);
        case DAP_PAYLOAD_TYPE_ACTION_OTA:
            return sizeof(DAP_action_ota_st);
        // Add other packet types here in the future
        default:
            return 0;
    }
}

// NOTE: The IRAM_ATTR attribute has been removed as it is not needed for a FreeRTOS task function.
void IRAM_ATTR_FLAG serialCommunicationTaskRx(void *pvParameters) {
    FunctionProfiler profiler_serialCommunicationTask;
    profiler_serialCommunicationTask.setName("SerialCommunicationRx");
    profiler_serialCommunicationTask.setNumberOfCalls(500);

    static DAP_config_st sct_dap_config_st;

    // Buffer to accumulate incoming serial data
    const size_t RX_BUFFER_SIZE = 1028; // Should be at least 2x the largest possible packet
    static uint8_t rx_buffer[RX_BUFFER_SIZE];
    static size_t buffer_len = 0;

    configDataPackage_t configPackage_st;

    for (;;) {
        // Wait for a notification that data might be available
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {


          // if new data package is available, update the local config
          if (xQueueReceive(configUpdateSendToSerialRXTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS) {
            sct_dap_config_st = configPackage_st.config_st;

            // activate profiler depending on pedal config
            if (sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
            {
              profiler_serialCommunicationTask.activate( true );
            }
            else
            {
              profiler_serialCommunicationTask.activate( false );
            }

            ActiveSerial->println("Update config: serial RX");
          } 

          

          // Activate profiler based on config
          profiler_serialCommunicationTask.start(0);

          // --- 1. Read all available data into our buffer ---
          if (ActiveSerial->available()) {
              // Prevent buffer overflow by only reading what fits
              size_t bytesToRead = min((size_t)ActiveSerial->available(), RX_BUFFER_SIZE - buffer_len);
              if (bytesToRead > 0) {
                  ActiveSerial->readBytes(&rx_buffer[buffer_len], bytesToRead);
                  buffer_len += bytesToRead;
              }

              // ActiveSerial->println("Serial data available");
          }

          // --- 2. Process all complete packets in the buffer ---
          size_t buffer_idx = 0;
          while (buffer_idx < buffer_len) {
              // A. Find the next valid Start-of-Frame (SOF)
              if (rx_buffer[buffer_idx] != SOF_BYTE_0 || (buffer_idx + 1 < buffer_len && rx_buffer[buffer_idx + 1] != SOF_BYTE_1)) {
                  buffer_idx++;
                  continue; // Keep scanning for a SOF
              }

              // ActiveSerial->println("1st check passed");

              // SOF found at buffer_idx. Check if we have enough data for a header.
              if (buffer_len < buffer_idx + 3) {
                  // Not enough data for a full header, stop parsing for now
                  break;
              }

              // B. Get expected packet size from payload type
              uint8_t payloadType = rx_buffer[buffer_idx + 2];
              size_t expectedSize = getExpectedPacketSize(payloadType);

              if (expectedSize == 0) {
                  // Unknown payload type, this SOF is corrupt. Skip it and continue scanning.
                  buffer_idx++;
                  continue;
              }

              // ActiveSerial->println("2nd check passed");

              // C. Check if the full packet has arrived
              if (buffer_len < buffer_idx + expectedSize) {
                  // Full packet is not yet in the buffer, wait for more data
                  break;
              }

              // D. Check for valid End-of-Frame (EOF)
              if (rx_buffer[buffer_idx + expectedSize - 2] != EOF_BYTE_0 || rx_buffer[buffer_idx + expectedSize - 1] != EOF_BYTE_1) {
                  // EOF is wrong, this packet is corrupt. Skip the SOF and continue scanning.
                  buffer_idx++;
                  continue;
              }
              
              // --- We have a candidate packet! Now validate and process it. ---
              uint8_t* packet_start = &rx_buffer[buffer_idx];
              bool structIsValid = true;
              uint16_t received_crc = 0;
              uint16_t calculated_crc = 0;

              switch (payloadType) {
                  case DAP_PAYLOAD_TYPE_CONFIG: {
                      DAP_config_st received_config;
                      memcpy(&received_config, packet_start, sizeof(DAP_config_st));
                      
                      calculated_crc = checksumCalculator((uint8_t*)(&(received_config.payLoadHeader_)), sizeof(received_config.payLoadHeader_) + sizeof(received_config.payLoadPedalConfig_));
                      received_crc = received_config.payloadFooter_.checkSum;

                      if (calculated_crc != received_crc || received_config.payLoadHeader_.version != DAP_VERSION_CONFIG) {
                          structIsValid = false;
                      } else {
                          // --- VALID CONFIG PACKET ---
                        ActiveSerial->println("Updating pedal config from serial");
                        //global_dap_config_class.setConfig(received_config);

                        configDataPackage_t configPackage_st;
                        configPackage_st.config_st = received_config;
                        xQueueSend(configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
                        
                        if (received_config.payLoadHeader_.storeToEeprom == 1) {
                          Buzzer.single_beep_tone(700, 100);
                        }
                      }
                      break;
                  }
                  case DAP_PAYLOAD_TYPE_ACTION: {
                      DAP_actions_st received_action;
                      memcpy(&received_action, packet_start, sizeof(DAP_actions_st));

                      // ActiveSerial->println("Action received");

                      calculated_crc = checksumCalculator((uint8_t*)(&(received_action.payLoadHeader_)), sizeof(received_action.payLoadHeader_) + sizeof(received_action.payloadPedalAction_));
                      received_crc = received_action.payloadFooter_.checkSum;

                      if (calculated_crc != received_crc || received_action.payLoadHeader_.version != DAP_VERSION_CONFIG) {
                          structIsValid = false;
                      } else {
                          // --- VALID ACTION PACKET ---
                          // Place your extensive action handling logic here
                          // For clarity, this could be moved to its own function: handleActionPacket(received_action);
                          if (received_action.payloadPedalAction_.system_action_u8 == 2) {
                              ActiveSerial->println("ESP restart by user request");
                              ESP.restart();
                          }

                          //3= Wifi OTA
                          #ifdef ESPNOW_Enable
                          if (received_action.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::ENABLE_OTA)
                          {
                            ActiveSerial->println("Get OTA command");
                            OTA_enable_b=true;
                            //OTA_enable_start=true;
                            ESPNow_OTA_enable=false;
                          }
                          #endif
                          //4 Enable pairing
                          if (received_action.payloadPedalAction_.system_action_u8==4)
                          {
                            #ifdef ESPNow_Pairing_function
                              ActiveSerial->println("Get Pairing command");
                              software_pairing_action_b=true;
                            #endif
                            #ifndef ESPNow_Pairing_function
                              ActiveSerial->println("no supporting command");
                            #endif
                          }
                          
                          if (received_action.payloadPedalAction_.system_action_u8==(uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
                          {
                            #ifdef ESPNow_S3
                              ActiveSerial->println("Restart into Download mode");
                              delay(1000);
                              REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
                              ESP.restart();
                            #else
                              ActiveSerial->println("Command not supported");
                              delay(1000);
                            #endif
                            //ESPNOW_BootIntoDownloadMode = false;
                          }
                          if (received_action.payloadPedalAction_.system_action_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
                          {
                            char logString[200];
                            snprintf(logString, sizeof(logString),
                                    "Pedal ID: %d\nBoard: %s\nLoadcell shift= %.3f kg\nLoadcell variance= %.3f kg\nPSU voltage:%.1f V\nMax endstop:%lu\nCurrentPos:%lu\n\0",
                                    sct_dap_config_st.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, loadcell->getShiftingEstimate(), loadcell->getSTDEstimate(), ((float)stepper->getServosVoltage() / 10.0f), dap_calculationVariables_st.stepperPosMaxEndstop, dap_calculationVariables_st.current_pedal_position);
                            ActiveSerial->println(logString);
                          }

                          // trigger ABS effect
                          if (received_action.payloadPedalAction_.triggerAbs_u8>0)
                          {
                            // ActiveSerial->println("Trigger ABS");
                            absOscillation.trigger();
                            if(received_action.payloadPedalAction_.triggerAbs_u8>1)
                            {
                              dap_calculationVariables_st.TrackCondition=received_action.payloadPedalAction_.triggerAbs_u8-1;
                            }
                            else
                            {
                              dap_calculationVariables_st.TrackCondition=received_action.payloadPedalAction_.triggerAbs_u8=0;
                            }
                          }
                          //RPM effect
                          _RPMOscillation.RPM_value=received_action.payloadPedalAction_.RPM_u8;
                          //G force effect
                          _G_force_effect.G_value=received_action.payloadPedalAction_.G_value-128;       
                          //wheel slip
                          if (received_action.payloadPedalAction_.WS_u8)
                          {
                            _WSOscillation.trigger();
                          }     
                          //Road impact
                          if(dap_calculationVariables_st.Rudder_status==false)
                          {
                            _Road_impact_effect.Road_Impact_value=received_action.payloadPedalAction_.impact_value_u8;
                          }
                          else
                          {

                          }
                          
                          // trigger system identification
                          if (received_action.payloadPedalAction_.startSystemIdentification_u8)
                          {
                            systemIdentificationMode_b = true;
                          }
                          // trigger Custom effect effect 1
                          if (received_action.payloadPedalAction_.Trigger_CV_1)
                          {
                            CV1.trigger();
                          }
                          // trigger Custom effect effect 2
                          if (received_action.payloadPedalAction_.Trigger_CV_2)
                          {
                            CV2.trigger();
                          }
                          // trigger return pedal position
                          if (received_action.payloadPedalAction_.returnPedalConfig_u8)
                          {
                          
                            DAP_config_st * dap_config_st_local_ptr;
                            dap_config_st_local_ptr = &sct_dap_config_st;
                            dap_config_st_local_ptr->payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
                            dap_config_st_local_ptr->payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
                            dap_config_st_local_ptr->payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
                            dap_config_st_local_ptr->payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
                            uint16_t crc = checksumCalculator((uint8_t*)(&(sct_dap_config_st.payLoadHeader_)), sizeof(sct_dap_config_st.payLoadHeader_) + sizeof(sct_dap_config_st.payLoadPedalConfig_));
                            dap_config_st_local_ptr->payloadFooter_.checkSum = crc;

                            // suspend the serial Tx task so that data can properly be send
                            vTaskSuspend(handle_serialCommunicationTx);
                            delay(50);
                            ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DAP_config_st));
                            ActiveSerial->print("Return pedal config");
                            delay(50);
                            vTaskResume(handle_serialCommunicationTx);

                          }
                          #ifdef ESPNOW_Enable
                            if(received_action.payloadPedalAction_.Rudder_action==1)//Enable Rudder
                            {
                              if(dap_calculationVariables_st.Rudder_status==false)
                              {
                                dap_calculationVariables_st.Rudder_status=true;
                                ActiveSerial->println("Rudder on");
                                Rudder_initializing=true;
                                moveSlowlyToPosition_b=true;
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
                              }
                              else
                              {
                                dap_calculationVariables_st.Rudder_status=false;
                                ActiveSerial->println("Rudder off");
                                Rudder_deinitializing=true;
                                moveSlowlyToPosition_b=true; 

                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
                              }
                            }
                            if(received_action.payloadPedalAction_.Rudder_brake_action==1)
                            {
                              if(dap_calculationVariables_st.rudder_brake_status==false&&dap_calculationVariables_st.Rudder_status==true)
                              {
                                dap_calculationVariables_st.rudder_brake_status=true;
                                ActiveSerial->println("Rudder brake on");
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
                              }
                              else
                              {
                                dap_calculationVariables_st.rudder_brake_status=false;
                                ActiveSerial->println("Rudder brake off");
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.Rudder_status);
                              }
                            }
                            //clear rudder status
                            if(received_action.payloadPedalAction_.Rudder_action==2)
                            {
                              dap_calculationVariables_st.Rudder_status=false;
                              dap_calculationVariables_st.rudder_brake_status=false;
                              ActiveSerial->println("Rudder Status Clear");
                              Rudder_deinitializing=true;
                              moveSlowlyToPosition_b=true;

                            }
                          #endif
                          
                          

                          
                      }
                      break;
                  }
                  case DAP_PAYLOAD_TYPE_ACTION_OTA:{
                    memcpy(&dap_action_ota_st, packet_start, sizeof(DAP_action_ota_st));
                    ActiveSerial->println("Get OTA command");
                    buzzerBeepAction_b=true;

                    //ActiveSerial->readBytes((char*)&dap_action_ota_st, sizeof(DAP_action_ota_st));
                    #ifdef OTA_update
                      if(dap_action_ota_st.payLoadHeader_.payloadType==DAP_PAYLOAD_TYPE_ACTION_OTA)
                      {
                        if(dap_action_ota_st.payloadOtaInfo_.device_ID == sct_dap_config_st.payLoadPedalConfig_.pedal_type)
                        {
                          SSID=new char[dap_action_ota_st.payloadOtaInfo_.SSID_Length+1];
                          PASS=new char[dap_action_ota_st.payloadOtaInfo_.PASS_Length+1];
                          memcpy(SSID,dap_action_ota_st.payloadOtaInfo_.WIFI_SSID,dap_action_ota_st.payloadOtaInfo_.SSID_Length);
                          memcpy(PASS,dap_action_ota_st.payloadOtaInfo_.WIFI_PASS,dap_action_ota_st.payloadOtaInfo_.PASS_Length);
                          SSID[dap_action_ota_st.payloadOtaInfo_.SSID_Length]=0;
                          PASS[dap_action_ota_st.payloadOtaInfo_.PASS_Length]=0;
                          OTA_enable_b=true;
                          OTA_enable_start=true;
                          #ifdef ESPNOW_Enable
                            ESPNow_OTA_enable=false;
                          #endif
                        }
                      }
                    #else
                      ActiveSerial->println("The command is not supported");
                    #endif    
                    break;
                }

                  
              } // end switch

              if (!structIsValid) {
                  ActiveSerial->printf("Invalid packet detected (Type: %d). Skipping SOF.\n", payloadType);
                  buffer_idx++; // Skip the failed SOF and continue scanning
              } else {
                  // Packet was valid and processed, advance index past this packet
                  buffer_idx += expectedSize;
              }
          } // end while

          // --- 3. Clean up the buffer ---
          if (buffer_idx > 0) {
              size_t remaining_len = buffer_len - buffer_idx;
              if (remaining_len > 0) {
                  memmove(rx_buffer, &rx_buffer[buffer_idx], remaining_len);
              }
              buffer_len = remaining_len;
          }

          profiler_serialCommunicationTask.end(0);
          profiler_serialCommunicationTask.report();
        } // end if TaskNotifyTake
    } // end for(;;)
}



uint32_t communicationTask_stackSizeIdx_u32 = 0;
void IRAM_ATTR_FLAG serialCommunicationTaskTx( void * pvParameters )
{ 
  // FunctionProfiler profiler_serialCommunicationTask;
  // profiler_serialCommunicationTask.setName("SerialCommunicationTx");
  // profiler_serialCommunicationTask.setNumberOfCalls(500);

  // static DAP_config_st sct_dap_config_st;

  // This task now waits for a complete package of data from the queue.
  PedalStatePackage_t receivedState;

  for(;;){

    // global_dap_config_class.getConfig(&sct_dap_config_st, 0);

    // Block indefinitely until a new state package arrives from pedalUpdateTask.
    // This is now the ONLY trigger for this task.
    if (xQueueReceive(pedalStateQueue, &receivedState, portMAX_DELAY) == pdPASS) {
      
      // Now, process the first item, and then enter a loop to
      // empty the rest of the queue.
      do {
                
        // Copy to a local variable to calculate CRC
        DAP_state_basic_st basic_to_send = receivedState.basic_st;
        DAP_state_extended_st extended_to_send = receivedState.extended_st;



  // Provide pedal states to ESPnow task
  #ifdef ESPNOW_Enable
        // update pedal states
        if(semaphore_updatePedalStates!=NULL)
        {
          if(xSemaphoreTake(semaphore_updatePedalStates, (TickType_t)0)==pdTRUE) 
          {
            // move local structure values to global structures
            dap_state_basic_st = basic_to_send;
            dap_state_extended_st = extended_to_send;

            // release semaphore
            xSemaphoreGive(semaphore_updatePedalStates);
          }
        }
        else
        {
          semaphore_updatePedalStates = xSemaphoreCreateMutex();
        }
  #endif



        // activate profiler depending on pedal config
        // if (sct_dap_config_st.payLoadPedalConfig_.debug_flags_0 & DEBUG_INFO_0_CYCLE_TIMER) 
        // {
        //   profiler_serialCommunicationTask.activate( true );
        // }
        // else
        // {
        //   profiler_serialCommunicationTask.activate( false );
        // }
        // // start profiler 0, overall function
        // profiler_serialCommunicationTask.start(0);

        // send basic pedal state struct
        if (receivedState.sendBasicFlag_b)
        {
          // update CRC before transmission
          basic_to_send.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(basic_to_send.payLoadHeader_)), sizeof(basic_to_send.payLoadHeader_) + sizeof(basic_to_send.payloadPedalState_Basic_));
          ActiveSerial->write((char*)&basic_to_send, sizeof(DAP_state_basic_st));
        }

        // send extended pedal state struct
        if (receivedState.sendExtendedFlag_b)
        {
          // update CRC before transmission
          extended_to_send.payloadFooter_.checkSum = checksumCalculator((uint8_t*)(&(extended_to_send.payLoadHeader_)), sizeof(extended_to_send.payLoadHeader_) + sizeof(extended_to_send.payloadPedalState_Extended_));
          ActiveSerial->write((char*)&extended_to_send, sizeof(DAP_state_extended_st));
        }

        // profiler_serialCommunicationTask.end(0);

        // // print profiler results
        // profiler_serialCommunicationTask.report();
      // Continue looping with a zero timeout to process any other items that are
      // already in the queue. The loop will exit when the queue is empty.
      } while (xQueueReceive(pedalStateQueue, &receivedState, (TickType_t)0) == pdPASS);


      // force a context switch
      // taskYIELD();
    }
  }
}

//OTA multitask

void otaUpdateTask( void * pvParameters )
{
  uint16_t OTA_count=0;
  unsigned long ota_debug_messaage_last=0;
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
              Buzzer.play_melody_tone(melody_victory_theme, sizeof(melody_victory_theme)/sizeof(melody_victory_theme[0]),melody_durations_Victory_theme);              
              ESP.restart();
            }
            else
            {
              if(dap_action_ota_st.payloadOtaInfo_.ota_action==OTA_ACTION_PLATFORMIO_DIRECT_UPLOAD)
              {
                //updload ota from platformio
                //sendESPNOWLog("Upload from platformIO");
                ArduinoOTA.handle(); 
                if(millis()-ota_debug_messaage_last>1000)
                {
                  ActiveSerial->println("Wait for ota update...");
                }
                
              }
              else
              {
                //show ota error
                Buzzer.single_beep_tone(770,100);
                #ifdef USING_LED
                  pixels.setPixelColor(0,0xff,0x00,0x00);//red
                  pixels.show(); 
                  delay(500);
                  pixels.setPixelColor(0,0x00,0x00,0x00);//no color
                  pixels.show();
                  delay(500);    
                #endif 
              }

            }

          #endif
          

        }
        else
        {
          esp_err_t result;
          ActiveSerial->println("de-initialize espnow");
          ActiveSerial->println("wait...");
          #ifdef ESPNOW_Enable
            sendESPNOWLog("OTA enabled, de-initialize espnow");
            sendESPNOWLog("wait...");
            delay(1000);
            result= esp_now_deinit();
            ESPNow_initial_status=false;
            ESPNOW_status=false;
          #else
            result = ESP_OK;
          #endif
          //result = ESP_OK;
          delay(3000);
          if(result==ESP_OK)
          {
            OTA_status=true;
            Buzzer.single_beep_tone(700,100); 
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
            //sendESPNOWLog("Wifi Connected");
            if(dap_action_ota_st.payloadOtaInfo_.ota_action!=OTA_ACTION_PLATFORMIO_DIRECT_UPLOAD)
            {
              ESP32OTAPull ota;
              int ret;
              ota.SetCallback(OTAcallback);
              ota.OverrideBoard(CONTROL_BOARD);
              char* version_tag;
              if(dap_action_ota_st.payloadOtaInfo_.ota_action==OTA_ACTION_FORCE_UPDATE)
              {
                const char* str;
                if(PCB_VERSION==3||PCB_VERSION==5||PCB_VERSION==9) str ="0.90.16";// for those board which change the partition table
                else str ="0.0.0";
                version_tag=new char[strlen(str) + 1];
                strcpy(version_tag, str);
                ActiveSerial->println("Force update");
              }
              if(dap_action_ota_st.payloadOtaInfo_.ota_action==OTA_ACTION_NORMAL)
              {
                version_tag=new char[strlen(DAP_FIRMWARE_VERSION) + 1];
                strcpy(version_tag, DAP_FIRMWARE_VERSION);
                //version_tag=DAP_FIRMWARE_VERSION;
              }
              switch (dap_action_ota_st.payloadOtaInfo_.mode_select)
              {
                case 1:
                  ActiveSerial->printf("Flashing to latest release, checking %s to see if an update is available...\n", OTA_JSON_URL_MAIN);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_MAIN, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                case 2:
                  ActiveSerial->printf("Flashing to latest dev build, checking %s to see if an update is available...\n", OTA_JSON_URL_DEV);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_DEV, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                case 3:
                  ActiveSerial->printf("Flashing to test build, checking %s to see if an update is available...\n", OTA_JSON_URL_TEST);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_TEST, version_tag, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                default:
                break;
                delete[] version_tag; 
              }
            }  
            else
            {
              // initialize ota for platformIO upload
              ActiveSerial->println("OTA from platformIO");
              ota_arduinoota_initialize();
            }         
            #endif
            delay(3000);
          }

        }
      }
      
      #endif
    }

    // force a context switch
		taskYIELD();
  }
}

#ifdef ESPNOW_Enable

void IRAM_ATTR_FLAG espNowCommunicationTaskTx( void * pvParameters )
{
  FunctionProfiler profiler_espNow;
  profiler_espNow.setName("EspNow");

  uint Pairing_timeout=20000;
  uint rudderPacketInterval=3;
  uint joystickPacketInterval=3;
  uint basicStateUpdateInterval=3;
  uint extendStateUpdateInterval=10;
  uint assignmentPacketUpdateInterval = 100;
  bool Pairing_timeout_status=false;
  bool building_dap_esppairing_lcl =false;
  unsigned long Pairing_state_start;
  unsigned long Pairing_state_last_sending;
  unsigned long Debug_rudder_last=0;
  unsigned long basic_state_update_last=0;
  unsigned long extend_state_update_last=0;
  unsigned long rudderPacketsUpdateLast=0;
  unsigned long joystickPacketsUpdateLast=0;
  unsigned long assignmentPacketUpdateLast = 0;
  uint32_t espNowTask_stackSizeIdx_u32 = 0;

  int error_count=0;
  int print_count=0;
  int ESPNow_no_device_count=0;
  bool basic_state_send_b=false;
  bool extend_state_send_b=false;
  bool noAssignmentStatus=false;
  bool assignmentUpdatePacketSend_b=false;
  uint8_t error_out;

  for(;;)
  {
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {

        //basic state sendout interval
        if(millis()-basic_state_update_last>basicStateUpdateInterval)
        {
          basic_state_send_b=true;
          basic_state_update_last=millis();
          
        }
        DAP_config_st espnow_dap_config_st;
        global_dap_config_class.getConfig(&espnow_dap_config_st, 500);
        if (espnow_dap_config_st.payLoadPedalConfig_.pedal_type ==PEDAL_ID_UNKNOWN)
        {
          noAssignmentStatus=true;
        }
        //restart from espnow
        if(ESPNow_restart)
        {
          ActiveSerial->println("ESP restart by ESPnow request");
          sendESPNOWLog("Pedal:%d restarted by request", espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
          delay(2);
          ESP.restart();
        }
        //entend state send out interval
        if((millis()-extend_state_update_last>extendStateUpdateInterval) && espnow_dap_config_st.payLoadPedalConfig_.debug_flags_0 == DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT)
        {
          extend_state_send_b=true;
          extend_state_update_last=millis();
          
        }
        if (millis() - assignmentPacketUpdateLast > assignmentPacketUpdateInterval)
        {
          assignmentUpdatePacketSend_b = true;
          assignmentPacketUpdateLast = millis();
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
              ActiveSerial->println("Pedal Pairing.....");
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
                ActiveSerial->print("Pedal: ");
                ActiveSerial->print(espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
                ActiveSerial->println(" timeout.");
                Buzzer.single_beep_tone(700,100);
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
                      ActiveSerial->print("#");
                      ActiveSerial->print(i);
                      ActiveSerial->print("Pair: ");
                      ActiveSerial->print(ESP_pairing_reg_local.Pair_status[i]);
                      ActiveSerial->printf(" Mac: %02X:%02X:%02X:%02X:%02X:%02X\n", ESP_pairing_reg_local.Pair_mac[i][0], ESP_pairing_reg_local.Pair_mac[i][1], ESP_pairing_reg_local.Pair_mac[i][2], ESP_pairing_reg_local.Pair_mac[i][3], ESP_pairing_reg_local.Pair_mac[i][4], ESP_pairing_reg_local.Pair_mac[i][5]);
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
          /*
          if((joystickPacketsUpdateLast-millis())>joystickPacketInterval) 
          {
            ESPNow_Joystick_Broadcast(joystickNormalizedToInt32);
            joystickPacketsUpdateLast=millis();
          }
          */
          
          profiler_espNow.end(1);

          profiler_espNow.start(2);
          // assignment request packet send out
          if(assignmentUpdatePacketSend_b && noAssignmentStatus)
          {
            //ActiveSerial->println("Send out assignment request");
            assignmentUpdatePacketSend_b=false;
            DAP_AssignmentBoardcast_st dap_assignmentBoardcast_st;
            dap_assignmentBoardcast_st.payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
            dap_assignmentBoardcast_st.payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
            dap_assignmentBoardcast_st.payLoadHeader_.version = DAP_VERSION_CONFIG;
            dap_assignmentBoardcast_st.payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_ASSIGNMENT;
            dap_assignmentBoardcast_st.payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
            dap_assignmentBoardcast_st.payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
            dap_assignmentBoardcast_st.payloadAssignmentRequest_.assignmentAction=1;
            memcpy(dap_assignmentBoardcast_st.payloadAssignmentRequest_.macAddress,esp_Mac,6);
            uint16_t crc = 0;
            crc = checksumCalculator((uint8_t *)(&(dap_assignmentBoardcast_st.payLoadHeader_)), sizeof(dap_assignmentBoardcast_st.payLoadHeader_) + sizeof(dap_assignmentBoardcast_st.payloadAssignmentRequest_));
            dap_assignmentBoardcast_st.payloadFooter_.checkSum=crc;
            ESPNow.send_message(broadcast_mac, (uint8_t *)&dap_assignmentBoardcast_st, sizeof(DAP_AssignmentBoardcast_st));
          }
          // basic state packet send out  
          if(basic_state_send_b && !noAssignmentStatus)
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

          if (extend_state_send_b && !noAssignmentStatus)
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

          if (ESPNow_config_request && !noAssignmentStatus)
          {
            DAP_config_st * dap_config_st_local_ptr;
            dap_config_st_local_ptr = &espnow_dap_config_st;
            dap_config_st_local_ptr->payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
            dap_config_st_local_ptr->payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
            dap_config_st_local_ptr->payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
            dap_config_st_local_ptr->payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;
            dap_config_st_local_ptr->payLoadHeader_.PedalTag = dap_config_st_local_ptr->payLoadPedalConfig_.pedal_type;
            uint16_t crc=0;
            crc = checksumCalculator((uint8_t*)(&(espnow_dap_config_st.payLoadHeader_)), sizeof(espnow_dap_config_st.payLoadHeader_) + sizeof(espnow_dap_config_st.payLoadPedalConfig_));
            dap_config_st_local_ptr->payloadFooter_.checkSum = crc;
            ESPNow.send_message(broadcast_mac,(uint8_t *) & espnow_dap_config_st, sizeof(espnow_dap_config_st));
            ESPNow_config_request=false;
            sendESPNOWLog("Pedal:%d Config returned by user request, CRC:%d", espnow_dap_config_st.payLoadPedalConfig_.pedal_type, crc);
            delay(2);
          }

          if (ESPNow_OTA_enable && !noAssignmentStatus)
          {
            ActiveSerial->println("Get OTA command");
            
            OTA_enable_b=true;
            OTA_enable_start=true;
            ESPNow_OTA_enable=false;
          }

          if (OTA_update_action_b && !noAssignmentStatus)
          {
            ActiveSerial->println("Starting Pedal OTA");
            buzzerBeepAction_b=true;
            OTA_enable_b=true;
            OTA_enable_start=true;
            ESPNow_OTA_enable=false;
            OTA_update_action_b = false;
            //ActiveSerial->println("get basic wifi info");
            //ActiveSerial->readBytes((char*)&dap_action_ota_st, sizeof(DAP_action_ota_st));
            #ifdef OTA_update

              if(dap_action_ota_st.payLoadHeader_.payloadType==DAP_PAYLOAD_TYPE_ACTION_OTA)
              {
                if(dap_action_ota_st.payloadOtaInfo_.device_ID == espnow_dap_config_st.payLoadPedalConfig_.pedal_type)
                {
                  SSID=new char[dap_action_ota_st.payloadOtaInfo_.SSID_Length+1];
                  PASS=new char[dap_action_ota_st.payloadOtaInfo_.PASS_Length+1];
                  memcpy(SSID,dap_action_ota_st.payloadOtaInfo_.WIFI_SSID,dap_action_ota_st.payloadOtaInfo_.SSID_Length);
                  memcpy(PASS,dap_action_ota_st.payloadOtaInfo_.WIFI_PASS,dap_action_ota_st.payloadOtaInfo_.PASS_Length);
                  SSID[dap_action_ota_st.payloadOtaInfo_.SSID_Length]=0;
                  PASS[dap_action_ota_st.payloadOtaInfo_.PASS_Length]=0;
                  OTA_enable_b=true;
                }
              }

            #endif

          }

          if (printPedalInfo_b && !noAssignmentStatus)
          {
            printPedalInfo_b=false;
            buzzerBeepAction_b=true;
            delay(100);
            pedalInfoBuilder.BuildInfoString(espnow_dap_config_st.payLoadPedalConfig_.pedal_type, CONTROL_BOARD, loadcell->getShiftingEstimate(), loadcell->getSTDEstimate(), ((float)stepper->getServosVoltage()/10.0f),dap_calculationVariables_st.stepperPosMaxEndstop,dap_calculationVariables_st.current_pedal_position);
            sendESPNOWLog(pedalInfoBuilder.logString);
            ActiveSerial->println(pedalInfoBuilder.logString);
            delay(3);
            pedalInfoBuilder.BuildESPNOWInfo(espnow_dap_config_st.payLoadPedalConfig_.pedal_type,rssi);
            sendESPNOWLog(pedalInfoBuilder.logESPNOWString);
            ActiveSerial->println(pedalInfoBuilder.logESPNOWString);

          }
          if (Get_Rudder_action_b && !noAssignmentStatus)
          {
            Get_Rudder_action_b=false;
            Buzzer.single_beep_tone(700,100);
          }
          if (Get_HeliRudder_action_b && !noAssignmentStatus)
          {
            Get_HeliRudder_action_b=false;
            Buzzer.single_beep_tone(700,100);
          }
          if (ESPNOW_BootIntoDownloadMode && !noAssignmentStatus)
          {
            #ifdef ESPNow_S3
              ActiveSerial->println("Restart into Download mode");
              sendESPNOWLog("Pedal:%d restart into Download mode", espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
              delay(1000);
              REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
              ESP.restart();
            #else
              ActiveSerial->println("Command not supported");
              delay(1000);
            #endif
            ESPNOW_BootIntoDownloadMode = false;
          }
          //send out rudder packet after rudder initialized
          if (rudderPacketsUpdateLast - millis() > rudderPacketInterval && !noAssignmentStatus)
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
              //  ActiveSerial->println("Error sending the data");
              //}
              if (ESPNow_Rudder_Update && !noAssignmentStatus)
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
              ActiveSerial->print("Pedal:");
              ActiveSerial->print(espnow_dap_config_st.payLoadPedalConfig_.pedal_type);
              ActiveSerial->print(", Send %: ");
              ActiveSerial->print(dap_rudder_sending.payloadRudderState_.pedal_position_ratio);
              ActiveSerial->print(", Recieve %:");
              ActiveSerial->print(dap_rudder_receiving.payloadRudderState_.pedal_position_ratio);
              ActiveSerial->print(", Send Position: ");
              ActiveSerial->print(dap_calculationVariables_st.current_pedal_position);
              ActiveSerial->print(", % in cal: ");
              ActiveSerial->print(dap_calculationVariables_st.current_pedal_position_ratio); 
              ActiveSerial->print(", min cal: ");
              ActiveSerial->print(dap_calculationVariables_st.stepperPosMin_default); 
              ActiveSerial->print(", max cal: ");
              ActiveSerial->print(dap_calculationVariables_st.stepperPosMax_default);
              ActiveSerial->print(", range in cal: ");
              ActiveSerial->println(dap_calculationVariables_st.stepperPosRange_default); 
            }

            //Debug_rudder_last=now_rudder;
            //ActiveSerial->println(dap_calculationVariables_st.current_pedal_position);                  
                
            print_count=0;
          }
          else
          {
            print_count++;
                
          } 
              
                  
        #endif



      }

      profiler_espNow.end(0);

      // print profiler results
      profiler_espNow.report();

      // force a context switch
		taskYIELD();
    }
}
#endif


void miscTask( void * pvParameters )
{
  static DAP_config_st misc_dap_config_st;
  // for the task no need complete asap, ex buzzer, led 
  for(;;)
  {
    global_dap_config_class.getConfig(&misc_dap_config_st, 500);
    
    #ifdef ESPNOW_Enable
      //software assignment
      if(assignmentUpdate_b)
      {
        assignmentUpdate_b = false;
        writeAssignmentToEeprom();
        delay(1000);
        //restart after aassignment
        ESP.restart();
      }
      if (assignmentClear_b)
      {
        assignmentClear_b = false;
        clearAssignmentToEeprom();
        delay(1000);
        // restart after aassignment
        ESP.restart();
      }
#endif
    // make buzzer sound actions here
    #ifdef ESPNOW_Enable
      if (Config_update_Buzzer_b)
      {
        Buzzer.single_beep_tone(700, 50);
        Config_update_Buzzer_b = false;
      }
      if (assignmentUpdateBuzzer_b)
      {
        ActiveSerial->println("Beep");
        Buzzer.single_beep_tone(700, 50);
        assignmentUpdateBuzzer_b = false;
      }
      if(buzzerBeepAction_b)
      {
        Buzzer.single_beep_tone(700,50);
        buzzerBeepAction_b=false;
      }
    #endif
    #if defined(OTA_update) && defined(USING_BUZZER)
      if(beepForOtaProgress)
      {
        Buzzer.single_beep_tone(700,50);
        beepForOtaProgress=false;
      }
    #endif
    delay(50);
  }
}
