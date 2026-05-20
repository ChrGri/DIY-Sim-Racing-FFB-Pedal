
/* Todo*/
// https://github.com/espressif/arduino-esp32/issues/7779


#include "esp_timer.h" // Include the header for the high-resolution timer
#include "esp_partition.h"


#define ESTIMATE_LOADCELL_VARIANCE_B
//#define PRINT_SERVO_STATES

#define DEBUG_INFO_0_CYCLE_TIMER_U8 1U
#define DEBUG_INFO_0_NET_RUNTIME_U8 2U
// #define DEBUG_INFO_0_LOADCELL_READING 4
#define DEBUG_INFO_0_SERVO_READINGS_U8 8U
#define DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS_U8 16U
#define DEBUG_INFO_0_RESET_SERVO_TO_FACTORY_U8 32U
#define DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT_U8 64U
#define DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS_U8 128U


#define EFFECT_SCALING_FACTOR_FL32 4.0f
#define EFFECT_POSITION_SCALING_FACTOR_FL32 0.1f

#define BAUD_3M_U32 3000000U
#define DEFAULT_BAUD_U32 921600U
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


#include "DiyActivePedal_types.h"


/**********************************************************************************************/
/*                                                                                            */
/*                         variable declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
DapConfigClass global_dap_config_class;
DRAM_ATTR DapCalculationVariables_t dap_calculationVariables_st;
DapStateBasic_t dap_state_basic_st;
DapStateExtended_t dap_state_extended_st;
DapEspPairing_t dap_esppairing_st;//saving
DapEspPairing_t dap_esppairing_lcl;//sending
DapActionOta_t dap_action_ota_st;//OTA command(do not check version)



/**********************************************************************************************/
/*                                                                                            */
/*                         struct definitions                                                 */
/*                                                                                            */
/**********************************************************************************************/
typedef struct {
    DapStateBasic_t    basic_st;
    DapStateExtended_t extended_st;
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
    DapConfig_t config_st;
} configDataPackage_t;


/**********************************************************************************************/
/*                                                                                            */
/*                         function declarations                                              */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters(const DapConfig_t& newConfig);
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
inline uint16_t checksumCalculator_u16(uint8_t * data_pu8, uint16_t length_u16)
{
  uint16_t curr_crc_u16 = 0x0000;
  uint8_t sum1_u8 = (uint8_t) curr_crc_u16;
  uint8_t sum2_u8 = (uint8_t) (curr_crc_u16 >> 8);
  int index_i32;
  for(index_i32 = 0; index_i32 < length_u16; index_i32 = index_i32 + 1)
   {
    sum1_u8 = (sum1_u8 + data_pu8[index_i32]) % 255;
    sum2_u8 = (sum2_u8 + sum1_u8) % 255;
   }
  return (sum2_u8 << 8) | sum1_u8;
}




unsigned long saveToEEPRomDuration=0;



bool splineDebug_b = false;



#include <EEPROM.h>
#define EEPROM_OFFSET_U32 15U


#include "ABSOscillation.h"
#include "Rudder.h"
ABSOscillation absOscillation;
RPMOscillation _RPMOscillation;
BitePointOscillation _BitePointOscillation;
GForceEffect gForceEffect_;
WSOscillation _WSOscillation;
RoadImpactEffect roadImpactEffect_;
CustomVibration customVibration1_;
CustomVibration customVibration2_;
CustomVibration customVibration3_;
CustomVibration customVibration4_;
Rudder _rudder;
HelicoptersRudder helicopterRudder_;
RudderGForce rudderGForce_;
MovingAverageFilter averagefilter_joystick(40);




/**********************************************************************************************/
/*                                                                                            */
/*                         Predictive Brake Controller                                        */
/*                                                                                            */
/**********************************************************************************************/
#include "PredictiveBrakeController.h"
PredictiveBrakeController brakeController;

/**********************************************************************************************/
/*                                                                                            */
/*                         iterpolation  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/

#include "ForceCurve.h"
ForceCurveInterpolated forceCurve;



/**********************************************************************************************/
/*                                                                                            */
/*                         multitasking  definitions                                          */
/*                                                                                            */
/**********************************************************************************************/
#ifndef CONFIG_IDF_TARGET_ESP32S3
  #include "rtc_wdt.h"
#endif





static SemaphoreHandle_t s_semaphore_updatePedalStates=NULL;



/**********************************************************************************************/
/*                                                                                            */
/*                         queue declarations                                                 */
/*                                                                                            */
/**********************************************************************************************/
// ADD THIS: The handle for our new FreeRTOS queue
static QueueHandle_t s_pedalStateQueue = NULL;
static QueueHandle_t s_joystickDataQueue = NULL;
static QueueHandle_t s_loadcellDataQueue = NULL;
static QueueHandle_t s_configUpdateAvailableQueue = NULL;
static QueueHandle_t s_configUpdateSendToPedalUpdateTaskQueue = NULL;
static QueueHandle_t s_configUpdateSendToLoadcellTaskQueue = NULL;
static QueueHandle_t s_actionCommandQueue = NULL;
// static QueueHandle_t configUpdateSendToJoystickTaskQueue = NULL;
static QueueHandle_t s_configUpdateSendToSerialRXTaskQueue = NULL;
static QueueHandle_t s_systemControlQueue = NULL;





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
KalmanFilter1stOrder* kalman = NULL;
KalmanFilter1stOrder* kalman_joystick = NULL;

#include "SignalFilter_2nd_order.h"
KalmanFilter2ndOrder* kalman_2nd_order = NULL;




/**********************************************************************************************/
/*                                                                                            */
/*                         loadcell definitions                                               */
/*                                                                                            */
/**********************************************************************************************/

#ifdef USES_ADS1220
  /*  Uses ADS1220 */
  #include "LoadCell_ads1220.h"
  LoadCellAds1220* loadcell = NULL;

#else
  /*  Uses ADS1256 */
  #include "LoadCell.h"
  LoadCellAds1256* loadcell = NULL;
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
volatile bool moveSlowlyToPosition_b = true;
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
char* g_apHost_pc;
#endif
#ifdef OTA_update_ESP32
  #include "ota.h"
  //#include "OTA_Pull.h"
  TaskHandle_t Task4;
  char* g_apHost_pc;
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
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO_U8, NEO_RGB + NEO_KHZ800);
  #else
    Adafruit_NeoPixel pixels(LEDS_COUNT, LED_GPIO_U8, NEO_GRB + NEO_KHZ800);
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
SimpleBuzzer Buzzer;
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
  DapConfig_t dap_config_st_local;
  configDataPackage_t configPackage_st;

  for (;;)
  {

    // check if config update is available
    if (xQueueReceive(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY) == pdPASS)
    {
      global_dap_config_class.setConfig(configPackage_st.config_st);

      ActiveSerial->println("Config update received: config handling task");

      // send queues to other tasks

      // Send the package to the queue. Use a timeout of 0 (non-blocking).
      // If the queue is full, the data is simply dropped. This prevents this
      // high-priority control task from ever blocking on a full serial buffer.
      xQueueSend(s_configUpdateSendToPedalUpdateTaskQueue, &configPackage_st, portMAX_DELAY);
      xQueueSend(s_configUpdateSendToLoadcellTaskQueue, &configPackage_st, portMAX_DELAY);
      // xQueueSend(configUpdateSendToJoystickTaskQueue, &configPackage_st, portMAX_DELAY);
      xQueueSend(s_configUpdateSendToSerialRXTaskQueue, &configPackage_st, portMAX_DELAY);

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
  static DapConfig_t loadcellTask_dap_config_st;
  configDataPackage_t configPackage_st;

  static float previousLoadcellReadingInKg_fl32 = 0.0f;

  for (;;)
  {

    if (loadcell != NULL)
    {

      // if new data package is available, update the local config
      if (xQueueReceive(s_configUpdateSendToLoadcellTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS)
      {
        loadcellTask_dap_config_st = configPackage_st.config_st;

        // activate profiler depending on pedal config
        if (loadcellTask_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER_U8) 
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
      loadcellReading_fl32 = loadcell->readLoadcellWeightInKg();

      // Invert the loadcell reading digitally if desired
      if (loadcellTask_dap_config_st.payloadPedalConfig_st.invertLoadcellReading_u8 == 1)
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
        if (s_loadcellDataQueue != NULL)
        {

          // Package the new state data into a single struct
          loadcellDataPackage_t newLoadcellPackage;
          newLoadcellPackage.loadcellReadingInKg_fl32 = loadcellReading_fl32;

            // Send the package to the queue. Use a timeout of 0 (non-blocking).
            // If the queue is full, the data is simply dropped. This prevents this
            // high-priority control task from ever blocking on a full serial buffer.
          xQueueSend(s_loadcellDataQueue, &newLoadcellPackage, (TickType_t)0);
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
#define BASE_TICK_US REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64   // base tick in microseconds
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
void IRAM_ATTR_FLAG onTimer() {
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

#define COUNTER_SIZE_U32 4u
uint16_t tickCount_au16[COUNTER_SIZE_U32] = {0};


static uint16_t s_timerTicks_espNowTask_u16 = REPETITION_INTERVAL_ESPNOW_TASK_IN_US_I64 / BASE_TICK_US;




/**********************************************************************************************/
/*                                                                                            */
/*                         setup function                                                     */
/*                                                                                            */
/**********************************************************************************************/
 // #define SERIAL_PATTERN_DETECTOR
#ifdef SERIAL_PATTERN_DETECTOR

#include "driver/uart.h"

// Structure to hold a complete UART packet
#define UART_RX_BUF_SIZE_U32 sizeof(DapConfig_t)
typedef struct {
  uint8_t data[UART_RX_BUF_SIZE_U32];
    size_t len;
} UartPacket_t;

// Queue to pass packets from the UART event task to the processing task
static QueueHandle_t s_serial_packet_queue;

// Queue to handle UART events
static QueueHandle_t s_uart_queue;

// --- ADD THIS LINE ---
#define TEMP_BUFFER_SIZE_U32 (UART_RX_BUF_SIZE_U32 * 2)

/**
 * @brief Task to handle UART events with persistent buffering.
 *
 * This task accumulates data in a static buffer. After new data arrives,
 * it scans the buffer for one or more complete packets ending in the
 * {EOF_BYTE_0_U8, EOF_BYTE_1_U8} sequence. Valid packets are extracted, queued
 * for processing, and removed from the buffer.
 */
static void uart_event_task(void *pvParameters) {
    uart_event_t event;
    
    // Persistent buffer to accumulate fragmented data
    static uint8_t temp_buffer[TEMP_BUFFER_SIZE_U32];
    static size_t temp_buffer_len = 0;

    for (;;) {
        // Wait for a UART event
        if (xQueueReceive(s_uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            
            ActiveSerial->println("UART event triggered");

            switch (event.type) {
                case UART_PATTERN_DET: {

                    ActiveSerial->println("EOF1 detected");

                    // // Read all available new data from the hardware buffer
                    // uint8_t incoming_data[UART_RX_BUF_SIZE_U32];
                    // size_t buffered_size;
                    // uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
                    // int read_len = uart_read_bytes(UART_NUM_0, incoming_data, buffered_size, pdMS_TO_TICKS(100));

                    // if (read_len > 0) {
                    //     // --- 1. Append new data, checking for overflow ---
                    //     if (temp_buffer_len + read_len > TEMP_BUFFER_SIZE_U32) {
                    //         ActiveSerial->println("ERROR: UART temporary buffer overflow. Discarding all data.");
                    //         temp_buffer_len = 0; // Reset the buffer
                    //         break; 
                    //     }
                    //     memcpy(&temp_buffer[temp_buffer_len], incoming_data, read_len);
                    //     temp_buffer_len += read_len;

                    //     // --- 2. Scan buffer for complete packets and process them ---
                    //     if (temp_buffer[temp_buffer_len-2] == EOF_BYTE_0_U8 && temp_buffer[temp_buffer_len-1] == EOF_BYTE_1_U8) {
                            

                    //         // --- 3. Extract the packet and send it to the queue ---
                    //         UartPacket_t packet_to_send;
                    //         packet_to_send.len = temp_buffer_len;
                    //         memcpy(packet_to_send.data, temp_buffer, temp_buffer_len);
                    //         xQueueSend(s_serial_packet_queue, &packet_to_send, (TickType_t)0);

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
                    xQueueReset(s_uart_queue);
                    temp_buffer_len = 0; // Also clear our temp buffer
                    break;

                case UART_BUFFER_FULL:
                    ActiveSerial->println("Ring buffer full");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(s_uart_queue);
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

class NonBlockingStreamWrapper : public Stream {
private:
    Stream* target;
public:
    NonBlockingStreamWrapper(Stream* target) : target(target) {}
    int available() override { return target->available(); }
    int read() override { return target->read(); }
    int peek() override { return target->peek(); }
    void flush() override { target->flush(); }
    
    size_t write(uint8_t c) override {
        // DER MAGISCHE FIX: Ist überhaupt ein Terminal offen?
        if (!Serial) return 1; // Nein? Daten direkt verwerfen.

        if (target->availableForWrite() > 0) {
            return target->write(c);
        }
        return 1;
    }
    
    size_t write(const uint8_t *buffer, size_t size) override {
        if (size == 0) return 0;
        
        // DER MAGISCHE FIX: Ist überhaupt ein Terminal offen?
        if (!Serial) return size; // So tun, als wäre alles gesendet worden!

        int avail = target->availableForWrite();
        if (avail > 0) {
            size_t toWrite = (size < (size_t)avail) ? size : (size_t)avail;
            target->write(buffer, toWrite);
        }
        return size; 
    }

    int availableForWrite() override { 
        if (!Serial) return 0;
        return target->availableForWrite(); 
    }
};

void setup()
{

  

  #ifdef DEBUG_KEEP_USB_SERIAL_JTAG
    // For ESP32-S3, the USB Serial is shared with JTAG. To allow debugging via JTAG while also using Serial for output, we can delay the start of Serial until after the debugger has had time to connect.
    // This is a workaround to ensure that the USB Serial doesn't interfere with JTAG debugging during startup. 
    delay(5000); // Gibt dem Debugger 5 Sekunden Zeit, sich in Ruhe zu verbinden!
  #endif

  // stabilize RS232 pins, so servo doesnt go into alarm on startup
  pinMode(ISV57_TXPIN, OUTPUT);
  digitalWrite(ISV57_TXPIN, HIGH); // HIGH ist der Idle-State für UART

  // disable WIFI & Bluetooth to improve loadcell reading. 
  // HINT: The ESP32 S3 zero board doen't have strong 5V signal smoothing hardware as the regular S3 devboard. 
  #ifdef WIFI_DISABLE
    WiFi.mode(WIFI_OFF);
    btStop();
  #endif
  DapConfig_t dap_config_st_local;
  DapConfig_t dap_config_st_eeprom;

  // setup serial
  // #define USE_CDC_INSTEAD_OF_UART
  #ifdef USE_CDC_INSTEAD_OF_UART
    Serial.begin(DEFAULT_BAUD_U32);
    //Serial.enableReboot(false);
    
    Serial.setTxTimeoutMs(100);
    ActiveSerial = &Serial;
    #ifdef USB_JOYSTICK
      ActiveSerial->println("Setup Controller");
      SetupController();
    #endif
  #elif CONFIG_IDF_TARGET_ESP32S3
    Serial1.begin(BAUD_3M_U32, SERIAL_8N1, 44, 43);
    // Serial.begin(BAUD_3M_U32, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial1;
  #elif CONFIG_IDF_TARGET_ESP32
    Serial.begin(DEFAULT_BAUD_U32);
    // Serial.begin(BAUD_3M_U32, SERIAL_8N1, 44, 43);
    ActiveSerial = &Serial;
  #endif





  // ADD THIS: Create the queue before creating the tasks that use it.
  // The queue can hold up to N state packages.
  // Depth = 200: holds 50 ms of 4 kHz production (4000 × 0.05 = 200 items)
  // giving ample margin even if the TX task is briefly delayed by USB activity.
  s_pedalStateQueue = xQueueCreate(200, sizeof(PedalStatePackage_t));
  if (s_pedalStateQueue == NULL)
  {
    ActiveSerial->println("Error creating the pedal state queue!");
  }
  s_joystickDataQueue = xQueueCreate(1, sizeof(joystickDataPackage_t));
  if (s_joystickDataQueue == NULL)
  {
    ActiveSerial->println("Error creating the joystick data queue!");
  }
  s_loadcellDataQueue = xQueueCreate(1, sizeof(loadcellDataPackage_t));
  if (s_loadcellDataQueue == NULL)
  {
    ActiveSerial->println("Error creating the joystick data queue!");
  }
  s_configUpdateAvailableQueue = xQueueCreate(1, sizeof(configDataPackage_t));
  if (s_configUpdateAvailableQueue == NULL)
  {
    ActiveSerial->println("Error creating the config data queue!");
  }
  s_configUpdateSendToPedalUpdateTaskQueue = xQueueCreate(1, sizeof(configDataPackage_t));
  if (s_configUpdateSendToPedalUpdateTaskQueue == NULL)
  {
    ActiveSerial->println("Error creating the config data queue!");
  }
  s_configUpdateSendToLoadcellTaskQueue = xQueueCreate(1, sizeof(configDataPackage_t));
  if (s_configUpdateSendToLoadcellTaskQueue == NULL)
  {
    ActiveSerial->println("Error creating the config data queue!");
  }
  // configUpdateSendToJoystickTaskQueue= xQueueCreate(1, sizeof(configDataPackage_t));
  // if (configUpdateSendToJoystickTaskQueue == NULL) {
  //     ActiveSerial->println("Error creating the config data queue!");
  // }
  s_configUpdateSendToSerialRXTaskQueue = xQueueCreate(1, sizeof(configDataPackage_t));
  if (s_configUpdateSendToSerialRXTaskQueue == NULL)
  {
    ActiveSerial->println("Error creating the config data queue!");
  }
  s_actionCommandQueue = xQueueCreate(10, sizeof(DapActions_t));
  if (s_actionCommandQueue == NULL)
  {
    ActiveSerial->println("Error creating the action command queue!");
  }
  s_systemControlQueue = xQueueCreate(5, sizeof(uint8_t));
  if (s_systemControlQueue == NULL)
  {
    ActiveSerial->println("Error creating the system control queue!");
  }



  xTaskCreatePinnedToCore(
                    configHandlingTask,   /* Task function. */
                    "configHandlingTask",     /* name of task. */
                    3000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    TASK_PRIORITY_CONFIG_HANDLING_TASK_UBASETYPE,           /* priority of the task */
                    &handle_configHandlingTask,      /* Task handle to keep track of created task */
                    CORE_ID_CONFIG_HANDLING_TASK_U8);          /* pin task to core 1 */  

  

  

  // setup brake resistor pin
  #ifdef BRAKE_RESISTOR_PIN_U8
    pinMode(BRAKE_RESISTOR_PIN_U8, OUTPUT);  // Set GPIO13 as an output
    digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);  // Turn the LED on
  #endif

  #ifdef EMERGENCY_PIN_U8
    pinMode(EMERGENCY_PIN_U8,INPUT_PULLUP);
  #endif

  

  #ifdef USING_LED
    pixels.begin();
    pixels.setBrightness(20);
    pixels.setPixelColor(0,0xff,0xff,0xff);
    pixels.show(); 
  #endif

  #ifdef BUZZER_PIN_U8
    Buzzer.initialized(BUZZER_PIN_U8, 1);
  #else
    Buzzer.initialized(1, 1);
  #endif
  Buzzer.single_beep_tone(770, 100);


  parse_version(DAP_FIRMWARE_VERSION, &g_versionMajor, &g_versionMinor, &g_versionPatch);
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
    pinMode(PAIRING_GPIO_U8, INPUT_PULLUP);
  #endif

  #ifdef USING_LED
    pixels.setPixelColor(0,0xff,0x00,0x00);
    pixels.show(); 
  #endif


  // Load config from EEPROM, if valid, overwrite initial config
  EEPROM.begin(2048);
  global_dap_config_class.loadConfigFromEeprom();
  global_dap_config_class.getConfig(&dap_config_st_eeprom, 500);


  // check validity of data from EEPROM  
  bool structChecker = true;
  uint16_t crc;
  if (dap_config_st_eeprom.payloadHeader_st.payloadType_u8 != DAP_PAYLOAD_TYPE_CONFIG_U8)
  {
    structChecker = false;
    /*ActiveSerial->print("Payload type expected: ");
    ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG_U8);
    ActiveSerial->print(",   Payload type received: ");
    ActiveSerial->println(dap_config_st_local.payloadHeader_st.payloadType_u8);*/
  }
  if (dap_config_st_eeprom.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
  {
    structChecker = false;
    /*ActiveSerial->print("Config version expected: ");
    ActiveSerial->print(DAP_VERSION_CONFIG_U8);
    ActiveSerial->print(",   Config version received: ");
    ActiveSerial->println(dap_config_st_local.payloadHeader_st.version_u8);*/
  }
  // checksum validation
  crc = checksumCalculator_u16((uint8_t*)(&(dap_config_st_eeprom.payloadHeader_st)), sizeof(dap_config_st_eeprom.payloadHeader_st) + sizeof(dap_config_st_eeprom.payloadPedalConfig_st));
  if (crc != dap_config_st_eeprom.payloadFooter_st.checkSum_u16)
  {
    structChecker = false;
    /*ActiveSerial->print("CRC expected: ");
    ActiveSerial->print(crc);
    ActiveSerial->print(",   CRC received: ");
    ActiveSerial->println(dap_config_st_local.payloadFooter_st.checkSum_u16);*/
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
    ActiveSerial->print(DAP_PAYLOAD_TYPE_CONFIG_U8);
    ActiveSerial->print(",   Payload type received: ");
    ActiveSerial->println(dap_config_st_local.payloadHeader_st.payloadType_u8);

    ActiveSerial->print("Target version: ");
    ActiveSerial->print(DAP_VERSION_CONFIG_U8);
    ActiveSerial->print(",    Source version: ");
    ActiveSerial->println(dap_config_st_local.payloadHeader_st.version_u8);

    ActiveSerial->print("CRC expected: ");
    ActiveSerial->print(crc);
    ActiveSerial->print(",   CRC received: ");
    ActiveSerial->println(dap_config_st_local.payloadFooter_st.checkSum_u16);
    //if the config check all failed, reinitialzie _config_st
    ActiveSerial->println("initialized config");
    global_dap_config_class.initializedConfig();
    global_dap_config_class.getConfig(&dap_config_st_local, 500);
  }

  ActiveSerial->println("Config sent successfully");
  // interprete config values
  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);
  //loadcell
  /*
  #ifdef USES_ADS1220
    //Uses ADS1220
    loadcell = new LoadCellAds1220();

  #else
    //Uses ADS1256
    loadcell = new LoadCellAds1256();
  #endif

  loadcell->setLoadcellRating(dap_config_st_local.payloadPedalConfig_st.loadcellRating_u8);
  loadcell->estimateBiasAndVariance();
  */


  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x5f,0x5f,0x00);//yellow
      pixels.show(); 
      //delay(3000);
  #endif

  bool invMotorDir = dap_config_st_local.payloadPedalConfig_st.invertMotorDirection_u8 > 0;
  stepper = new StepperWithLimits(STEP_PIN_STEPPER_U8, DIR_PIN_STEPPER_U8, invMotorDir, dap_calculationVariables_st.stepsPerMotorRevolution_u32, dap_config_st_local.payloadPedalConfig_st.endstopDetectionThreshold_u8); 

  motorRevolutionsPerSteps_fl32 = 1.0f / ( (float)dap_calculationVariables_st.stepsPerMotorRevolution_u32 );
  // ActiveSerial->printf("Steps per motor revolution: %d\n", dap_calculationVariables_st.stepsPerMotorRevolution_u32);

  #ifdef USES_ADS1220
    //Uses ADS1220
    loadcell = new LoadCellAds1220();

  #else
    //Uses ADS1256
    loadcell = new LoadCellAds1256();
  #endif

  loadcell->setLoadcellRating(dap_config_st_local.payloadPedalConfig_st.loadcellRating_u8);
  loadcell->estimateBiasAndVariance();       // automatically identify sensor noise for KF parameterization

	// find the min & max endstops
  ActiveSerial->println("Start homing");
  stepper->findMinMaxSensorless(dap_config_st_local);
  ActiveSerial->print("Min Position is "); ActiveSerial->println(stepper->getLimitMin());
  ActiveSerial->print("Max Position is "); ActiveSerial->println(stepper->getLimitMax());


  // setup Kalman filters
  // ActiveSerial->print("Given loadcell variance: ");
  // ActiveSerial->println(loadcell->getVarianceEstimate(), 5);
  kalman = new KalmanFilter1stOrder(loadcell->getVarianceEstimate());
  kalman_joystick =new KalmanFilter1stOrder(0.1f);
  kalman_2nd_order = new KalmanFilter2ndOrder(loadcell->getVarianceEstimate());


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
  xQueueSend(s_configUpdateAvailableQueue, &dap_config_st_local, portMAX_DELAY);


  // setup multi tasking
  s_semaphore_updatePedalStates = xSemaphoreCreateMutex();



  delay(10);

  // disableCore0WDT();
  // disableCore1WDT();

  ActiveSerial->println("Starting other tasks");

  // Register tasks
  addScheduledTask(pedalUpdateTask, "pedalUpdateTask", REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64, TASK_PRIORITY_PEDAL_UPDATE_TASK_UBASETYPE, CORE_ID_PEDAL_UPDATE_TASK_U8, 7000);
  addScheduledTask(serialCommunicationTaskRx, "serComRx", REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US_I64, TASK_PRIORITY_SERIALCOMMUNICATION_TASK_UBASETYPE, CORE_ID_SERIAL_COMMUNICATION_TASK_U8, 6000);

  // === Scheduler timer (hw_timer, ISR dispatch) ===
  // timerBegin(1000000) ? 1 MHz counter, 1 tick = 1 µs.
  // timerAlarm fires every BASE_TICK_US ticks (= BASE_TICK_US µs), auto-reload.
  timer0 = timerBegin(1000000);
  timerAttachInterrupt(timer0, &onTimer);
  timerAlarm(timer0, BASE_TICK_US, true, 0);


	// the serialCommunicationTaskTx does not need a dedicated timer, since it triggered by queue 
  xTaskCreatePinnedToCore(
      serialCommunicationTaskTx,      /* Task function. */
      "serComTx",    /* name of task. */
      2000,                           /* Stack size of task */
      NULL,                           /* parameter of the task */
      TASK_PRIORITY_SERIALCOMMUNICATION_TX_TASK_UBASETYPE,                              /* priority of the task (e.g., 2, slightly higher than producer) */
      &handle_serialCommunicationTx,  /* Task handle */
      CORE_ID_SERIAL_COMMUNICATION_TASK_U8); /* pin task to core */

  // the joystickOutputTask does not need a dedicated timer, since it triggered by queue 
#ifdef USB_JOYSTICK
  xTaskCreatePinnedToCore(
      joystickOutputTask,      /* Task function. */
      "joystickOutputTask",    /* name of task. */
      4000,                           /* Stack size of task */
      NULL,                           /* parameter of the task */
      TASK_PRIORITY_JOYSTICKOUTPUT_TASK_UBASETYPE,                              /* priority of the task (e.g., 2, slightly higher than producer) */
      &handle_joystickOutput,  /* Task handle */
      CORE_ID_JOYSTICK_TASK_U8); /* pin task to core */
#endif
  // the loadcell task does not need a dedicated timer, since it blocks by DRDY ready ISR
  xTaskCreatePinnedToCore(
                    loadcellReadingTask,   /* Task function. */
                    "loadcellReadingTask",     /* name of task. */
                    1500,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    TASK_PRIORITY_LOADCELL_READING_TASK_UBASETYPE,           /* priority of the task */
                    &handle_loadcellReadingTask,      /* Task handle to keep track of created task */
                    CORE_ID_LOADCELLREADING_TASK_U8);          /* pin task to core 1 */  

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
                    TASK_PRIORITY_PROFILER_TASK_UBASETYPE,           /* priority of the task */
                    &handle_profilerTask,      /* Task handle to keep track of created task */
                    CORE_ID_PROFILER_TASK_U8);          /* pin task to core 1 */

  xTaskCreatePinnedToCore(
                    miscTask,   
                    "miscTask", 
                    2000,  
                    NULL,      
                    TASK_PRIORITY_MISC_TASK_UBASETYPE,         
                    &handle_miscTask,    
                    CORE_ID_MISC_TASK_U8);     


                    
 
  #ifdef SERIAL_PATTERN_DETECTOR

    // --- ADD: Create the serialCommunicationTask as a standalone task ---
  // Create the queue to hold incoming serial packets
  s_serial_packet_queue = xQueueCreate(10, sizeof(UartPacket_t)); // Queue can hold 10 packets


  // This prevents the "UART driver already installed" error.
  uart_driver_delete(UART_NUM_0);
  
  // --- MODIFIED: Install driver over the existing UART0 ---
  // Note: This will reconfigure the port used by the Arduino `Serial` object.
  esp_err_t err = uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE_U32 * 2, 0, 20, &s_uart_queue, 0);
  if (err != ESP_OK) {
    ActiveSerial->printf("Failed to install UART driver: %d\n", err);
    return;
  }

  // Configure UART parameters
  // SERIAL_8N1
  uart_config_t uart_config = {
      .baud_rate = BAUD_3M_U32,
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
  uart_enable_pattern_det_baud_intr(UART_NUM_0, EOF_BYTE_1_U8, 1, SERIAL_PATTERN_DETECTION_TIMEOUT_IN_US, 0, 0);

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
    switch(dap_config_st_local.payloadPedalConfig_st.pedalType_u8)
    {
      case 0:
        g_apHost_pc=new char[strlen("FFBPedalClutch") + 1];
        strcpy(g_apHost_pc, "FFBPedalClutch");
        //APhost="FFBPedalClutch";
        break;
      case 1:
        g_apHost_pc=new char[strlen("FFBPedalBrake") + 1];
        strcpy(g_apHost_pc, "FFBPedalBrake");
        //APhost="FFBPedalBrake";
        break;
      case 2:
        g_apHost_pc=new char[strlen("FFBPedalGas") + 1];
        strcpy(g_apHost_pc, "FFBPedalGas");
        //APhost="FFBPedalGas";
        break;
      default:
        g_apHost_pc=new char[strlen("FFBPedal") + 1];
        strcpy(g_apHost_pc, "FFBPedal");
        //APhost="FFBPedal";
        break;        
    }                    
    addScheduledTask(otaUpdateTask, "OTATask", REPETITION_INTERVAL_OTA_TASK_IN_US_I64, TASK_PRIORITY_OTA_TASK_UBASETYPE, CORE_ID_OTA_TASK_U8, 16000);
    delay(200);
  #endif
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
      pixels.show(); 
      //delay(3000);
  #endif

  //print pedal role assignment
  if(dap_config_st_local.payloadPedalConfig_st.pedalType_u8!=PEDAL_ID_UNKNOWN)
  {
    ActiveSerial->print("Pedal Assignment: ");
    ActiveSerial->println(dap_config_st_local.payloadPedalConfig_st.pedalType_u8);
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
      dap_config_st_local.payloadPedalConfig_st.pedalType_u8 = dap_assignement_reg.deviceId_u8;
    }
    else
    {
      dap_config_st_local.payloadPedalConfig_st.pedalType_u8 = PEDAL_ID_UNKNOWN;
    }
  #endif
  #ifdef PEDAL_HARDWARE_ASSIGNMENT
    pinMode(CFG1_U8, INPUT_PULLUP);
    pinMode(CFG2_U8, INPUT_PULLUP);
    delay(50); // give the pin time to settle
    ActiveSerial->println("Overriding Pedal Role Assignment from Hardware switch......");
    uint8_t CFG1_reading = digitalRead(CFG1_U8);
    uint8_t CFG2_reading = digitalRead(CFG2_U8);
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
        DapConfig_t tmp;
        global_dap_config_class.getConfig(&tmp, 500);
        tmp.payloadPedalConfig_st.pedalType_u8 = Pedal_assignment;
        dap_config_st_local.payloadPedalConfig_st.pedalType_u8 = Pedal_assignment;
        // global_dap_config_class.setConfig(tmp);

        configDataPackage_t configPackage_st;
        configPackage_st.config_st = tmp;
        xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
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
    dap_calculationVariables_st.rudderBrakeStatus_b=false;
    ActiveSerial->println("Starting ESP now tasks");
    ESPNow_initialize();
    ActiveSerial->println("ESPNOW initialized, add task in");
    addScheduledTask(espNowCommunicationTaskTx, "ESPNOW_update_Task", REPETITION_INTERVAL_ESPNOW_TASK_IN_US_I64, TASK_PRIORITY_ESPNOW_TASK_UBASETYPE, CORE_ID_ESPNOW_TASK_U8, 10000);
    ActiveSerial->println("ESPNOW task added");
    delay(500);
  #endif

  #if defined(CONTROLLER_SPECIFIC_VIDPID) && defined(USB_JOYSTICK) && !defined(USE_CDC_INSTEAD_OF_UART)
      ActiveSerial->println("Setup Controller");
      SetupController_USB(dap_config_st_local.payloadPedalConfig_st.pedalType_u8);
      delay(500);
  #endif
  #ifdef ESPNOW_Enable
      //print out basic pedal info via espnow
      sendESPNOWLog("Pedal:%d DAP version: %d", dap_config_st_local.payloadPedalConfig_st.pedalType_u8, DAP_VERSION_CONFIG_U8);
      delay(2);
      #ifdef LOWER_WIFI_TRANSMISSION_POWER
        sendESPNOWLog("Pedal:%d WIFI TX Power set to 8.5dBm", dap_config_st_local.payloadPedalConfig_st.pedalType_u8);
        delay(2);
      #endif
      sendESPNOWLog("Pedal:%d Control Board: %s, Firmware: %s", dap_config_st_local.payloadPedalConfig_st.pedalType_u8, CONTROL_BOARD, DAP_FIRMWARE_VERSION);
      delay(2);
      sendESPNOWLog("Pedal:%d Servo Voltage: %.0f V, Rail pitch set to %d mm.",dap_config_st_local.payloadPedalConfig_st.pedalType_u8, (float)stepper->getServosVoltage()/10.0f , dap_config_st_local.payloadPedalConfig_st.spindlePitch_mmPerRev_u8);
      delay(2);
      sendESPNOWLog("Pedal:%d Loadcell shifting: %.3f kg, Stdev: %.4f",dap_config_st_local.payloadPedalConfig_st.pedalType_u8, loadcell->getBiasEstimate(),loadcell->getStandardDeviationEstimate());
      delay(2);
      sendESPNOWLog("Pedal:%d Min pos: %d, Max pos: %d, Current pos: %d",dap_config_st_local.payloadPedalConfig_st.pedalType_u8, stepper->getLimitMin(), stepper->getLimitMax(), stepper->getCurrentPosition());
      delay(2);
      sendESPNOWLog("Pedal:%d Setup end.",dap_config_st_local.payloadPedalConfig_st.pedalType_u8);
  #endif
  ActiveSerial->println("Setup end");
  #ifdef USING_LED
      //pixels.setBrightness(20);
      pixels.setPixelColor(0,0x00,0xff,0x00);//Green
      pixels.show(); 
      //delay(3000);
  #endif
  
  // set brake resistor voltage
  float servoOperationVoltageInVolt_fl32 = stepper->getBrakeResistorActivationVoltage();
  brakeController.setVoltageThreshold(servoOperationVoltageInVolt_fl32);


  Buzzer.InitializedSound((int)dap_config_st_local.payloadPedalConfig_st.pedalType_u8);

}




/**********************************************************************************************/
/*                                                                                            */
/*                         Calc update function                                               */
/*                                                                                            */
/**********************************************************************************************/
void updatePedalCalcParameters(const DapConfig_t& newConfig)
{
  DapConfig_t dap_config_st_local;  
  global_dap_config_class.getConfig(&dap_config_st_local, 500);

  dap_calculationVariables_st.updateFromConfig(dap_config_st_local);
  dap_calculationVariables_st.updateEndstops(stepper->getLimitMin(), stepper->getLimitMax());
  stepper->updatePedalMinMaxPos(dap_config_st_local.payloadPedalConfig_st.pedalStartPosition_u8, dap_config_st_local.payloadPedalConfig_st.pedalEndPosition_u8);
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
        if (pxPreviousTaskArray != NULL)
        {
            vPortFree(pxPreviousTaskArray);
        }
        pxPreviousTaskArray = pxCurrentTaskArray;
        ulPreviousTotalRunTime = ulCurrentTotalRunTime;
        uxPreviousArraySize = uxCurrentArraySize;
    }
    else
    {
        ActiveSerial->println("Failed to allocate memory for task stats.");
    }
}

void profilerTask( void * pvParameters )
{
  for (;;)
  {
    // copy global struct to local for faster and safe executiion
    DapConfig_t dap_config_profilerTask_st;
    global_dap_config_class.getConfig(&dap_config_profilerTask_st, 500);

    // activate profiler depending on pedal config
    if (dap_config_profilerTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_NET_RUNTIME_U8) 
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

  static DRAM_ATTR DapStateExtended_t dap_state_extended_st_lcl_pedalUpdateTask;
  static DRAM_ATTR DapStateBasic_t dap_state_basic_st_lcl_pedalUpdateTask;
  static DRAM_ATTR DapConfig_t dap_config_pedalUpdateTask_st;

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
  EffectOffsets_t effectOffsets_st;
  EndstopBehavior_t endstopBehavior_st; 
  RudderOffsets_t rudderOffsets_st;
  int32_t Position_effect;
  int32_t bpTriggerValue_u8;
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

  static const uint8_t joystickSendCounterMax_u8 = (REPETITION_INTERVAL_JOYSTICKOUTPUT_TASK_IN_US_I64) / (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) ;
  static const uint8_t serialSendCounterMax_u8 = (REPETITION_INTERVAL_SERIALCOMMUNICATION_TASK_IN_US_I64) / (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) ;

  
  

  
  static float changeVelocity = 0.0f;
  static float normalizedPedalReading_fl32 = 0.0f;
  static float stepperPosFraction_fl32 = 0.0f;
  static bool sendBasicFlag_b = false;
  static bool sendExtendedFlag_b = false;
  static int32_t stepperPosCurrent_i32;
  static uint32_t cycleCount_u32 = 0;

  bool local_systemIdentificationMode_b = false;
  bool local_OTA_status_b = false;

  AdmittanceDebugState_t admittanceDebugInfo_st;
  AdmittanceStates_t admittanceStates_st;
  
  for (;;)
  {


    // trigger task 
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {
DapActions_t incomingAction;
      if (xQueueReceive(s_actionCommandQueue, &incomingAction, 0) == pdPASS) {
        if (incomingAction.payloadPedalAction_st.triggerAbs_u8 > 0) {
          absOscillation.trigger();
          dap_calculationVariables_st.trackCondition_u8 =
              (incomingAction.payloadPedalAction_st.triggerAbs_u8 > 1)
              ? (incomingAction.payloadPedalAction_st.triggerAbs_u8 - 1) : 0;
        }
        _RPMOscillation.rpmValue_fl32 = incomingAction.payloadPedalAction_st.rpm_u8;
        gForceEffect_.gValue_fl32     = incomingAction.payloadPedalAction_st.gValue_u8 - 128;
        if (incomingAction.payloadPedalAction_st.wheelSlip_u8) _WSOscillation.trigger();
        if (!dap_calculationVariables_st.rudderStatus_b) {
          roadImpactEffect_.roadImpactValue_u8 = incomingAction.payloadPedalAction_st.impactValue_u8;
        }
        if (incomingAction.payloadPedalAction_st.startSystemIdentification_u8) {
          local_systemIdentificationMode_b = true;
        }
        if (incomingAction.payloadPedalAction_st.triggerCv1_u8) customVibration1_.trigger();
        if (incomingAction.payloadPedalAction_st.triggerCv2_u8) customVibration2_.trigger();
        if (incomingAction.payloadPedalAction_st.triggerCv3_u8) customVibration3_.trigger();
        if (incomingAction.payloadPedalAction_st.triggerCv4_u8) customVibration4_.trigger();
      }

      uint8_t systemControlEvent;
      if (xQueueReceive(s_systemControlQueue, &systemControlEvent, 0) == pdPASS) {
        if (systemControlEvent == 1) { // 1 = OTA Start / Stop Motor
            local_OTA_status_b = true;
        }
      }
      // if new data package is available, update the local config
      if (xQueueReceive(s_configUpdateSendToPedalUpdateTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS)
      {
        dap_config_pedalUpdateTask_st = configPackage_st.config_st;
    ActiveSerial->printf("[pedalTask] debugFlags0=%d\n",
            dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8);

        // activate profiler depending on pedal config
        if (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER_U8) 
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
        //ActiveSerial->println(dap_config_pedalUpdateTask_st.payloadHeader_st.storeToEeprom_u8);
        saveToEEPRomDuration = millis();
        
        if (true == dap_config_pedalUpdateTask_st.payloadHeader_st.storeToEeprom_u8)
        {
          dap_config_pedalUpdateTask_st.payloadHeader_st.storeToEeprom_u8 = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
          uint16_t crc = checksumCalculator_u16((uint8_t*)(&(dap_config_pedalUpdateTask_st.payloadHeader_st)), sizeof(dap_config_pedalUpdateTask_st.payloadHeader_st) + sizeof(dap_config_pedalUpdateTask_st.payloadPedalConfig_st));
          dap_config_pedalUpdateTask_st.payloadFooter_st.checkSum_u16 = crc;
          global_dap_config_class.setConfig(dap_config_pedalUpdateTask_st);
          ActiveSerial->println("Saving into EEPROM");
          global_dap_config_class.storeConfigToEeprom();
          
          saveToEEPRomDuration = 0;
        }
        
        updatePedalCalcParameters(dap_config_pedalUpdateTask_st); // update the calc parameters
        moveSlowlyToPosition_b = true;

        // enable/disable step loss recovery and crash
        stepper->configSteplossRecovAndCrashDetection(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.stepLossFunctionFlags_u8);
        stepper->configSetProfilingFlag( (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER_U8) );

        // reset all servo alarms
        if ( (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS_U8) )
        {
          ActiveSerial->println("Set clear alarm history flag");
          stepper->clearAllServoAlarms();
          delay(1000); // makes sure the routine has finished

          DapConfig_t tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payloadPedalConfig_st.debugFlags0_u8 &= ( ~(uint8_t)DEBUG_INFO_0_RESET_ALL_SERVO_ALARMS_U8); // clear the debug bit
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
        }

        // reset all servo alarms
        if ( (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_RESET_SERVO_TO_FACTORY_U8) )
        {
          DapConfig_t tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payloadPedalConfig_st.debugFlags0_u8 &= ( ~(uint8_t)DEBUG_INFO_0_RESET_SERVO_TO_FACTORY_U8); // clear the debug bit
          tmp.payloadHeader_st.storeToEeprom_u8 = 1;
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);

          delay(500);

          global_dap_config_class.storeConfigToEeprom();

          ActiveSerial->println("Resetting servo parameters to factory values");
          stepper->resetServoParametersToFactoryValues();
        }

        

        // print all servo parameters for debug purposes
        if ( (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS_U8) )
        {
          DapConfig_t tmp;
          global_dap_config_class.getConfig(&tmp, 500);
          tmp.payloadPedalConfig_st.debugFlags0_u8 &= ( ~(uint8_t)DEBUG_INFO_0_LOG_ALL_SERVO_PARAMS_U8); // clear the debug bit
          //global_dap_config_class.setConfig(tmp);

          configDataPackage_t configPackage_st;
          configPackage_st.config_st = tmp;
          xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);

          delay(1000);  
          stepper->printAllServoParameters();
        }

        // ActiveSerial->printf("Abs ampl.: %0.3f\n", (float)dap_calculationVariables_st.absAmplitude_fl32);
        // ActiveSerial->printf("force range.: %0.3f\n", (float)dap_calculationVariables_st.forceRange_fl32);
        // ActiveSerial->printf("pos range: %0.3f\n", (float)dap_calculationVariables_st.stepperPosRange_fl32);
        
        //bitepoint trigger
        bpTriggerValue_u8 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.bpTriggerValue_u8;
        BP_trigger_min = (bpTriggerValue_u8-4);
        BP_trigger_max = (bpTriggerValue_u8+4);

      }


      // start profiler 0, overall function
      profiler_pedalUpdateTask.start(0);
      

      uint32_t cycleCallTimeInUs_u32 = micros();
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
      dap_calculationVariables_st.setDefaultPos();

      // trigger and compute effects
      absOscillation.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.absPattern_u8, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.absForceOrTarvelBit_u8);
      _RPMOscillation.trigger();
      _RPMOscillation.forceOffset(&dap_calculationVariables_st);
      _BitePointOscillation.forceOffset(&dap_calculationVariables_st);
      gForceEffect_.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.gMulti_u8);
      _WSOscillation.forceOffset(&dap_calculationVariables_st);
      roadImpactEffect_.forceOffset(&dap_calculationVariables_st, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.roadMulti_u8);
      customVibration1_.forceOffset(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvFreq1_u8,dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvAmp1_u8,dap_calculationVariables_st.stepperPosRange_fl32);
      customVibration2_.forceOffset(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvFreq2_u8,dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvAmp2_u8,dap_calculationVariables_st.stepperPosRange_fl32);
      customVibration3_.forceOffset(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvFreq3_u8,dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvAmp3_u8,dap_calculationVariables_st.stepperPosRange_fl32);
      customVibration4_.forceOffset(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvFreq4_u8,dap_config_pedalUpdateTask_st.payloadPedalConfig_st.cvAmp4_u8,dap_calculationVariables_st.stepperPosRange_fl32);     
      if(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.bpTrigger_u8==1)
      {
        if(Position_check > BP_trigger_min)
        {
          if(Position_check < BP_trigger_max)
          {
            _BitePointOscillation.trigger();
          }
        }
      }
      

      if(dap_calculationVariables_st.rudderStatus_b) 
      {
        rudderGForce_.offsetCalculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.updateStepperMaxPos(rudderGForce_.offsetFilter_l);
        _rudder.offsetCalculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.updateStepperMinPos(_rudder.offsetFilter_i32);

      }
      if(dap_calculationVariables_st.helicopterRudderStatus_b) 
      {
        helicopterRudder_.offsetCalculate(&dap_calculationVariables_st);
        dap_calculationVariables_st.updateStepperMinPos(helicopterRudder_.offsetFilter_i32);
      }
      #ifdef ESPNow_debug_rudder
      if(dap_calculationVariables_st.rudderStatus_b || dap_calculationVariables_st.helicopterRudderStatus_b)
      {
        if(millis()-debugMessageLast>500 )
        {
          debugMessageLast=millis();
          ActiveSerial->print("Center offset:");
          ActiveSerial->println(_rudder.offsetFilter_i32);
          ActiveSerial->print("pos min:");
          ActiveSerial->println(dap_calculationVariables_st.softEndstopMinStepperPos_i32);
          ActiveSerial->print("pos max:");
          ActiveSerial->println(dap_calculationVariables_st.softEndstopMaxStepperPos_i32);
          ActiveSerial->print("max Force:");
          ActiveSerial->print("max Force:");
          ActiveSerial->print(dap_calculationVariables_st.forceMax_fl32);
          ActiveSerial->print(" min Force:");
          ActiveSerial->println(dap_calculationVariables_st.forceMin_fl32);
          ActiveSerial->print("Force:");
          ActiveSerial->println(filteredReading);
        }
      }
      #endif

      //_rudder.forceOffsetCalculate(&dap_calculationVariables_st);


      //update max force with G force effect
      g_movingAverageFilter_st.dataPointsCount_i32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.gWindow_u8;
      g_movingAverageFilterRoadImpact_st.dataPointsCount_i32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.roadWindow_u8;
      dap_calculationVariables_st.resetMaxForce();
      dap_calculationVariables_st.forceMax_fl32 += gForceEffect_.gForce_fl32;
      dap_calculationVariables_st.forceMax_fl32 += roadImpactEffect_.roadImpactForce_fl32;
      dap_calculationVariables_st.dynamicUpdate();
      dap_calculationVariables_st.updateStiffness();
    
      // end profiler 1, effects
      profiler_pedalUpdateTask.end(1);

      // read loadcell data, when available
      profiler_pedalUpdateTask.start(2);
      if (xQueueReceive(s_loadcellDataQueue, &loadcellDataReceived_st, (TickType_t)0) == pdPASS) {
        loadcellReading = loadcellDataReceived_st.loadcellReadingInKg_fl32;
      }
      profiler_pedalUpdateTask.end(2);


      // start profiler 3, loadcell reading conversion
      profiler_pedalUpdateTask.start(3);

      // Convert loadcell reading to pedal force
      float sledPosition = sledPositionInMM(stepper, &dap_config_pedalUpdateTask_st, motorRevolutionsPerSteps_fl32);
      float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, &dap_config_pedalUpdateTask_st);
      float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
      // float d_phi_d_x = convertToPedalForceGain(sledPosition, &dap_config_pedalUpdateTask_st);

      // compute gain for horizontal foot model
      float b = (float)dap_config_pedalUpdateTask_st.payloadPedalConfig_st.lengthPedalB_i16;
      float d = (float)dap_config_pedalUpdateTask_st.payloadPedalConfig_st.lengthPedalD_i16;
      float d_x_hor_d_phi = -(float)(b+d) * isin(pedalInclineAngleInDeg_fl32);
      d_x_hor_d_phi *= DEG_TO_RAD_FL32; // inner derivative

      // start profiler 3, loadcell reading conversion
      profiler_pedalUpdateTask.end(3);

      // start profiler 4, loadcell reading filtering
      profiler_pedalUpdateTask.start(4);
      
      // Do the loadcell signal filtering
      float alpha_exp_filter = 1.0f - ( (float)dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfModelNoise_u8) / 5000.0f;
      static float lastPedalForce_fl32 = 0.0f;
      // const velocity model denoising filter
      switch (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfModelOrder_u8) {
        case 0: // const. velocity Kalman filter
          filteredReading = kalman->filteredValue(pedalForce_fl32, 0.0f, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfModelNoise_u8);
          changeVelocity = kalman->changeVelocity();
          break;
        case 1: // const. accel. Kalman filter
          filteredReading = kalman_2nd_order->filteredValue(pedalForce_fl32, 0.0f, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfModelNoise_u8);
          changeVelocity = kalman_2nd_order->changeVelocity();
          break;
        case 2: // exponential filter
          filteredReading_exp_filter = filteredReading_exp_filter * alpha_exp_filter + pedalForce_fl32 * (1.0f-alpha_exp_filter);
          filteredReading = filteredReading_exp_filter;
          changeVelocity = (filteredReading - lastPedalForce_fl32) /  ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 * 1e-6f);
          lastPedalForce_fl32 = filteredReading;
          break;
        default: // No filter
          filteredReading = pedalForce_fl32;
          changeVelocity = (filteredReading - lastPedalForce_fl32) /  ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 * 1e-6f);
          lastPedalForce_fl32 = filteredReading;
      }
      //write filter reading into calculation_st
      dap_calculationVariables_st.currentForceReading_fl32=filteredReading;


      // end profiler 4, loadcell reading filtering
      profiler_pedalUpdateTask.end(4);


      
      float FilterReadingJoystick=0.0f;
      if(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfJoystick_u8==1)
      {
        FilterReadingJoystick=kalman_joystick->filteredValue(filteredReading,0.0f,dap_config_pedalUpdateTask_st.payloadPedalConfig_st.kfModelNoiseJoystick_u8);

      }
      else
      {
        FilterReadingJoystick=filteredReading;
      }


      //if filtered reading > min force, mark the servo was in aciton
      if(filteredReading > dap_config_pedalUpdateTask_st.payloadPedalConfig_st.preloadForce_fl32)
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
      uint32_t pedalIdleTimout = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.servoIdleTimeout_u8 * 60 * 1000; // timeout in ms
      if ((stepper->servoStatus == SERVO_CONNECTED) && ((millis() - servoActionLast) > pedalIdleTimout) && (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.servoIdleTimeout_u8 != 0))
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

      #ifdef EMERGENCY_PIN_U8
        if ((stepper->servoStatus == SERVO_CONNECTED) && (stepper->servoStatus != SERVO_FORCE_STOP) && (digitalRead(EMERGENCY_PIN_U8) == LOW))
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
      float Position_Next_fl32 = 0.0f;

      // Adding effects
      effect_pos_fl32 = 0.0f;
      effect_force_fl32 = 0.0f;
      bool effectsCalculated_b=false;
      // only add force when not at min position 
      if(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.minForceForEffects_u8!=0)
      {
        if(filteredReading >= (float)dap_config_pedalUpdateTask_st.payloadPedalConfig_st.minForceForEffects_u8)
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
        effect_force_fl32 += absOscillation.absOscillationForceOffset_fl32;
        // accumulate position offsets
        effect_pos_fl32 += absOscillation.absOscillationPositionOffset_fl32;
        effect_pos_fl32 += _WSOscillation.wheelSlipOffset_fl32;
        effect_pos_fl32 += _BitePointOscillation.bitePointOffset_fl32;
        effect_pos_fl32 += customVibration1_.customVibrationOffset_fl32;
        effect_pos_fl32 += customVibration2_.customVibrationOffset_fl32;
        effect_pos_fl32 += customVibration3_.customVibrationOffset_fl32;
        effect_pos_fl32 += customVibration4_.customVibrationOffset_fl32;
        effect_pos_fl32 += _RPMOscillation.rpmPositionOffset_i32;
      }
      effect_pos_fl32 *= EFFECT_POSITION_SCALING_FACTOR_FL32;
      // write the effect offsets into a struct for debug output and potential use in other functions like the effect offset PID
      effectOffsets_st.forceOffset_kg_fl32 = effect_force_fl32;
      effectOffsets_st.forceOffset_Steps_fl32 = effect_pos_fl32;


      // chatter reduction gain, reduce the gain when chatter happened
      /*
      if (chatterReduction.checkForChatter(stepperPosCurrent_i32, esp_timer_get_time(), false))
      {
        effect_pos_fl32 *= chatterReduction.DynamicEffectGain();
      }
      */


      // --- Predictive Brake Resistor Activator ---
      // Cache servo state once per cycle; reused in extended debug struct
      // to avoid duplicate ISR-spinlock-contending calls later.
      const int32_t  cached_servosPosError_i32            = stepper->getServosPosError();
      const int32_t  cached_currentSpeedInHz_i32          = stepper->getCurrentSpeedInHz();
      const int16_t  cached_servosVoltage_i16             = stepper->getServosVoltage();
      const int16_t  cached_servosCurrent_i16             = stepper->getServosCurrent();
      const uint32_t cached_servoCycleCounter_u32         = stepper->getServoCycleCounter();
      const int32_t  cached_servosInternalPosCorrected_i32 = stepper->getServosInternalPositionCorrected();
      uint32_t current_time_us = micros();
      bool brake_state = brakeController.Update(
          cached_servosPosError_i32
          , stepper->getServosPosErrorChangeRateInStepsPerSecond()
          , changeVelocity
          , cached_currentSpeedInHz_i32
          , ( (float)cached_servosVoltage_i16 ) * 0.1f
          , current_time_us
      );
      #ifdef BRAKE_RESISTOR_PIN_U8
        if (brake_state) {
            digitalWrite(BRAKE_RESISTOR_PIN_U8, HIGH);
        } else {
            digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);
        }
      #endif

      
      // compute next position with PID strategy
      // MPC control strataegy for rudder
      int32_t positionWithoutEffect=0;
      if(dap_calculationVariables_st.rudderStatus_b || dap_calculationVariables_st.helicopterRudderStatus_b)
      {
        //
        endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.endstopStiffness_kg_mm_u8 * 9.81f; 
        endstopBehavior_st.travelRange_mm_fl32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.endstopTravelRange_mm_u8;
        endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 = constrain(endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32, 0.0f, 500.0f); // constrain the stiffness to a max value for safety
        endstopBehavior_st.travelRange_mm_fl32 = constrain(endstopBehavior_st.travelRange_mm_fl32, 0.0f, 10.0f); // constrain the stiffness to a max value for safety
        

        // rudder variables
        rudderOffsets_st.isRudderMode = false;
        rudderOffsets_st.centerPosition_01 = 0.0f;
        rudderOffsets_st.deadzone_01 = 0.0f;
        rudderOffsets_st.trimOffset_01 = 0.0f;

        // Pedal control algorithm
        Position_Next_fl32 = MoveByAdmittanceStrategy(
          filteredReading
          , stepper
          , &forceCurve
          , &dap_calculationVariables_st
          , &dap_config_pedalUpdateTask_st
          , effectOffsets_st
          , endstopBehavior_st
          , rudderOffsets_st
          , &admittanceDebugInfo_st
          , &admittanceStates_st
        ); 
        // Rudder only
        //Position_Next = MoveByForceTargetingStrategy(filteredReading, stepper, &forceCurve, &dap_calculationVariables_st, &dap_config_pedalUpdateTask_st, 0.0f/*effect_force*/, changeVelocity, d_phi_d_x, d_x_hor_d_phi);
        positionWithoutEffect=Position_Next_fl32;//send the value without rpm effect
        if(effectsCalculated_b)
        {
          //float effectsOffsetFiltered= effectOffsetPID.computeEffectOffset(effect_pos_fl32, &dap_calculationVariables_st);
          //Position_Next -= effectsOffsetFiltered;
          Position_Next_fl32 -= effect_pos_fl32;
        } 
        //if(effectsCalculated_b) Position_Next -= _RPMOscillation.RPM_position_offset;
      }
      else 
      {

        endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.endstopStiffness_kg_mm_u8 * 9.81f; 
        endstopBehavior_st.travelRange_mm_fl32 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.endstopTravelRange_mm_u8;

        endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 = constrain(endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32, 0.0f, 500.0f); // constrain the stiffness to a max value for safety
        endstopBehavior_st.travelRange_mm_fl32 = constrain(endstopBehavior_st.travelRange_mm_fl32, 0.0f, 10.0f); // constrain the stiffness to a max value for safety
        

        // rudder variables
        rudderOffsets_st.isRudderMode = false;
        rudderOffsets_st.centerPosition_01 = 0.0f;
        rudderOffsets_st.deadzone_01 = 0.0f;
        rudderOffsets_st.trimOffset_01 = 0.0f;

        // Pedal control algorithm
        Position_Next_fl32 = MoveByAdmittanceStrategy(
          filteredReading
          , stepper
          , &forceCurve
          , &dap_calculationVariables_st
          , &dap_config_pedalUpdateTask_st
          , effectOffsets_st
          , endstopBehavior_st
          , rudderOffsets_st
          , &admittanceDebugInfo_st
          , &admittanceStates_st
        ); 
        
      }
      // end profiler 4, movement strategy
      profiler_pedalUpdateTask.end(5);

      // start profiler 6, ...
      profiler_pedalUpdateTask.start(6);

      // clip target position to hard endstops
      Position_Next_fl32 = constrain(Position_Next_fl32, stepper->getHardEndstopMinPosition(), stepper->getHardEndstopMaxPosition());
      Position_Next = Position_Next_fl32;
      
      // rudder helper
      Rudder_real_poisiton= 100.0f*((float)(positionWithoutEffect-dap_calculationVariables_st.stepperPosMinDefault_i32) / dap_calculationVariables_st.stepperPosRangeDefault_fl32);

      dap_calculationVariables_st.currentPedalPosition_u32 = positionWithoutEffect;
      dap_calculationVariables_st.currentPedalPositionRatio_fl32=((float)(dap_calculationVariables_st.currentPedalPosition_u32-dap_calculationVariables_st.stepperPosMinDefault_i32))/((float)dap_calculationVariables_st.stepperPosRangeDefault_fl32);
      //Rudder initialzing and de initializing
      #ifdef ESPNOW_Enable
        if(dap_calculationVariables_st.rudderStatus_b)
        {
          if(Rudder_initializing)
          {
            moveSlowlyToPosition_b=true;
            /*
            ActiveSerial->print("current position ratio: ");
            ActiveSerial->print(dap_calculationVariables_st.currentPedalPositionRatio_fl32);
            ActiveSerial->print(" ,currentPedalPosition_u32: ");
            ActiveSerial->print(dap_calculationVariables_st.currentPedalPosition_u32);
            ActiveSerial->print(" ,stepperPosMinDefault_i32: ");
            ActiveSerial->print(dap_calculationVariables_st.stepperPosMinDefault_i32);
            ActiveSerial->print(" ,stepperPosRangeDefault_fl32: ");
            ActiveSerial->println(dap_calculationVariables_st.stepperPosRangeDefault_fl32);
            */
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
                dap_calculationVariables_st.isRudderInitialized_b=true;
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
          dap_calculationVariables_st.isRudderInitialized_b=false;
        }
        //helicopter rudder initialzied
        if(dap_calculationVariables_st.helicopterRudderStatus_b)
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
                dap_calculationVariables_st.isHelicopterRudderInitialized_b=true;
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
          dap_calculationVariables_st.isHelicopterRudderInitialized_b=false;
        }
      #endif

      

      // if pedal in min position, recalibrate position --> automatic step loss compensation
      // stepper->configSteplossRecovAndCrashDetection(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.stepLossFunctionFlags_u8);
      if( (stepper->getLifelineSignal()==true) && (stepper->servoStatus==SERVO_CONNECTED) )
      {
        //if (stepper->isAtMinPos())
        {
          #if defined(OTA_update_ESP32) || defined(OTA_update)
            if(local_OTA_status_b==false)
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
        if(local_OTA_status_b==true)
        {
          doMovement_b = false;
        } 
      #endif

      // Move to new position
      if (doMovement_b)
      {
        if (!moveSlowlyToPosition_b)
        {
          static float Position_Last_fl32 = 0.0f;
          static int32_t s_lastCommandedTarget_i32 = -1;
          static uint32_t s_lastCommandedSpeed_u32 = 0;

          // compute required speed to reach the target position within the next control cycle, so that the movement appears smooth and without delay.
          // float distanceToMove = Position_Next_fl32 - (float)stepperPosCurrent_i32;
          float distanceToMove = Position_Next_fl32 - Position_Last_fl32;
          Position_Last_fl32 = Position_Next_fl32;

          // prevent very small movements, since it will induce pulse frequency 
          // of 1 / (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 * 1e-6) Hz = 4000Hz with sign flips
          float distanceToMoveAbs_fl32 = fabsf(distanceToMove);
          
          if (distanceToMoveAbs_fl32 != 0) 
          {
            float deltaTime_s_fl32 = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
            float requiredSpeed = distanceToMoveAbs_fl32 / deltaTime_s_fl32;

            // slightly overspeed to make sure pulses reach in time
            // requiredSpeed *= 1.1f;

            // Catch-up propotional speed gain
            // Hardware distance (integer steps) for fallback and step-loss checks
            int32_t hardwareDistance_i32 = (int32_t)Position_Last_fl32 - stepper->getCurrentPosition();

            float catchUpSpeedHz = 0.0f;

            // add catchup speed only near standstill & near min endstop
            float targetPosFraction_fl32 = stepper->getCurrentPositionFractionFromExternalPos(Position_Next_fl32 - stepper->getMinPosition() );
            if ( (fabsf(requiredSpeed) < 10) && (targetPosFraction_fl32 <= 0.05f) )
            {
              if (abs(hardwareDistance_i32) > 1) 
              {
                  float catchUpKp = 400.0f; 
                  catchUpSpeedHz = (float)(abs(hardwareDistance_i32) - 1) * catchUpKp; 
              }
            }

            // total speed
            requiredSpeed = requiredSpeed + catchUpSpeedHz;
            
            if (requiredSpeed > (float)MAXIMUM_STEPPER_SPEED_U32) {
                requiredSpeed = (float)MAXIMUM_STEPPER_SPEED_U32;
            }

            stepper->moveToWithSpeed((int32_t)Position_Next_fl32, requiredSpeed);
            s_lastCommandedTarget_i32 = (int32_t)Position_Next_fl32;
            s_lastCommandedSpeed_u32 = requiredSpeed;
  
          }
        }
        else
        {
          moveSlowlyToPosition_b = false;
          stepper->moveSlowlyToPos(Position_Next);
        }
      }
    
      // compute controller output
      dap_calculationVariables_st.stepperPosSetback();
      dap_calculationVariables_st.resetMaxForce();
      dap_calculationVariables_st.dynamicUpdate();
      dap_calculationVariables_st.updateStiffness();
      

      // compute joystick value
      if(dap_calculationVariables_st.rudderStatus_b&&dap_calculationVariables_st.rudderBrakeStatus_b)
      {
        if (1 == dap_config_pedalUpdateTask_st.payloadPedalConfig_st.travelAsJoystickOutput_u8)
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((stepperPosCurrent_i32-dap_calculationVariables_st.stepperPosRange_fl32/2), dap_calculationVariables_st.softEndstopMinStepperPos_i32, dap_calculationVariables_st.softEndstopMinStepperPos_i32+dap_calculationVariables_st.stepperPosRange_fl32/2.0f, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.maxGameOutput_u8);
        }
        else
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue((FilterReadingJoystick/*filteredReading*/), dap_calculationVariables_st.forceMin_fl32, dap_calculationVariables_st.forceMax_fl32, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.maxGameOutput_u8);
        }
      }
      else
      {
        if (1 == dap_config_pedalUpdateTask_st.payloadPedalConfig_st.travelAsJoystickOutput_u8)
        {
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(stepperPosCurrent_i32, dap_calculationVariables_st.softEndstopMinStepperPos_i32, dap_calculationVariables_st.softEndstopMaxStepperPos_i32, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.maxGameOutput_u8);
        }
        else
        {            
          joystickNormalizedToInt32_orig = NormalizeControllerOutputValue(FilterReadingJoystick/*filteredReading*/, dap_calculationVariables_st.forceMin_fl32, dap_calculationVariables_st.forceMax_fl32, dap_config_pedalUpdateTask_st.payloadPedalConfig_st.maxGameOutput_u8);
        }
      }
      joystickfrac=(float)joystickNormalizedToInt32_orig/(float)s_JOYSTICK_MAX_VALUE_U16;
      joystickNormalizedToInt32_eval = forceCurve.EvalJoystickCubicSpline(&dap_config_pedalUpdateTask_st, &dap_calculationVariables_st, joystickfrac);
      
      joystickNormalizedToUInt16 = joystickNormalizedToInt32_eval/100.0f* s_JOYSTICK_MAX_VALUE_U16;
      joystickNormalizedToUInt16 = constrain(joystickNormalizedToUInt16, s_JOYSTICK_MIN_VALUE_U16, s_JOYSTICK_MAX_VALUE_U16);      


      // send joystick data to queue
      if (s_joystickDataQueue != NULL) {

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
          xQueueSend(s_joystickDataQueue, &newJoystickPackage, (TickType_t)0);
        }
      }


      

      // provide joystick output on PIN
      #ifdef Using_analog_output
        int dac_value=(int)(joystickNormalizedToInt32*255/10000);
        dacWrite(DAC_OUTPUT_PIN_U8,dac_value);
      #endif

      #ifdef Using_analog_output_ESP32_S3
        if(MCP_status)
        {
          int dac_value=(int)(joystickNormalizedToInt32*4096*0.9/10000);//limit the max to 5V*0.9=4.5V to prevent the overvolatage
          dac.setVoltage(dac_value, false);
        }
      #endif

      
      
        if ( fabsf(dap_calculationVariables_st.forceRange_fl32) > 0.01f)
      {
          normalizedPedalReading_fl32 = constrain((filteredReading - dap_calculationVariables_st.forceMin_fl32) / dap_calculationVariables_st.forceRange_fl32, 0.0f, 1.0f);
      }
      
      // simulate ABS trigger 
      if(dap_config_pedalUpdateTask_st.payloadPedalConfig_st.simulateAbsTrigger_u8==1)
      {
        ABS_trigger_value=dap_config_pedalUpdateTask_st.payloadPedalConfig_st.simulateAbsValue_u8;
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
      if ( (dap_config_pedalUpdateTask_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT_U8) )
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

      if (s_pedalStateQueue != NULL) {


        if (sendBasicFlag_b)
        {
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalForce_u16 =  normalizedPedalReading_fl32 * 65535.0f;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalPosition_u16 = constrain(stepperPosFraction_fl32, 0.0f, 1.0f) * 65535.0f;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.joystickOutput_u16 = joystickNormalizedToUInt16;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalFirmwareVersion_au8[0] = g_versionMajor;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalFirmwareVersion_au8[1] = g_versionMinor;  
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalFirmwareVersion_au8[2] = g_versionPatch;
          //error code
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.errorCode_u8 = 0;
          //pedal status update
          if(dap_calculationVariables_st.rudderStatus_b)
          {
            if(dap_calculationVariables_st.rudderBrakeStatus_b) dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalStatus_u8=PEDAL_STATUS_RUDDERBRAKE;
            else dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalStatus_u8=PEDAL_STATUS_RUDDER;
          }
          else dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.pedalStatus_u8=PEDAL_STATUS_NORMAL;
          //servo status update
          dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.servoStatus_u8=stepper->servoStatus;

          #ifdef ESPNOW_Enable
            if(g_ESPNow_error_code!=0)
            {
              dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.errorCode_u8=g_ESPNow_error_code;
              g_ESPNow_error_code=0;
            }
          #endif

          if( (stepper->getLifelineSignal()==false) && (stepper->servoStatus!=SERVO_IDLE_NOT_CONNECTED) )
          {
            dap_state_basic_st_lcl_pedalUpdateTask.payloadPedalStateBasic_st.errorCode_u8=12;
          }
          
          //fill the header
          dap_state_basic_st_lcl_pedalUpdateTask.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;

          dap_state_basic_st_lcl_pedalUpdateTask.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_STATE_BASIC_U8;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
          dap_state_basic_st_lcl_pedalUpdateTask.payloadHeader_st.pedalTag_u8 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.pedalType_u8;        
          
        }

        if (sendExtendedFlag_b)
        {
          // update extended pedal structures
          dap_state_extended_st_lcl_pedalUpdateTask.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8; // 170
          dap_state_extended_st_lcl_pedalUpdateTask.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8; // 85

          dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_st.enfOfFrame0_u8 =  EOF_BYTE_0_U8; // 170
          dap_state_extended_st_lcl_pedalUpdateTask.payloadFooter_st.enfOfFrame1_u8 =  EOF_BYTE_1_U8; // 86

          dap_state_extended_st_lcl_pedalUpdateTask.payloadHeader_st.pedalTag_u8 = dap_config_pedalUpdateTask_st.payloadPedalConfig_st.pedalType_u8;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_STATE_EXTENDED_U8;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;


          // update extended struct 
          int32_t minPos = 0; //stepper->getMinPosition();

          // Servo states (values cached earlier in the cycle)
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoStateCycleCount_u32  = cached_servoCycleCounter_u32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoPositionTarget_i32   = cached_servosInternalPosCorrected_i32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoPositionFeedback_i32 = cached_servosInternalPosCorrected_i32 + cached_servosPosError_i32 - minPos;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoPositionError_i16    = cached_servosPosError_i32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoVoltage0p1V_i16      = cached_servosVoltage_i16;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.servoCurrentPercent_i16   = cached_servosCurrent_i16;

          // ESP states
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.timeInUs_u32 = cycleCallTimeInUs_u32;//micros();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.cycleCount_u32 = cycleCount_u32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.pedalForceRaw_fl32 =  loadcellReading;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.pedalForceFiltered_fl32 =  filteredReading;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.forceVelEst_fl32 =  changeVelocity;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.targetPosition_i32   = stepperPosCurrent_i32 - minPos;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.currentSpeedInHz_i32 = cached_currentSpeedInHz_i32;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.brakeResistorState_b = brake_state * 255;//stepper->getBrakeResistorState();
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.oscillationMonitorValue_u8 = 0.0f;
        
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_expectedForce_N = admittanceDebugInfo_st.expectedForce_N;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_isOscillating = admittanceDebugInfo_st.isOscillating;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_admittancePsi_N = admittanceDebugInfo_st.admittancePsi_N;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_virtualMass_kg = admittanceDebugInfo_st.activeVirtualMass_kg + admittanceDebugInfo_st.massAdaptationOffset_kg;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_virtualDamping_Ns_m = admittanceDebugInfo_st.activeDamping_Ns_m;
          
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_virtualPosition_m = admittanceStates_st.physicalPos_m;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_virtualVelocity_mps = admittanceStates_st.virtualVel_mps;
          dap_state_extended_st_lcl_pedalUpdateTask.payloadPedalStateExtended_st.admittance_virtualAcceleration_mps2 = admittanceStates_st.virtualAcc_mps2;

        
        }



          // Package the new state data into a single struct
          PedalStatePackage_t newStatePackage;
          newStatePackage.basic_st = dap_state_basic_st_lcl_pedalUpdateTask;
          newStatePackage.extended_st = dap_state_extended_st_lcl_pedalUpdateTask;
          newStatePackage.sendBasicFlag_b = sendBasicFlag_b;
          newStatePackage.sendExtendedFlag_b = sendExtendedFlag_b;

          // Send the package to the queue. Use a timeout of 0 (non-blocking).
          // If the queue is full, the data is simply dropped. This prevents this
          // high-priority control task from ever blocking on a full serial buffer.
          xQueueSend(s_pedalStateQueue, &newStatePackage, (TickType_t)0);
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
  // static DapConfig_t jut_dap_config_st;
  

  // This task now waits for a complete package of data from the queue.
  joystickDataPackage_t receivedJoystickData;

  
  for (;;)
  {

    // if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0) {
    if (xQueueReceive(s_joystickDataQueue, &receivedJoystickData, portMAX_DELAY) == pdPASS)
    {


      // start profiler 0, overall function
      // profiler_joystickOutputTask.start(0);
      // // if new data package is available, update the local config
      // if (xQueueReceive(configUpdateSendToJoystickTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS) {
      //   jut_dap_config_st = configPackage_st.config_st;
      //   // activate profiler depending on pedal config
      //   if (jut_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER) 
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
          if(dap_calculationVariables_st.rudderStatus_b==false)
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
        case DAP_PAYLOAD_TYPE_CONFIG_U8:
            return sizeof(DapConfig_t);
        case DAP_PAYLOAD_TYPE_ACTION_U8:
            return sizeof(DapActions_t);
        case DAP_PAYLOAD_TYPE_ACTION_OTA_U8:
            return sizeof(DapActionOta_t);
 case DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8:
            return sizeof(DAP_servo_config_st);
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

    static DapConfig_t sct_dap_config_st;

    // Buffer to accumulate incoming serial data
    const size_t RX_BUFFER_SIZE = 1028; // Should be at least 2x the largest possible packet
    static uint8_t rx_buffer[RX_BUFFER_SIZE];
    static size_t buffer_len = 0;

    configDataPackage_t configPackage_st;

    for (;;)
    {
        // Wait for a notification that data might be available
      if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
      {


          // if new data package is available, update the local config
          if (xQueueReceive(s_configUpdateSendToSerialRXTaskQueue, &configPackage_st, (TickType_t)0) == pdPASS)
          {
            sct_dap_config_st = configPackage_st.config_st;

            // activate profiler depending on pedal config
            if (sct_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER_U8) 
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
            if (ActiveSerial->available())
            {
              // Prevent buffer overflow by only reading what fits
              size_t bytesToRead = min((size_t)ActiveSerial->available(), RX_BUFFER_SIZE - buffer_len);
              if (bytesToRead > 0)
              {
                  ActiveSerial->readBytes(&rx_buffer[buffer_len], bytesToRead);
                  buffer_len += bytesToRead;
              }

              // ActiveSerial->println("Serial data available");
          }

          // --- 2. Process all complete packets in the buffer ---
          size_t buffer_idx = 0;
            while (buffer_idx < buffer_len)
            {
              // A. Find the next valid Start-of-Frame (SOF)
              if (rx_buffer[buffer_idx] != SOF_BYTE_0_U8 || (buffer_idx + 1 < buffer_len && rx_buffer[buffer_idx + 1] != SOF_BYTE_1_U8))
              {
                  buffer_idx++;
                  continue; // Keep scanning for a SOF
              }

              // ActiveSerial->println("1st check passed");

              // SOF found at buffer_idx. Check if we have enough data for a header.
                if (buffer_len < buffer_idx + 3)
                {
                  // Not enough data for a full header, stop parsing for now
                  break;
              }

              // B. Get expected packet size from payload type
              uint8_t payloadType = rx_buffer[buffer_idx + 2];
              size_t expectedSize = getExpectedPacketSize(payloadType);

                if (expectedSize == 0)
                {
                  // Unknown payload type, this SOF is corrupt. Skip it and continue scanning.
                  buffer_idx++;
                  continue;
              }

              // ActiveSerial->println("2nd check passed");

              // C. Check if the full packet has arrived
                if (buffer_len < buffer_idx + expectedSize)
                {
                  // Full packet is not yet in the buffer, wait for more data
                  break;
              }

              // D. Check for valid End-of-Frame (EOF)
                if (rx_buffer[buffer_idx + expectedSize - 2] != EOF_BYTE_0_U8 || rx_buffer[buffer_idx + expectedSize - 1] != EOF_BYTE_1_U8)
                {
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
                  case DAP_PAYLOAD_TYPE_CONFIG_U8: {
                      DapConfig_t received_config;
                      memcpy(&received_config, packet_start, sizeof(DapConfig_t));
                      
                      calculated_crc = checksumCalculator_u16((uint8_t*)(&(received_config.payloadHeader_st)), sizeof(received_config.payloadHeader_st) + sizeof(received_config.payloadPedalConfig_st));
                      received_crc = received_config.payloadFooter_st.checkSum_u16;

                        if (calculated_crc != received_crc || received_config.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
                        {
                          structIsValid = false;
                        }
                        else
                        {
                          // --- VALID CONFIG PACKET ---
                        ActiveSerial->println("Updating pedal config from serial");
                        //global_dap_config_class.setConfig(received_config);

                        configDataPackage_t configPackage_st;
                        configPackage_st.config_st = received_config;
                        xQueueSend(s_configUpdateAvailableQueue, &configPackage_st, portMAX_DELAY);
                        
                        if (received_config.payloadHeader_st.storeToEeprom_u8 == 1)
                        {
                          Buzzer.single_beep_tone(700, 100);
                        }
                      }
                      break;
                  }
                  case DAP_PAYLOAD_TYPE_ACTION_U8: {
                      DapActions_t received_action;
                      memcpy(&received_action, packet_start, sizeof(DapActions_t));

                      // ActiveSerial->println("Action received");

                      calculated_crc = checksumCalculator_u16((uint8_t*)(&(received_action.payloadHeader_st)), sizeof(received_action.payloadHeader_st) + sizeof(received_action.payloadPedalAction_st));
                      received_crc = received_action.payloadFooter_st.checkSum_u16;

                        if (calculated_crc != received_crc || received_action.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
                        {
                          structIsValid = false;
                        }
                        else
                        {
                          // --- VALID ACTION PACKET ---
                          // Place your extensive action handling logic here
                          // For clarity, this could be moved to its own function: handleActionPacket(received_action);
                          if (received_action.payloadPedalAction_st.systemAction_u8 == 2)
                          {
                              ActiveSerial->println("ESP restart by user request");
                              ESP.restart();
                          }

                          //3= Wifi OTA
                          #ifdef ESPNOW_Enable
                          if (received_action.payloadPedalAction_st.systemAction_u8==(uint8_t)PedalSystemAction::ENABLE_OTA)
                          {
                            ActiveSerial->println("Get OTA command");
                            g_OTA_enable_b=true;
                            //g_OTA_enable_start=true;
                            g_ESPNow_OTA_enable=false;
                          }
                          #endif
                          //4 Enable pairing
                          if (received_action.payloadPedalAction_st.systemAction_u8==4)
                          {
                            #ifdef ESPNow_Pairing_function
                              ActiveSerial->println("Get Pairing command");
                              software_pairing_action_b=true;
                            #endif
                            #ifndef ESPNow_Pairing_function
                              ActiveSerial->println("no supporting command");
                            #endif
                          }
                          
                          if (received_action.payloadPedalAction_st.systemAction_u8==(uint8_t)PedalSystemAction::ESP_BOOT_INTO_DOWNLOAD_MODE)
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
                          if (received_action.payloadPedalAction_st.systemAction_u8 == (uint8_t)PedalSystemAction::PRINT_PEDAL_INFO)
                          {
                            char logString[200];
                            snprintf(logString, sizeof(logString),
                                    "Pedal ID: %d\nBoard: %s\nLoadcell shift= %.3f kg\nLoadcell variance= %.3f kg\nPSU voltage:%.1f V\nMax endstop:%lu\nCurrentPos:%lu\n\0",
                                     sct_dap_config_st.payloadPedalConfig_st.pedalType_u8, CONTROL_BOARD, loadcell->getBiasEstimate(), loadcell->getStandardDeviationEstimate(), ((float)stepper->getServosVoltage() / 10.0f), dap_calculationVariables_st.stepperPosMaxEndstop_i32, dap_calculationVariables_st.currentPedalPosition_u32);
                            ActiveSerial->println(logString);
                          }

                          // Send action to pedalUpdateTask via Queue
                          xQueueSend(s_actionCommandQueue, &received_action, (TickType_t)0);
                          // trigger return pedal position
                          if (received_action.payloadPedalAction_st.returnPedalConfig_u8)
                          {
                          
                            DapConfig_t * dap_config_st_local_ptr;
                            dap_config_st_local_ptr = &sct_dap_config_st;
                            dap_config_st_local_ptr->payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
                            dap_config_st_local_ptr->payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
                            dap_config_st_local_ptr->payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
                            dap_config_st_local_ptr->payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
                            uint16_t crc = checksumCalculator_u16((uint8_t*)(&(sct_dap_config_st.payloadHeader_st)), sizeof(sct_dap_config_st.payloadHeader_st) + sizeof(sct_dap_config_st.payloadPedalConfig_st));
                            dap_config_st_local_ptr->payloadFooter_st.checkSum_u16 = crc;

                            // suspend the serial Tx task so that data can properly be send
                            vTaskSuspend(handle_serialCommunicationTx);
                            delay(50);
                            ActiveSerial->write((char*)dap_config_st_local_ptr, sizeof(DapConfig_t));
                            ActiveSerial->print("Return pedal config");
                            delay(50);
                            vTaskResume(handle_serialCommunicationTx);

                          }
                          #ifdef ESPNOW_Enable
                            if(received_action.payloadPedalAction_st.rudderAction_u8==1)//Enable Rudder
                            {
                              if(dap_calculationVariables_st.rudderStatus_b==false)
                              {
                                dap_calculationVariables_st.rudderStatus_b=true;
                                ActiveSerial->println("Rudder on");
                                Rudder_initializing=true;
                                moveSlowlyToPosition_b=true;
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
                              }
                              else
                              {
                                dap_calculationVariables_st.rudderStatus_b=false;
                                ActiveSerial->println("Rudder off");
                                Rudder_deinitializing=true;
                                moveSlowlyToPosition_b=true; 

                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
                              }
                            }
                            if(received_action.payloadPedalAction_st.rudderBrakeAction_u8==1)
                            {
                              if(dap_calculationVariables_st.rudderBrakeStatus_b==false&&dap_calculationVariables_st.rudderStatus_b==true)
                              {
                                dap_calculationVariables_st.rudderBrakeStatus_b=true;
                                ActiveSerial->println("Rudder brake on");
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
                              }
                              else
                              {
                                dap_calculationVariables_st.rudderBrakeStatus_b=false;
                                ActiveSerial->println("Rudder brake off");
                                //ActiveSerial->print("status:");
                                //ActiveSerial->println(dap_calculationVariables_st.rudderStatus_b);
                              }
                            }
                            //clear rudder status
                            if(received_action.payloadPedalAction_st.rudderAction_u8==2)
                            {
                              dap_calculationVariables_st.rudderStatus_b=false;
                              dap_calculationVariables_st.rudderBrakeStatus_b=false;
                              ActiveSerial->println("Rudder Status Clear");
                              Rudder_deinitializing=true;
                              moveSlowlyToPosition_b=true;

                            }
                          #endif
                          
                          

                          
                      }
                      break;
                  }
                  case DAP_PAYLOAD_TYPE_ACTION_OTA_U8:{
                    memcpy(&dap_action_ota_st, packet_start, sizeof(DapActionOta_t));
                    ActiveSerial->println("Get OTA command");
                    buzzerBeepAction_b=true;

                    //ActiveSerial->readBytes((char*)&dap_action_ota_st, sizeof(DapActionOta_t));
                    #ifdef OTA_update
                      if(dap_action_ota_st.payloadHeader_st.payloadType_u8==DAP_PAYLOAD_TYPE_ACTION_OTA_U8)
                      {
                        if(dap_action_ota_st.payloadOtaInfo_st.deviceId_u8 == sct_dap_config_st.payloadPedalConfig_st.pedalType_u8)
                        {
                          g_SSID=new char[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8+1];
                          g_PASS=new char[dap_action_ota_st.payloadOtaInfo_st.passLength_u8+1];
                          memcpy(g_SSID,dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8,dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8);
                          memcpy(g_PASS,dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8,dap_action_ota_st.payloadOtaInfo_st.passLength_u8);
                          g_SSID[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8]=0;
                          g_PASS[dap_action_ota_st.payloadOtaInfo_st.passLength_u8]=0;
                          g_OTA_enable_b=true;
                          g_OTA_enable_start=true;
                          #ifdef ESPNOW_Enable
                            g_ESPNow_OTA_enable=false;
                          #endif
                        }
                      }
                    #else
                      ActiveSerial->println("The command is not supported");
                    #endif    
                    break;
                }
case DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8: {
                      DAP_servo_config_st received_servo_config;
                      memcpy(&received_servo_config, packet_start, sizeof(DAP_servo_config_st));

                      calculated_crc = checksumCalculator_u16((uint8_t*)(&(received_servo_config.payloadHeader_st)), sizeof(received_servo_config.payloadHeader_st) + sizeof(received_servo_config.payloadServoConfig_st));
                      received_crc = received_servo_config.payloadFooter_st.checkSum_u16;

                      if (calculated_crc != received_crc || received_servo_config.payloadHeader_st.version_u8 != DAP_VERSION_CONFIG_U8)
                      {
                          structIsValid = false;
                      }
                      else
                      {
                          ActiveSerial->println("Valid servo config packet received");
                          uint8_t validFields = received_servo_config.payloadServoConfig_st.numValidFields;
                          if (validFields > 10) validFields = 10;

                          if (received_servo_config.payloadServoConfig_st.readWriteFlag == 1) {
                              // Write
                              for (uint8_t i = 0; i < validFields; i++) {
                                  uint16_t addr = received_servo_config.payloadServoConfig_st.registerAddresses[i];
                                  uint16_t val = received_servo_config.payloadServoConfig_st.registerValues[i];
                                  if (stepper != nullptr) {
                                      ServoModbusCmd_t cmd = {};
                                      cmd.startAddr_u16 = addr;
                                      cmd.count_u8      = 1;   // single register per packet
                                      cmd.isWrite_b     = true;
                                      cmd.values[0] = (int16_t)(val & 0xFFFF);
                                      stepper->scheduleServoModbusCmd(cmd);
                                  }
                                  ActiveSerial->printf("Servo config write: addr=0x%X, val=%u\n", addr, val);
                                  // Add small delay to avoid overwriting pending command too quickly
                                  vTaskDelay(pdMS_TO_TICKS(10));
                              }
                          } else {
                              // Read
                              // Assuming validFields > 0, we schedule the first requested register.
                              // If multiple sequential registers are requested, we could optimize,
                              // but here we simply read from the first address for 'validFields' count.
                              if (validFields > 0 && stepper != nullptr) {
                                  uint16_t addr = received_servo_config.payloadServoConfig_st.registerAddresses[0];
                                  ServoModbusCmd_t cmd = {};
                                  cmd.startAddr_u16 = addr;
                                  cmd.count_u8      = validFields;
                                  if (cmd.count_u8 > 8) cmd.count_u8 = 8; // Modbus limit
                                  cmd.isWrite_b     = false;
                                  stepper->scheduleServoModbusCmd(cmd);
                                  ActiveSerial->printf("Servo config read scheduled: addr=0x%X, count=%u\n", addr, cmd.count_u8);
                              }
                          }
                      }
                      break;
                  }

                  
              } // end switch

                if (!structIsValid)
                {
                  ActiveSerial->printf("Invalid packet detected (Type: %d). Skipping SOF.\n", payloadType);
                  buffer_idx++; // Skip the failed SOF and continue scanning
                }
                else
                {
                  // Packet was valid and processed, advance index past this packet
                  buffer_idx += expectedSize;
              }
          } // end while

          // --- 3. Clean up the buffer ---
              if (buffer_idx > 0)
              {
              size_t remaining_len = buffer_len - buffer_idx;
                if (remaining_len > 0)
                {
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

  // static DapConfig_t sct_dap_config_st;

  // This task now waits for a complete package of data from the queue.
  PedalStatePackage_t receivedState;

  for (;;)
  {

    // global_dap_config_class.getConfig(&sct_dap_config_st, 0);

    // Block indefinitely until a new state package arrives from pedalUpdateTask.
    // This is now the ONLY trigger for this task.
    if (xQueueReceive(s_pedalStateQueue, &receivedState, portMAX_DELAY) == pdPASS)
    {
      
      // Now, process the first item, and then enter a loop to
      // empty the rest of the queue.
      do {
                
        // Copy to a local variable to calculate CRC
        DapStateBasic_t basic_to_send = receivedState.basic_st;
        DapStateExtended_t extended_to_send = receivedState.extended_st;



  // Provide pedal states to ESPnow task
  #ifdef ESPNOW_Enable
        // update pedal states
        if (s_semaphore_updatePedalStates != NULL)
        {
          if (xSemaphoreTake(s_semaphore_updatePedalStates, (TickType_t)0) == pdTRUE)
          {
            // move local structure values to global structures
            dap_state_basic_st = basic_to_send;
            dap_state_extended_st = extended_to_send;

            // release semaphore
            xSemaphoreGive(s_semaphore_updatePedalStates);
          }
        }
        else
        {
          s_semaphore_updatePedalStates = xSemaphoreCreateMutex();
        }
  #endif



        // activate profiler depending on pedal config
        // if (sct_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER) 
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
          basic_to_send.payloadFooter_st.checkSum_u16 = checksumCalculator_u16((uint8_t*)(&(basic_to_send.payloadHeader_st)), sizeof(basic_to_send.payloadHeader_st) + sizeof(basic_to_send.payloadPedalStateBasic_st));
          ActiveSerial->write((char*)&basic_to_send, sizeof(DapStateBasic_t));
        }

        // send extended pedal state struct
        if (receivedState.sendExtendedFlag_b)
        {
          // update CRC before transmission
          extended_to_send.payloadFooter_st.checkSum_u16 = checksumCalculator_u16((uint8_t*)(&(extended_to_send.payloadHeader_st)), sizeof(extended_to_send.payloadHeader_st) + sizeof(extended_to_send.payloadPedalStateExtended_st));
          ActiveSerial->write((char*)&extended_to_send, sizeof(DapStateExtended_t));
        }

        // --- Send servo Modbus read result back to SimHub plugin ---
        if (stepper != nullptr) {
          uint16_t addr_u16 = 0;
          int16_t  vals[8]  = {};
          uint8_t  cnt_u8   = 0;
          if (stepper->tryGetServoModbusReadResult(addr_u16, vals, cnt_u8) && cnt_u8 > 0) {
            DAP_servo_config_st_t resp = {};
            resp.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
            resp.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
            resp.payloadHeader_st.payloadType_u8   = DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8;
            resp.payloadHeader_st.version_u8       = DAP_VERSION_CONFIG_U8;
            resp.payloadHeader_st.storeToEeprom_u8 = 0;
            resp.payloadHeader_st.pedalTag_u8      = 0;
            resp.payloadServoConfig_st.readWriteFlag  = 0; // read response
            resp.payloadServoConfig_st.numValidFields = cnt_u8;
            for (uint8_t i = 0; i < cnt_u8; i++) {
              resp.payloadServoConfig_st.registerAddresses[i] = addr_u16 + i;
              resp.payloadServoConfig_st.registerValues[i]    = (uint16_t)vals[i];
            }
            resp.payloadFooter_st.checkSum_u16 = checksumCalculator_u16(
              (uint8_t*)(&resp.payloadHeader_st),
              sizeof(resp.payloadHeader_st) + sizeof(resp.payloadServoConfig_st));
            resp.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
            resp.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
            ActiveSerial->write((char*)&resp, sizeof(DAP_servo_config_st_t));
          }
        }

        // profiler_serialCommunicationTask.end(0);

        // // print profiler results
        // profiler_serialCommunicationTask.report();
      // Continue looping with a zero timeout to process any other items that are
      // already in the queue. The loop will exit when the queue is empty.
      } while (xQueueReceive(s_pedalStateQueue, &receivedState, (TickType_t)0) == pdPASS);


      // force a context switch
      // taskYIELD();
    }
  }
}


//OTA multitask
void otaUpdateTask( void * pvParameters )
{
  uint16_t OTA_count = 0;
  unsigned long ota_debug_messaage_last = 0;
  bool message_out_b = false;
  int OTA_update_status = 99;

  for (;;)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) > 0)
    {

      if (OTA_count > 200)
      {
        message_out_b = true;
        OTA_count = 0;
      }
      else
      {
        OTA_count++;
      }

      #if defined(OTA_update)  || defined(OTA_update_ESP32)
      if(g_OTA_enable_b)
      {
        if(message_out_b)
        {
          message_out_b=false;
          Serial1.println("OTA enable flag on");
        }
        if(g_OTA_status)
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
              if(dap_action_ota_st.payloadOtaInfo_st.otaAction_u8==OTA_ACTION_PLATFORMIO_DIRECT_UPLOAD)
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
            g_ESPNow_initial_status=false;
            g_ESPNOW_status=false;
          #else
            result = ESP_OK;
          #endif
          //result = ESP_OK;
          delay(3000);
          if(result==ESP_OK)
          {
            g_OTA_status=true;
 // notify pedal task to stop movement
            uint8_t ota_event = 1;
            xQueueSend(s_systemControlQueue, &ota_event, (TickType_t)0);
            Buzzer.single_beep_tone(700,100); 
            delay(1000);
            #ifdef OTA_update_ESP32
            ota_wifi_initialize(g_apHost_pc);
            #endif
            #ifdef USING_LED
                //pixels.setBrightness(20);
                pixels.setPixelColor(0,0x00,0x00,0xff);//Blue
                pixels.show(); 
                //delay(3000);
            #endif
            #ifdef OTA_update
            wifi_initialized(g_SSID,g_PASS);
            delay(2000);
            //sendESPNOWLog("Wifi Connected");
            if(dap_action_ota_st.payloadOtaInfo_st.otaAction_u8!=OTA_ACTION_PLATFORMIO_DIRECT_UPLOAD)
            {
              ESP32OTAPull ota;
              int ret;
              ota.SetCallback(OTAcallback);
              ota.OverrideBoard(CONTROL_BOARD);
              char* versionTag_pc;
              if(dap_action_ota_st.payloadOtaInfo_st.otaAction_u8==OTA_ACTION_FORCE_UPDATE)
              {
                const char* versionTagSource_pc;
                if(PCB_VERSION==3||PCB_VERSION==5||PCB_VERSION==9) versionTagSource_pc ="0.90.16";// for those board which change the partition table
                else versionTagSource_pc ="0.0.0";
                versionTag_pc=new char[strlen(versionTagSource_pc) + 1];
                strcpy(versionTag_pc, versionTagSource_pc);
                ActiveSerial->println("Force update");
              }
              if(dap_action_ota_st.payloadOtaInfo_st.otaAction_u8==OTA_ACTION_NORMAL)
              {
                versionTag_pc=new char[strlen(DAP_FIRMWARE_VERSION) + 1];
                strcpy(versionTag_pc, DAP_FIRMWARE_VERSION);
                //version_tag=DAP_FIRMWARE_VERSION;
              }
              switch (dap_action_ota_st.payloadOtaInfo_st.modeSelect_u8)
              {
                case 1:
                  ActiveSerial->printf("Flashing to latest release, checking %s to see if an update is available...\n", OTA_JSON_URL_MAIN);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_MAIN, versionTag_pc, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                case 2:
                  ActiveSerial->printf("Flashing to latest dev build, checking %s to see if an update is available...\n", OTA_JSON_URL_DEV);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_DEV, versionTag_pc, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                case 3:
                  ActiveSerial->printf("Flashing to test build, checking %s to see if an update is available...\n", OTA_JSON_URL_TEST);
                  ret = ota.CheckForOTAUpdate(OTA_JSON_URL_TEST, versionTag_pc, ESP32OTAPull::UPDATE_BUT_NO_BOOT);
                  ActiveSerial->printf("CheckForOTAUpdate returned %d (%s)\n\n", ret, errtext(ret));
                  OTA_update_status=ret;
                  break;
                default:
                break;
                delete[] versionTag_pc; 
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
  uint basicStateUpdateIntervalBase[3]={5,4,3};
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


        DapConfig_t espnow_dap_config_st;
        global_dap_config_class.getConfig(&espnow_dap_config_st, 500);
        //basic state sendout interval
        uint basicStateUpdateInterval = 8;
        int pedalId=espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8;
        if(pedalId < 3) basicStateUpdateInterval= basicStateUpdateIntervalBase[pedalId];
        if(millis()-basic_state_update_last>basicStateUpdateInterval)
        {
          basic_state_send_b=true;
          basic_state_update_last=millis();
          
        }
        if (espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8 ==PEDAL_ID_UNKNOWN)
        {
          noAssignmentStatus=true;
        }
        //restart from espnow
        if(g_ESPNow_restart)
        {
          ActiveSerial->println("ESP restart by ESPnow request");
          sendESPNOWLog("Pedal:%d restarted by request", espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8);
          delay(2);
          ESP.restart();
        }
        //entend state send out interval
        if((millis()-extend_state_update_last>extendStateUpdateInterval) && espnow_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 == DEBUG_INFO_0_STATE_EXTENDED_INFO_STRUCT_U8)
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
        if (espnow_dap_config_st.payloadPedalConfig_st.debugFlags0_u8 & DEBUG_INFO_0_CYCLE_TIMER_U8) 
        {
          profiler_espNow.activate( true );
        }
        else
        {
          profiler_espNow.activate( false );
        }

        // start profiler 0, overall function
        profiler_espNow.start(0);

        
        if(g_ESPNow_initial_status==false  )
        {
          if(g_OTA_enable_b==false)
          {
            ESPNow_initialize();
          }
          
        }
        else
        {
          #ifdef ESPNow_Pairing_function
          #ifdef Hardware_Pairing_button
            if(digitalRead(PAIRING_GPIO_U8)==LOW)
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
                dap_esppairing_lcl.payloadEspnowInfo_st.deviceId_u8 = espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8;
                dap_esppairing_lcl.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_ESPNOW_PAIRING_U8;
                dap_esppairing_lcl.payloadHeader_st.pedalTag_u8 = espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8;
                dap_esppairing_lcl.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
                crc = checksumCalculator_u16((uint8_t*)(&(dap_esppairing_lcl.payloadHeader_st)), sizeof(dap_esppairing_lcl.payloadHeader_st) + sizeof(dap_esppairing_lcl.payloadEspnowInfo_st));
                dap_esppairing_lcl.payloadFooter_st.checkSum_u16=crc;
              }
              if(now-Pairing_state_last_sending>400)
              {
                Pairing_state_last_sending=now;
                ESPNow.send_message(g_broadcast_mac,(uint8_t *) &dap_esppairing_lcl, sizeof(dap_esppairing_lcl));
              }

              

              //timeout check
              if(now-Pairing_state_start>Pairing_timeout)
              {
                ESPNow_pairing_action_b=false;
                ActiveSerial->print("Pedal: ");
                ActiveSerial->print(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8);
                ActiveSerial->println(" timeout.");
                Buzzer.single_beep_tone(700,100);
                if(g_UpdatePairingToEeprom)
                {
                  EEPROM.put(EEPROM_OFFSET_U32,_ESP_pairing_reg);
                  EEPROM.commit();
                  g_UpdatePairingToEeprom=false;
                  //list eeprom
                  ESP_pairing_reg ESP_pairing_reg_local;
                  EEPROM.get(EEPROM_OFFSET_U32, ESP_pairing_reg_local);
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
                        ESPNow.remove_peer(g_Clu_mac);
                        memcpy(&g_Clu_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                        delay(100);
                        ESPNow.add_peer(g_Clu_mac);
                        
                      }
                      if(i==1)
                      {
                        ESPNow.remove_peer(g_Brk_mac);
                        memcpy(&g_Brk_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                        delay(100);
                        ESPNow.add_peer(g_Brk_mac);
                      }
                      if(i==2)
                      {
                        ESPNow.remove_peer(g_Gas_mac);
                        memcpy(&g_Gas_mac,&_ESP_pairing_reg.Pair_mac[i],6);
                        delay(100);
                        ESPNow.add_peer(g_Gas_mac);
                      }        
                      if(i==3)
                      {
                        ESPNow.remove_peer(g_esp_Host);
                        memcpy(&g_esp_Host,&_ESP_pairing_reg.Pair_mac[i],6);
                        delay(100);
                        ESPNow.add_peer(g_esp_Host);                
                      }        
                      if(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8==1)
                      {
                        memcpy(g_Recv_mac, g_Gas_mac, 6);
                      }
                      if(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8==2)
                      {
                        memcpy(g_Recv_mac, g_Brk_mac, 6);
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
            DapAssignmentBroadcast_t dap_assignmentBoardcast_st;
            dap_assignmentBoardcast_st.payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
            dap_assignmentBoardcast_st.payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
            dap_assignmentBoardcast_st.payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
            dap_assignmentBoardcast_st.payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_ASSIGNMENT_U8;
            dap_assignmentBoardcast_st.payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
            dap_assignmentBoardcast_st.payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
            dap_assignmentBoardcast_st.payloadAssignmentRequest_st.assignmentAction_u8=1;
            memcpy(dap_assignmentBoardcast_st.payloadAssignmentRequest_st.macAddress_au8,g_esp_Mac,6);
            uint16_t crc = 0;
            crc = checksumCalculator_u16((uint8_t *)(&(dap_assignmentBoardcast_st.payloadHeader_st)), sizeof(dap_assignmentBoardcast_st.payloadHeader_st) + sizeof(dap_assignmentBoardcast_st.payloadAssignmentRequest_st));
            dap_assignmentBoardcast_st.payloadFooter_st.checkSum_u16=crc;
            ESPNow.send_message(g_broadcast_mac, (uint8_t *)&dap_assignmentBoardcast_st, sizeof(DapAssignmentBroadcast_t));
          }
          // basic state packet send out  
          if(basic_state_send_b && !noAssignmentStatus)
          {
            // update pedal states
            DapStateBasic_t dap_state_basic_st_lcl;       
            // initialize with zeros in case semaphore couldn't be aquired
            memset(&dap_state_basic_st_lcl, 0, sizeof(dap_state_basic_st_lcl));
            if(s_semaphore_updatePedalStates!=NULL)
            {  
              if(xSemaphoreTake(s_semaphore_updatePedalStates, (TickType_t)5)==pdTRUE) 
              {
                // UPDATE basic pedal state struct
                dap_state_basic_st_lcl = dap_state_basic_st;

                // release semaphore
                xSemaphoreGive(s_semaphore_updatePedalStates);
                dap_state_basic_st_lcl.payloadFooter_st.checkSum_u16 = checksumCalculator_u16((uint8_t*)(&(dap_state_basic_st_lcl.payloadHeader_st)), sizeof(dap_state_basic_st_lcl.payloadHeader_st) + sizeof(dap_state_basic_st_lcl.payloadPedalStateBasic_st));
              }
            }
            else
            {
              s_semaphore_updatePedalStates = xSemaphoreCreateMutex();
            }
            ESPNow.send_message(g_broadcast_mac,(uint8_t *) & dap_state_basic_st_lcl,sizeof(dap_state_basic_st_lcl));
            basic_state_send_b=false;
          }

          profiler_espNow.end(2);

          profiler_espNow.start(3);

          if (extend_state_send_b && !noAssignmentStatus)
          {
            // update pedal states
            DapStateExtended_t dap_state_extended_st_espNow; 
            // initialize with zeros in case semaphore couldn't be aquired
            memset(&dap_state_extended_st_espNow, 0, sizeof(dap_state_extended_st_espNow));
            if(s_semaphore_updatePedalStates!=NULL)
            {  
              if(xSemaphoreTake(s_semaphore_updatePedalStates, (TickType_t)5)==pdTRUE) 
              {
                // UPDATE extended pedal state struct
                dap_state_extended_st_espNow = dap_state_extended_st; 
                // release semaphore
                xSemaphoreGive(s_semaphore_updatePedalStates);
                dap_state_extended_st_espNow.payloadFooter_st.checkSum_u16 = checksumCalculator_u16((uint8_t*)(&(dap_state_extended_st_espNow.payloadHeader_st)), sizeof(dap_state_extended_st_espNow.payloadHeader_st) + sizeof(dap_state_extended_st_espNow.payloadPedalStateExtended_st));
              }
            }
            else
            {
              s_semaphore_updatePedalStates = xSemaphoreCreateMutex();
            }

            ESPNow.send_message(g_broadcast_mac,(uint8_t *)&dap_state_extended_st_espNow, sizeof(dap_state_extended_st_espNow));
            extend_state_send_b=false;
          }

          profiler_espNow.end(3);

          if (g_ESPNow_config_request && !noAssignmentStatus)
          {
            DapConfig_t * dap_config_st_local_ptr;
            dap_config_st_local_ptr = &espnow_dap_config_st;
            dap_config_st_local_ptr->payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
            dap_config_st_local_ptr->payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
            dap_config_st_local_ptr->payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
            dap_config_st_local_ptr->payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;
            dap_config_st_local_ptr->payloadHeader_st.pedalTag_u8 = dap_config_st_local_ptr->payloadPedalConfig_st.pedalType_u8;
            uint16_t crc=0;
            crc = checksumCalculator_u16((uint8_t*)(&(espnow_dap_config_st.payloadHeader_st)), sizeof(espnow_dap_config_st.payloadHeader_st) + sizeof(espnow_dap_config_st.payloadPedalConfig_st));
            dap_config_st_local_ptr->payloadFooter_st.checkSum_u16 = crc;
            ESPNow.send_message(g_broadcast_mac,(uint8_t *) & espnow_dap_config_st, sizeof(espnow_dap_config_st));
            g_ESPNow_config_request=false;
            sendESPNOWLog("Pedal:%d Config returned by user request, CRC:%d", espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8, crc);
            delay(2);
          }

          if (g_ESPNow_OTA_enable && !noAssignmentStatus)
          {
            ActiveSerial->println("Get OTA command");
            
            g_OTA_enable_b=true;
            g_OTA_enable_start=true;
            g_ESPNow_OTA_enable=false;
          }

          if (OTA_update_action_b && !noAssignmentStatus)
          {
            ActiveSerial->println("Starting Pedal OTA");
            buzzerBeepAction_b=true;
            g_OTA_enable_b=true;
            g_OTA_enable_start=true;
            g_ESPNow_OTA_enable=false;
            OTA_update_action_b = false;
            //ActiveSerial->println("get basic wifi info");
            //ActiveSerial->readBytes((char*)&dap_action_ota_st, sizeof(DapActionOta_t));
            #ifdef OTA_update

              if(dap_action_ota_st.payloadHeader_st.payloadType_u8==DAP_PAYLOAD_TYPE_ACTION_OTA_U8)
              {
                if(dap_action_ota_st.payloadOtaInfo_st.deviceId_u8 == espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8)
                {
                  g_SSID=new char[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8+1];
                  g_PASS=new char[dap_action_ota_st.payloadOtaInfo_st.passLength_u8+1];
                  memcpy(g_SSID,dap_action_ota_st.payloadOtaInfo_st.wifiSsid_au8,dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8);
                  memcpy(g_PASS,dap_action_ota_st.payloadOtaInfo_st.wifiPass_au8,dap_action_ota_st.payloadOtaInfo_st.passLength_u8);
                  g_SSID[dap_action_ota_st.payloadOtaInfo_st.ssidLength_u8]=0;
                  g_PASS[dap_action_ota_st.payloadOtaInfo_st.passLength_u8]=0;
                  g_OTA_enable_b=true;
                }
              }

            #endif

          }

          if (printPedalInfo_b && !noAssignmentStatus)
          {
            printPedalInfo_b=false;
            buzzerBeepAction_b=true;
            delay(100);
             pedalInfoBuilder.BuildInfoString(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8, CONTROL_BOARD, loadcell->getBiasEstimate(), loadcell->getStandardDeviationEstimate(), ((float)stepper->getServosVoltage()/10.0f),dap_calculationVariables_st.stepperPosMaxEndstop_i32,dap_calculationVariables_st.currentPedalPosition_u32);
            sendESPNOWLog(pedalInfoBuilder.logString);
            ActiveSerial->println(pedalInfoBuilder.logString);
            delay(3);
            pedalInfoBuilder.BuildESPNOWInfo(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8,g_rssi);
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
              sendESPNOWLog("Pedal:%d restart into Download mode", espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8);
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
            if((dap_calculationVariables_st.rudderStatus_b || dap_calculationVariables_st.helicopterRudderStatus_b) && (!Rudder_initializing && !HeliRudder_initializing))
            {              
               dap_rudder_sending.payloadRudderState_st.pedalPositionRatio_fl32=dap_calculationVariables_st.currentPedalPositionRatio_fl32;
               dap_rudder_sending.payloadRudderState_st.pedalPosition_u16=dap_calculationVariables_st.currentPedalPosition_u32;
              dap_rudder_sending.payloadHeader_st.payloadType_u8=DAP_PAYLOAD_TYPE_ESPNOW_RUDDER_U8;
              dap_rudder_sending.payloadHeader_st.pedalTag_u8 = espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8;
              dap_rudder_sending.payloadHeader_st.version_u8=DAP_VERSION_CONFIG_U8;
              uint16_t crc=0;
              crc = checksumCalculator_u16((uint8_t*)(&(dap_rudder_sending.payloadHeader_st)), sizeof(dap_rudder_sending.payloadHeader_st) + sizeof(dap_rudder_sending.payloadRudderState_st));
              dap_rudder_sending.payloadFooter_st.checkSum_u16=crc;
              ESPNow.send_message(g_broadcast_mac,(uint8_t *) &dap_rudder_sending,sizeof(dap_rudder_sending));   
               //ESPNow_send=dap_calculationVariables_st.currentPedalPosition_u32; 
              //esp_err_t result =ESPNow.send_message(Recv_mac,(uint8_t *) &_ESPNow_Send,sizeof(_ESPNow_Send));                
              //if (result == ESP_OK) 
              //{
              //  ActiveSerial->println("Error sending the data");
              //}
              if (g_ESPNow_Rudder_Update && !noAssignmentStatus)
              {
                //dap_calculationVariables_st.syncPedalPosition_u32=ESPNow_recieve;
                dap_calculationVariables_st.syncPedalPosition_u32=dap_rudder_receiving.payloadRudderState_st.pedalPosition_u16;
                dap_calculationVariables_st.syncPedalPositionRatio_fl32=dap_rudder_receiving.payloadRudderState_st.pedalPositionRatio_fl32;
                g_ESPNow_Rudder_Update=false;
              }                
            }
            rudderPacketsUpdateLast=millis();
          }    
        }

        #ifdef ESPNow_debug_rudder
          if(print_count>1000)
          {
            if(dap_calculationVariables_st.rudderStatus_b)
            {
              ActiveSerial->print("Pedal:");
              ActiveSerial->print(espnow_dap_config_st.payloadPedalConfig_st.pedalType_u8);
              ActiveSerial->print(", Send %: ");
              ActiveSerial->print(dap_rudder_sending.payloadRudderState_st.pedalPositionRatio_fl32);
              ActiveSerial->print(", Recieve %:");
              ActiveSerial->print(dap_rudder_receiving.payloadRudderState_st.pedalPositionRatio_fl32);
              ActiveSerial->print(", Send Position: ");
               ActiveSerial->print(dap_calculationVariables_st.currentPedalPosition_u32);
              ActiveSerial->print(", % in cal: ");
               ActiveSerial->print(dap_calculationVariables_st.currentPedalPositionRatio_fl32); 
              ActiveSerial->print(", min cal: ");
              ActiveSerial->print(dap_calculationVariables_st.stepperPosMinDefault_i32); 
              ActiveSerial->print(", max cal: ");
              ActiveSerial->print(dap_calculationVariables_st.stepperPosMaxDefault_i32);
              ActiveSerial->print(", range in cal: ");
              ActiveSerial->println(dap_calculationVariables_st.stepperPosRangeDefault_fl32); 
            }

            //Debug_rudder_last=now_rudder;
             //ActiveSerial->println(dap_calculationVariables_st.currentPedalPosition_u32);                  
                
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
  static DapConfig_t misc_dap_config_st;
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
      if(g_beepForOtaProgress)
      {
        Buzzer.single_beep_tone(700,50);
        g_beepForOtaProgress=false;
      }
    #endif
    delay(50);
  }
}

