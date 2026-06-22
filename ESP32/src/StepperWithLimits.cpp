#include "StepperWithLimits.h"
#include "Main.h"
#include "Math.h"
#include "FunctionProfiler.h"
#include "esp_task_wdt.h"

// ==============================================================================
// Constants & Magic Numbers Refactored
// ==============================================================================
constexpr int32_t MIN_POS_MAX_ENDSTOP = 10000;          // Minimum steps the servo must travel before max endstop detection is allowed
constexpr int32_t ENDSTOP_MOVEMENT = 100;               // Baseline movement amount for trigger checks
constexpr int32_t s_ENDSTOP_MOVEMENT_SENSORLESS = ENDSTOP_MOVEMENT * 5; // Movement offset when backing off from a sensorless endstop
constexpr uint32_t TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_STANDSTILL_IN_MS = 200; // Time without movement to consider the pedal idle
constexpr uint32_t TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_CRASH_IN_MS = 10000;    // Time without movement to enable deep crash detection
constexpr uint32_t LIFELINE_CHECK_INTERVAL_MS = 500;    // Interval to verify serial heartbeat
constexpr int32_t ENCODER_OVERFLOW_THRESHOLD = INT16_MAX; // Upper boundary for 16-bit encoder wrap detection
constexpr int32_t ENCODER_UNDERFLOW_THRESHOLD = INT16_MIN; // Lower boundary for 16-bit encoder wrap detection
constexpr int32_t ENCODER_WRAP_VALUE = 65536;           // Value added/subtracted to unwrap 16-bit values (2^16)
constexpr uint8_t MAX_WRAP_ITERATIONS = 50;             // Failsafe to prevent infinite loops during position unwrapping

// ==============================================================================
// Global Task & Synchronization Variables
// ==============================================================================
TaskHandle_t task_iSV_Communication;
static SemaphoreHandle_t s_timer_fireServoCommunication = NULL;

// --- Lazy Initialization of Semaphores ---
// Ensures that Mutexes are created exactly once when they are first needed.
static SemaphoreHandle_t getResetServoPosSemaphore() {
    static SemaphoreHandle_t sem = xSemaphoreCreateMutex();
    return sem;
}
static SemaphoreHandle_t getReadServoValuesSemaphore() {
    static SemaphoreHandle_t sem = xSemaphoreCreateMutex();
    return sem;
}
static SemaphoreHandle_t getCorrectedServoPosSemaphore() {
    static SemaphoreHandle_t sem = xSemaphoreCreateMutex();
    return sem;
}

// --- Global Parameters ---
static float s_servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_36V;
static bool s_servoBusVoltageParameterized_b = true;
static bool s_printProfilingFlag_b = false;
bool setServoToSleep_b = false;

// ==============================================================================
// RAII Mutex Guard Class
// ==============================================================================
// This class automatically acquires a FreeRTOS Semaphore/Mutex when created,
// and guarantees that the lock is released when the object goes out of scope,
// even if an early return or exception occurs. This eliminates deadlocks.
class MutexGuard {
private:
    SemaphoreHandle_t _mutex;
public:
    explicit MutexGuard(SemaphoreHandle_t mutex) : _mutex(mutex) {
        if (_mutex != NULL) {
            xSemaphoreTake(_mutex, portMAX_DELAY);
        }
    }
    ~MutexGuard() {
        if (_mutex != NULL) {
            xSemaphoreGive(_mutex);
        }
    }
};

// ==============================================================================
// Constructor & Initialization
// ==============================================================================
StepperWithLimits::StepperWithLimits(uint8_t pinStep, uint8_t pinDirection, bool invertMotorDir_b, uint32_t stepsPerMotorRev_arg_u32, uint8_t _endstopDetectionThreshold)
  :  _endstopLimitMin(0), _endstopLimitMax(0),
     _posMin(0), _posMax(0),
     stepsPerMotorRev_u32(stepsPerMotorRev_arg_u32)
{
    // 1. Initialize pulse generator library for high-frequency step output
    _stepper = new FastNonAccelStepper(pinStep, pinDirection, invertMotorDir_b); 
    _stepper->begin();
    _stepper->setExpectedCycleTimeUs(REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64);

    // Clamp the endstop detection threshold to safe limits (10% to 50% current load)
    endstopDetectionThreshold_u8 = constrain(endstopDetectionThreshold_u8, 10, 50);
    Serial.printf("InvertStepperDir: %d\n", invertMotorDir_b);

    // 2. Power on the servo if a hardware power pin is configured
    #ifdef SERVO_POWER_PIN
        gpio_set_direction((gpio_num_t)SERVO_POWER_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)SERVO_POWER_PIN, 1);
        delay(500); // Give the servo processor time to boot
    #endif


#ifdef ALM_PORT_GPIO
    // ==============================================================================
    // Wait for Servo Ready (SRDY) Signal on GPIO ALM_PORT_GPIO
    // ==============================================================================
    pinMode(ALM_PORT_GPIO, INPUT_PULLUP); 
    
    if (ActiveSerial) ActiveSerial->println("Waiting for stable Servo ready signal...");
    
    uint32_t srdyWaitStart = millis();      // Global timeout counter
    uint32_t readyStableStartTime = 0;      // Debounce counter
    bool isStableReady = false;
    bool forceProceed_b = false;
    
    while (!isStableReady && !forceProceed_b) {
        if (digitalRead(ALM_PORT_GPIO) == LOW) {
            // Signal is 'Ready'. Check if timer is already running.
            if (readyStableStartTime == 0) {
                readyStableStartTime = millis(); // Start timer
            } 
            // Check if 500ms has been reached continuously
            else if (millis() - readyStableStartTime >= 500) {
                isStableReady = true; // Successfully debounced!
            }
        } else {
            // Signal is HIGH again ('Not Ready' or bouncing). Reset timer!
            readyStableStartTime = 0;
        }

        delay(10); // Give watchdog and system time
        
        // Failsafe-Timeout (e.g. 5 seconds total for boot process)
        if (millis() - srdyWaitStart > 5000) { 
            //if (ActiveSerial) ActiveSerial->println("CRITICAL: Timeout waiting for stable SRDY! Restarting ESP.");
            //delay(100);
            //ESP.restart();
            forceProceed_b = true;
        }
    }


    if (isStableReady)
    {
        if (ActiveSerial) ActiveSerial->println("Stable servo ready signal detected");
    }
    else
    {
        if (ActiveSerial) ActiveSerial->println("Stable servo ready signal not detected. Procedding anyway, since servo parameters might not have been flashed yet.");
    }
    
    // ==============================================================================
#endif

    // 3. Attempt to discover the Modbus Slave ID of the connected iSV57 servo
    bool isv57slaveIdFound_b = isv57.findServosSlaveId();
    ActiveSerial->print("iSV57 slaveId found:  ");
    ActiveSerial->println( isv57slaveIdFound_b );
    
    // Critical failure: If the servo isn't found, the system cannot function. Restart ESP.
    if (!isv57slaveIdFound_b) {
        ActiveSerial->println("No servo found! Restarting ESP");
        ESP.restart();
    }

    // 4. Verify the communication lifeline
    setLifelineSignal();
    if (getLifelineSignal() == false) {
        ActiveSerial->println("No lifeline detected! Restarting ESP");
        ESP.restart();
    } else {
        // 5. Read historical alarms and configure the servo parameters
        isv57.readAlarmHistory();
        ActiveSerial->print("iSV57 communication state:  ");
        ActiveSerial->println(getLifelineSignal());

        // Prepare the telemetry read structure and push tuned parameters
        isv57.setupServoStateReading();
        invertMotorDir_global_b = invertMotorDir_b;
        isv57.sendTunedServoParameters(invertMotorDir_global_b, stepsPerMotorRev_u32);

        // Clear axis state and enable the motor
        delay(30);
        isv57.enableAxis();
        delay(100);

        // 6. Spawn the dedicated FreeRTOS task for continuous Modbus telemetry polling
        xTaskCreatePinnedToCore(
            this->servoCommunicationTask,   
            "servoCommunicationTask", 
            4096,  
            this,      
            TASK_PRIORITY_SERVO_COMMUNICATION_TASK_UBASETYPE,         
            &task_iSV_Communication,    
            CORE_ID_SERVO_COMMUNICATION_TASK_U8);   
    }
}

// ==============================================================================
// Public API & Core Actions
// ==============================================================================
void StepperWithLimits::resetServoParametersToFactoryValues() { resetServoRegistersToFactoryValues_b = true; }
void StepperWithLimits::clearAllServoAlarms() { clearAllServoAlarms_b = true; }
void StepperWithLimits::printAllServoParameters() { logAllServoParams = true; }
void StepperWithLimits::configSetProfilingFlag(bool proFlag_b) { s_printProfilingFlag_b = proFlag_b; }

void StepperWithLimits::scheduleServoModbusCmd(const ServoModbusCmd_t& cmd)
{
    // Write struct first, then set flag (acts as release fence on volatile).
    servoModbusPendingCmd_st = cmd;
    servoModbusCmdPending_b  = true;
}

bool IRAM_ATTR StepperWithLimits::tryGetServoModbusReadResult(
        uint16_t& addrOut, int16_t* valuesOut, uint8_t& countOut, uint16_t* addrsOut)
{
    if (!servoModbusReadReady_b) return false;
    addrOut  = servoModbusReadAddr_u16;
    countOut = servoModbusReadCount_u8;
    memcpy(valuesOut, servoModbusReadResult, countOut * sizeof(int16_t));
    if (addrsOut != nullptr) {
        memcpy(addrsOut, servoModbusReadAddresses, countOut * sizeof(uint16_t));
    }
    servoModbusReadReady_b = false;  // consume the result
    return true;
}

// --- Sensorless Homing Routine ---
// Automatically sweeps the pedal to find physical minimum and maximum bounds
// by monitoring the servo's internal current/load telemetry.
void StepperWithLimits::findMinMaxSensorless(DapConfig_t dap_config_st) {
    if (!hasValidStepper()) return;

    if (getLifelineSignal()) {
        // 1. Verify that voltage readings are stable and trustworthy
        bool servoRadingsTrustworthy_36VRange_b = false;
        bool servoRadingsTrustworthy_48VRange_b = false;
        
        for (uint16_t waitTillServoCounterWasReset_Idx = 0; waitTillServoCounterWasReset_Idx < 10; waitTillServoCounterWasReset_Idx++) {
            delay(100);
            esp_task_wdt_reset();
            // Voltage is reported in 0.1V units (e.g. 360 = 36.0V)
            float servosBusVoltageInVolt_fl32 = ((float)getServosVoltage()) / 10.0f;
            
            // Check if voltage falls into expected 36V or 48V power supply brackets
            servoRadingsTrustworthy_36VRange_b = (servosBusVoltageInVolt_fl32 >= 16.0f) && (servosBusVoltageInVolt_fl32 < SERVO_MAX_VOLTAGE_IN_V_36V);
            servoRadingsTrustworthy_48VRange_b = (servosBusVoltageInVolt_fl32 >= 16.0f) && (servosBusVoltageInVolt_fl32 < SERVO_MAX_VOLTAGE_IN_V_48V);

            if (servoRadingsTrustworthy_36VRange_b) {
                s_servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_36V;
                s_servoBusVoltageParameterized_b = false;
                ActiveSerial->printf("Servos bus voltage in expected range (36V range): %.1fV\n", servosBusVoltageInVolt_fl32);
                break;
            }
            if (servoRadingsTrustworthy_48VRange_b) {
                s_servoBusVoltageParameterized_fl32 = SERVO_MAX_VOLTAGE_IN_V_48V;
                s_servoBusVoltageParameterized_b = false;
                ActiveSerial->printf("Servos bus voltage in expected range (48V range): %.1fV\n", servosBusVoltageInVolt_fl32);
                break;
            }
        }

        if (!servoRadingsTrustworthy_36VRange_b && !servoRadingsTrustworthy_48VRange_b) {
            ActiveSerial->print("Servo bus voltage not in expected range (16V-50V). Restarting ESP!");
            ESP.restart();
        }

        float spindlePitch = max(dap_config_st.payloadPedalConfig_st.spindlePitch_mmPerRev_u8, (uint8_t)1);

        // 2. Minimum Endstop Detection (Homing to 0)
        bool endPosDetected = true; 
        int32_t setPosition = 0;
        
        // Target sweep speed: 2.5cm/s
        float endstopApproachingSpeedInMmPerSecond_fl32 = 25.0f;
        float endstopApproachingSpeed_fl32 = endstopApproachingSpeedInMmPerSecond_fl32 / spindlePitch * stepsPerMotorRev_u32;
        endstopApproachingSpeed_fl32 = constrain(endstopApproachingSpeed_fl32, 10000, MAXIMUM_STEPPER_SPEED_U32 * 0.2f);

        // Wait for current signal to stabilize before moving
        for (uint16_t tryIdx = 0; tryIdx < 500; tryIdx++) {
            delay(5);
            esp_task_wdt_reset();
            endPosDetected = abs(getServosCurrent()) > endstopDetectionThreshold_u8;
            if (!endPosDetected) break;
        }
            
        // Move backward until load threshold is exceeded
        _stepper->setMaxSpeed(endstopApproachingSpeed_fl32);
        ActiveSerial->println("Move to min");
        _stepper->keepRunningBackward(endstopApproachingSpeed_fl32);
        _stepper->setSpeedLive(endstopApproachingSpeed_fl32);

        while ((!endPosDetected) && (getLifelineSignal())) {
            delay(1);
            esp_task_wdt_reset();
            endPosDetected = abs(getServosCurrent()) > endstopDetectionThreshold_u8;
        }
        _stepper->forceStop();

        // Back off slightly from the hard block to prevent binding during operation
        setPosition = -5 * s_ENDSTOP_MOVEMENT_SENSORLESS;
        _stepper->forceStopAndNewPosition(setPosition);
        
        ActiveSerial->println("Min endstop reached.");
        ActiveSerial->printf("Current pos: %d\n", _stepper->getCurrentPosition());
        
        // Re-zero the internal coordinate system
        _stepper->moveTo(0, true); 
        _endstopLimitMin = 0;
        delay(200);
        isv57.setZeroPos(); // Zero out the servo's internal encoder position

        // 3. Maximum Endstop Detection (Finding maximum travel)
        float maxRevToReachEndPos = (float)dap_config_st.payloadPedalConfig_st.lengthPedalTravel_i16 / spindlePitch;
        float maxStepsToReachEndPos = maxRevToReachEndPos * (float)stepsPerMotorRev_u32;
        endPosDetected = false;
        
        // Move forward until load threshold or virtual max limit is hit
        _stepper->keepRunningForward(endstopApproachingSpeed_fl32);
        _stepper->setSpeedLive(endstopApproachingSpeed_fl32);

        while ((!endPosDetected) && (getLifelineSignal())) {
            delay(1);
            esp_task_wdt_reset();
            // Only allow crash detection if we have driven a minimum distance (prevents false positives from initial inertia)
            if (_stepper->getCurrentPosition() > MIN_POS_MAX_ENDSTOP) {
                endPosDetected = abs(getServosCurrent()) > endstopDetectionThreshold_u8;
            }
            // Trigger if we hit the configured software length limit
            endPosDetected |= (_stepper->getCurrentPosition() > maxStepsToReachEndPos);
        }
        
        // Calculate max soft limit, backing off slightly from the hard crash point
        _stepper->forceStop();
        _endstopLimitMax = _stepper->getCurrentPosition() - 5 * s_ENDSTOP_MOVEMENT_SENSORLESS;

        // Return home
        moveToPosWithSpeed(0, endstopApproachingSpeed_fl32);
        moveSlowlyToPos(0);
        
        

        ActiveSerial->printf("Max endstop reached: %d\n", _endstopLimitMax);
        
        
    }   
}

// --- Movement Helpers ---
void IRAM_ATTR StepperWithLimits::moveToPosWithSpeed(int32_t targetPos_ui32, uint32_t speedInHz_u32) {
    _stepper->setMaxSpeed(speedInHz_u32);
    _stepper->moveTo(targetPos_ui32, false);
}
void IRAM_ATTR StepperWithLimits::setSpeedLive(uint32_t speedInHz_u32) { _stepper->setSpeedLive(speedInHz_u32); }
void IRAM_ATTR StepperWithLimits::moveSlowlyToPos(int32_t targetPos_ui32) {
    _stepper->setSpeedLive((uint32_t)(MAXIMUM_STEPPER_SPEED_U32 / 8));
    _stepper->moveTo(targetPos_ui32, true); 
}
void IRAM_ATTR StepperWithLimits::pauseTask() { vTaskSuspend(task_iSV_Communication); }

void IRAM_ATTR StepperWithLimits::updatePedalMinMaxPos(uint8_t pedalStartPosPct, uint8_t pedalEndPosPct) {
    int32_t limitRange = _endstopLimitMax - _endstopLimitMin;
    _posMin = (int32_t)(_endstopLimitMin + (((float)limitRange * (float)pedalStartPosPct) * 0.01f));
    _posMax = (int32_t)(_endstopLimitMin + (((float)limitRange * (float)pedalEndPosPct) * 0.01f));
}

void IRAM_ATTR StepperWithLimits::forceStop() { _stepper->forceStop(); }
int8_t StepperWithLimits::moveTo(int32_t position, bool blocking) {
    _stepper->moveTo(position, blocking);
    return 1;
}

// --- Simple Getters ---
int32_t IRAM_ATTR StepperWithLimits::getCurrentPositionFromMin() const { return _stepper->getCurrentPosition() - _posMin; }
int32_t IRAM_ATTR StepperWithLimits::getMinPosition() const { return _posMin; }
int32_t IRAM_ATTR StepperWithLimits::getMaxPosition() const { return _posMax; }
int32_t IRAM_ATTR StepperWithLimits::getCurrentPosition() const { return _stepper->getCurrentPosition(); }
float IRAM_ATTR StepperWithLimits::getCurrentPositionFraction() const { return float(getCurrentPositionFromMin()) / getTravelSteps(); }
float IRAM_ATTR StepperWithLimits::getCurrentPositionFractionFromExternalPos(int32_t extPos_i32) const { return ((float)(extPos_i32)) / getTravelSteps(); }
int32_t IRAM_ATTR StepperWithLimits::getTargetPositionSteps() const { return _stepper->getPositionAfterCommandsCompleted(); }
void IRAM_ATTR StepperWithLimits::setSpeed(uint32_t speedInStepsPerSecond) { _stepper->setMaxSpeed(speedInStepsPerSecond); }
bool IRAM_ATTR StepperWithLimits::isAtMinPos() { return (abs(getCurrentPositionFromMin()) < 10) && !_stepper->isRunning(); }
int32_t IRAM_ATTR StepperWithLimits::getCurrentSpeedInHz() { return _stepper->getMaxSpeed(); }
uint32_t IRAM_ATTR StepperWithLimits::getMaxSpeedInHz() { return MAXIMUM_STEPPER_SPEED_U32; } 
bool IRAM_ATTR StepperWithLimits::isRunning() { return _stepper->isRunning(); }
void IRAM_ATTR StepperWithLimits::keepRunningInDir(bool forwardDir_b, uint32_t speed_u32) { _stepper->keepRunningInDir(forwardDir_b, speed_u32); }
void IRAM_ATTR StepperWithLimits::moveToWithSpeed(int32_t targetPos_i32, uint32_t speed_u32) { _stepper->moveToWithSpeed(targetPos_i32, speed_u32); }

// ==============================================================================
// Step Loss Recovery & Thread-Safe Telemetry Access
// ==============================================================================

// Injects measured step-loss offsets back into the ESP pulse generator 
// to keep mechanical position aligned with the software coordinates.
void IRAM_ATTR StepperWithLimits::correctPos() {
    if(!_stepper->isRunning()) {
        // servo_offset_compensation_steps_i32 is volatile int32_t: 32-bit aligned → atomic read/write on ESP32-S3, no mutex needed.
        int32_t maxStepsToRecoverPerCall_i32 = 2;
        int32_t stepOffset = (int32_t)constrain(servo_offset_compensation_steps_i32, -maxStepsToRecoverPerCall_i32, maxStepsToRecoverPerCall_i32);
        

        /*if (stepOffset != 0)
        {
            ActiveSerial->print("Position compensation: ");
            ActiveSerial->print(servo_offset_compensation_steps_i32);
            ActiveSerial->print(",   ");
            ActiveSerial->println(stepOffset);
        }*/

        // offset = ESPs position - servos position
        // new ESP pos = ESPs position - offset = ESPs position - ESPs position + servos position = servos position
        
        _stepper->setCurrentPosition(_stepper->getCurrentPosition() - stepOffset);
        servo_offset_compensation_steps_i32 = 0; // Prevent overcompensation
    }
}

bool IRAM_ATTR StepperWithLimits::getLifelineSignal() { return isv57LifeSignal_b; }
void IRAM_ATTR StepperWithLimits::setLifelineSignal() { isv57LifeSignal_b = isv57.checkCommunication(); }

// Raw telemetry getters mapped to the isv57 module
int32_t IRAM_ATTR StepperWithLimits::getServosVoltage() { return isv57.isv57dynamicStates_.servoVoltage0p1V_i16; }
int32_t IRAM_ATTR StepperWithLimits::getServosCurrent() { return isv57.isv57dynamicStates_.servo_current_percent; }
int32_t IRAM_ATTR StepperWithLimits::getServosPos() { return isv57.getPosFromMin(); }
int32_t IRAM_ATTR StepperWithLimits::getServosPosError() { return isv57.isv57dynamicStates_.servo_pos_error_p; }
uint32_t IRAM_ATTR StepperWithLimits::getServoCycleTimestamp() { return isv57.isv57dynamicStates_.lastUpdateTimeInMS_u32; }
int32_t IRAM_ATTR StepperWithLimits::getServosPosErrorChangeRateInStepsPerSecond() { return trackingErrorChangeInStepsPerS_fl32; }
uint32_t IRAM_ATTR StepperWithLimits::getServoCycleCounter() { return isv57.isv57dynamicStates_.servo_cycleCounter_u32; }

void IRAM_ATTR StepperWithLimits::setServosInternalPositionCorrected(int32_t posCorrected_i32) {
    // servoPos_local_corrected_i32 is volatile DRAM_ATTR int32_t.
    // 32-bit aligned single-word writes are atomic on ESP32-S3 → no mutex needed.
    servoPos_local_corrected_i32 = posCorrected_i32;
}

int32_t IRAM_ATTR StepperWithLimits::getServosInternalPositionCorrected() {
    // Lockless read: 32-bit aligned single-word reads are atomic on ESP32-S3.
    return servoPos_local_corrected_i32;
}

int32_t IRAM_ATTR StepperWithLimits::getServosInternalPosition() {
    // Lockless read: 32-bit aligned single-word reads are atomic on ESP32-S3.
    return servoPos_i16;
}


// ==============================================================================
// Configuration Updates
// ==============================================================================

// Sets the flags for step-loss recovery and crash detection based on external bitmask
void StepperWithLimits::configSteplossRecovAndCrashDetection(uint8_t flags_u8) {
    enableSteplossRecov_b = (flags_u8 >> 0) & 1;
    enableCrashDetection_b = (flags_u8 >> 1) & 1;
}


// ==============================================================================
// Refactored Sub-Routines for the FreeRTOS Orchestrator Task
// ==============================================================================

// Handles user or system requests for servo parameter changes, clears, or resets 
// asynchronously without blocking the main telemetry loop.
void IRAM_ATTR StepperWithLimits::processPendingCommands() {
    // Non-blocking sleep delay handler
    static uint32_t sleepTimerStart = 0;
    static bool isSleeping = false;

    if (setServoToSleep_b && !isSleeping) {
        isv57.disableAxis();
        sleepTimerStart = millis();
        isSleeping = true;
        setServoToSleep_b = false;
    }
    
    // Evaluate if sleep timeout is reached (simulating delay(500) asynchronously)
    if (isSleeping && (millis() - sleepTimerStart >= 500)) {
        isSleeping = false; 
    }

    if (clearAllServoAlarms_b) {
        ActiveSerial->println("Clearing all servo alarms.");
        isv57.clearServoAlarms();
        isv57.readAlarmHistory();
        clearAllServoAlarms_b = false;
    }

    if (resetServoRegistersToFactoryValues_b) {
        ActiveSerial->println("Reset to factory settings.");
        isv57.resetToFactoryParams();
        resetServoRegistersToFactoryValues_b = false;
        delay(500); // Intended blocking wait before hard ESP reset
        ESP.restart();
    }

    if (logAllServoParams) {
        logAllServoParams = false;
        isv57.readAllServoParameters();
    }

    if (updateServoParams_b) {
        updateServoParams_b = false;
        ActiveSerial->println("Updating Servo parameters.");
        // Note: The specific parameter updates would be applied here
    }

    // --- Pending Servo Modbus Command (UI-triggered register access) ---
    // Consumed here on the servo task to avoid concurrent Serial2 access.
    if (servoModbusCmdPending_b) {
        servoModbusCmdPending_b = false;  // clear flag first

        const ServoModbusCmd_t& cmd = servoModbusPendingCmd_st;
        uint8_t count = (cmd.count_u8 > 10u) ? 10u : cmd.count_u8;

        if (cmd.isWrite_b) {
            // Wenn SimHub einen vollständigen Schreibvorgang startet (beginnt immer bei Pr0.00 = 0x0000),
            // oder den NVM-Flash-Befehl (0x019A) sendet, schalten wir die Achse ab, da der Servo sonst blockiert.
            if (cmd.readAddresses[0] == 0x0000 || cmd.readAddresses[0] == 0x019A) {
                isv57.disableAxis();
                servoStatus = SERVO_IDLE_NOT_CONNECTED; // Erlaubt den automatischen ESP-Neustart beim nächsten Pedal-Druck!
            }
            delay(20);

            int16_t resultBuf[10] = {};
            uint8_t resultCount = 0;
            
            // Check if the addresses are sequential
            bool isSequential = (count > 0);
            for (uint8_t i = 1; i < count; i++) {
                if (cmd.readAddresses[i] != cmd.readAddresses[i - 1] + 1) {
                    isSequential = false;
                    break;
                }
            }

            if (isSequential && count > 1) {
                // SEQUENTIAL BATCH WRITE (Modbus FC16)
                uint16_t writeValues[10];
                for (uint8_t i = 0; i < count; i++) {
                    writeValues[i] = (uint16_t)cmd.values[i];
                    servoModbusReadAddresses[i] = cmd.readAddresses[i];
                }

                // 1. Alle Werte auf einmal schreiben (erfordert FC16 in Modbus.cpp)
                isv57.writeHoldingRegistersToDevice(isv57.slaveId, cmd.readAddresses[0], writeValues, count);
                
                // 2. Kurze Pause für den Servo
                vTaskDelay(pdMS_TO_TICKS(10));
                
                // 3. Verifizierung: Alle tatsächlichen Werte auf einmal zurücklesen
                int16_t readBackBuf[10] = {0};
                int n = isv57.readRegisters(cmd.readAddresses[0], count, readBackBuf);
                
                for (uint8_t i = 0; i < count; i++) {
                    resultBuf[i] = (n == count) ? readBackBuf[i] : 0xFFFF;
                }
                resultCount = count;
            } else {
                // FALLBACK: Einzelne Writes (FC06)
                for (uint8_t i = 0; i < count; i++) {
                    // 1. Wert schreiben
                    isv57.writeHoldingRegisterToDevice(
                        isv57.slaveId,
                        (int32_t)(cmd.readAddresses[i]),
                        (uint16_t)cmd.values[i]);
                    
                    // 2. Kurze Pause, damit der Servo das Register intern verarbeiten kann
                    vTaskDelay(pdMS_TO_TICKS(5));
                    
                    // 3. Verifizierung: Direktes Zurücklesen des tatsächlichen Wertes
                    int16_t buf[1] = {-1};
                    int n = isv57.readRegisters(cmd.readAddresses[i], 1, buf);
                    
                    servoModbusReadAddresses[resultCount] = cmd.readAddresses[i];
                    resultBuf[resultCount] = (n > 0) ? buf[0] : 0xFFFF; // Error code if read timeout
                    resultCount++;
                }
            }
            memcpy(servoModbusReadResult, resultBuf, resultCount * sizeof(int16_t));
            servoModbusReadCount_u8 = resultCount;
            servoModbusReadAddr_u16 = (count > 0) ? cmd.readAddresses[0] : 0;
            servoModbusReadReady_b  = true;
        } else {
            // READ: read each requested address individually (supports non-consecutive registers).
            int16_t resultBuf[10] = {};
            uint8_t resultCount = 0;
            
            bool isSequential = (count > 0);
            for (uint8_t i = 1; i < count; i++) {
                if (cmd.readAddresses[i] != cmd.readAddresses[i - 1] + 1) {
                    isSequential = false;
                    break;
                }
            }

            if (isSequential && count > 1) {
                // SEQUENTIAL BATCH READ (Modbus FC03)
                int16_t readBackBuf[10] = {0};
                int n = isv57.readRegisters(cmd.readAddresses[0], count, readBackBuf);
                
                for (uint8_t i = 0; i < count; i++) {
                    resultBuf[i] = (n == count) ? readBackBuf[i] : 0xFFFF;
                    servoModbusReadAddresses[i] = cmd.readAddresses[i];
                }
                resultCount = count;
            } else {
                // FALLBACK: Individual Reads
                for (uint8_t i = 0; i < count; i++) {
                    int16_t buf[1] = {-1};
                    int n = isv57.readRegisters(cmd.readAddresses[i], 1, buf);
                    servoModbusReadAddresses[resultCount] = cmd.readAddresses[i];
                    resultBuf[resultCount++] = (n > 0) ? buf[0] : 0xFFFF; 
                    vTaskDelay(pdMS_TO_TICKS(5)); // small delay to let RS485 driver turn around
                }
            }
            memcpy(servoModbusReadResult, resultBuf, resultCount * sizeof(int16_t));
            servoModbusReadCount_u8 = resultCount;
            servoModbusReadAddr_u16 = (count > 0) ? cmd.readAddresses[0] : 0;
            servoModbusReadReady_b  = true;
            // ActiveSerial->printf("[ServoModbus] READ done, %u register(s) ready\n", resultCount);
        }
    }
}

// Periodically validates that the Modbus serial connection is alive
void IRAM_ATTR StepperWithLimits::updateLifeline() {
    if ((timeNow_isv57SerialCommunicationTask_l - cycleTimeLastCall_lifelineCheck) > LIFELINE_CHECK_INTERVAL_MS) {
        cycleTimeLastCall_lifelineCheck = timeNow_isv57SerialCommunicationTask_l;
        setLifelineSignal();
    }
}

// Failsafe sequence triggered when the serial connection is lost
void IRAM_ATTR StepperWithLimits::handleConnectionLoss() {
    if (servoStatus != SERVO_IDLE_NOT_CONNECTED && servoStatus != SERVO_FORCE_STOP) {
        servoStatus = SERVO_NOT_CONNECTED;
    }

    if (servoStatus == SERVO_NOT_CONNECTED) {
        ActiveSerial->println("Servo communication lost!");
    }
    
    previousIsv57LifeSignal_b = false;
    
    // Safety requirement: Disable brake resistor instantly to prevent thermal destruction
    #ifdef BRAKE_RESISTOR_PIN_U8
        digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);
        brakeResistorState_b = false;
    #endif
}

// Reads raw 16-bit encoder position and handles mechanical inversion
void IRAM_ATTR StepperWithLimits::readAndFormatServoPosition() {
    MutexGuard guard(getReadServoValuesSemaphore());
    servoPos_i16 = isv57.getPosFromMin();
    
    // Invert reading if the motor is mounted opposite to standard configuration
    if (!invertMotorDir_global_b) {
        servoPos_i16 *= -1;
    }
}

// Resolves 16-bit integer overflows from the servo's absolute encoder
// by tracking wraps and aligning to the ESP32's internal 32-bit position tracker.
// Tests: https://colab.research.google.com/github/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/Validation/TestsServoPosition/UnitTestUnwrapping.ipynb
void IRAM_ATTR StepperWithLimits::unwrapServoPosition() {
    int32_t servoPosCorrected_i32 = getServosInternalPosition();
    int32_t espPos_i32 = getCurrentPosition();

    int32_t posDiff = espPos_i32 - servoPosCorrected_i32;

    // determine number of wraps based on the difference between the ESP's position and the servo's reported position.
    int32_t wraps = 0;
    if (posDiff > ENCODER_OVERFLOW_THRESHOLD) { // > 32767
        // Offset +32768 fits perfectly for the positive branch
        wraps = (posDiff + 32768) / ENCODER_WRAP_VALUE; 
        
    } else if (posDiff < ENCODER_UNDERFLOW_THRESHOLD) { // < -32768
        // Offset -32767 compensates for C++ integer division (truncation towards zero) 
        // and the asymmetric boundary of the two's complement!
        wraps = (posDiff - 32767) / ENCODER_WRAP_VALUE; 
    }

    // correct the servo position by adding the wrap offset, which aligns the 16-bit encoder reading with the ESP's 32-bit position tracking.
    servoPosCorrected_i32 += wraps * ENCODER_WRAP_VALUE;
    setServosInternalPositionCorrected(servoPosCorrected_i32);
}

// Computes the rate of change of the servo's internal PI-loop tracking error.
// This derivative is useful for predicting spikes that require brake resistor activation.
void IRAM_ATTR StepperWithLimits::calculateTrackingErrorChange() {
    uint32_t servoCurrentStateCycleCounter_u32 = getServoCycleCounter();
    
    // Only compute if we have received a fresh cycle update from the servo
    if (servoCurrentStateCycleCounter_u32 == (servoLastStateCycleCounter_u32 + 1u)) {
        uint32_t servoCurrentTimeStampInMs_u32 = getServoCycleTimestamp();
        float timeStampDiffInMs_fl32 = (float)(servoCurrentTimeStampInMs_u32 - servoLastTimeStampInMs_u32);
        servoLastTimeStampInMs_u32 = servoCurrentTimeStampInMs_u32;

        int32_t currentTrackingError_i32 = getServosPosError();
        // Calculate change in steps per second
        trackingErrorChangeInStepsPerS_fl32 = (currentTrackingError_i32 - lastTrackingError_i32) / (timeStampDiffInMs_fl32 * 1e-3f);
        lastTrackingError_i32 = currentTrackingError_i32;
    } else {
        // Fallback if data is missing or stale
        trackingErrorChangeInStepsPerS_fl32 = 0u;
    }
    servoLastStateCycleCounter_u32 = servoCurrentStateCycleCounter_u32;
}

// Checks for standstill and uses current telemetry to identify mechanical crashes
// (e.g., when the user pushes the pedal hard against an unyielding mechanical limit)
#define CYCLES_SINCE_SERVO_POS_CORREECTED (uint32_t)5
void IRAM_ATTR StepperWithLimits::performSafetyChecks() {
    int32_t espPos_i32 = getCurrentPosition();
    int32_t servoPosCorrected_i32 = getServosInternalPositionCorrected();
    static uint32_t servoLastCycleCounter_u32 = 0;
    static uint32_t servoLastCycleCounterWhenPositionWasCorrected_u32 = 0;

    bool cycleCounterAdvanced_b = false;
    uint32_t servosCycleCount_u32 = getServoCycleCounter(); // Ensure we have the latest data for current and position before making safety decisions
    if (servoLastCycleCounter_u32 != servosCycleCount_u32) {
       cycleCounterAdvanced_b = true;
    }
    
    // Calculate elapsed cycles, explicitly accounting for uint32_t overflow/wrap-around
    uint32_t cyclesSinceCorrection_u32 = 0;
    if (servosCycleCount_u32 >= servoLastCycleCounterWhenPositionWasCorrected_u32) {
        cyclesSinceCorrection_u32 = servosCycleCount_u32 - servoLastCycleCounterWhenPositionWasCorrected_u32;
    } else {
        cyclesSinceCorrection_u32 = (UINT32_MAX - servoLastCycleCounterWhenPositionWasCorrected_u32) + servosCycleCount_u32 + 1;
    }
    
    bool cond_cyclesSinceServoPosCorrected = cyclesSinceCorrection_u32 >= CYCLES_SINCE_SERVO_POS_CORREECTED;

    servoLastCycleCounter_u32 = servosCycleCount_u32;

    bool cond_stepperIsAtMinPos = isAtMinPos();
    bool cond_crash_detected = false;

    // Monitor for position stability (standstill)
    if (cond_stepperIsAtMinPos) {
        int16_t servoPos_now_i16 = isv57.getPosFromMin();
        int64_t timeNow_l = millis();

        // Update timestamp if movement larger than noise threshold (30 steps) is detected
        if (abs((int32_t)servoPos_last_i16 - (int32_t)servoPos_now_i16) > 30) {
            servoPos_last_i16 = servoPos_now_i16;
            timeSinceLastServoPosChange_l = timeNow_l;
        }

        timeDiff = timeNow_l - timeSinceLastServoPosChange_l;
        // If pedal has been idle for an extended period, enable deep crash detection flag
        cond_crash_detected = (timeDiff > TIME_SINCE_SERVO_POS_CHANGE_TO_DETECT_CRASH_IN_MS && timeNow_l > 0);
    }

    // Capture position offset (lost steps) if the latest telemetry packet was valid
    if (isv57.isv57dynamicStates_.servo_receivedPacketIsValid_b) {
        int32_t servo_offset_compensation_steps_local_i32 = 0;
        if (enableSteplossRecov_b) {
            servo_offset_compensation_steps_local_i32 = espPos_i32 - servoPosCorrected_i32;
        }

        // Lockless write: servo_offset_compensation_steps_i32 is volatile int32_t → atomic on ESP32-S3.
        servo_offset_compensation_steps_i32 = servo_offset_compensation_steps_local_i32;
    }

    // Execute crash recovery bump if stuck against a hard stop
    // If current spikes high while the motor is commanded to be idle, it's pushing against a block.
    if (cond_stepperIsAtMinPos && cond_crash_detected && enableCrashDetection_b && cycleCounterAdvanced_b && cond_cyclesSinceServoPosCorrected) {
        if (abs(getServosCurrent()) >= 50) {

            servoLastCycleCounterWhenPositionWasCorrected_u32 = servosCycleCount_u32; // Prevent multiple corrections within the same stall event

            // Apply a small inverse position offset to relax the servo strain against the block
            if (getServosCurrent() > 0) {
                isv57.applyOfsetToZeroPos(-500); 
            } else {
                isv57.applyOfsetToZeroPos(500); 
            }
        }
    }
}

// Wraps all data gathering, decoding, and analytical functions for the servo
void IRAM_ATTR StepperWithLimits::processActiveServo(FunctionProfiler* profiler) {
    if (servoStatus != SERVO_IDLE_NOT_CONNECTED) {
        servoStatus = SERVO_CONNECTED;
    }

    // Perform requested axis restarts
    if (restartServo) {
        isv57.disableAxis();
        delay(15);              
        isv57.enableAxis();
        restartServo = false;
        delay(15);
    }

    // Configure the internal voltage limits dynamically
    if (!s_servoBusVoltageParameterized_b) {
        isv57.setServoVoltage(s_servoBusVoltageParameterized_fl32);
        s_servoBusVoltageParameterized_b = true;
    }

    // Initialize telemetry reading registers on fresh connection
    if (!previousIsv57LifeSignal_b) {
        isv57.setupServoStateReading();
        previousIsv57LifeSignal_b = true;
        delay(50);
    }
    
    // Poll the servo via Modbus/Serial
    profiler->start(1);
    isv57.readServoStates();
    profiler->end(1);

    // Process the raw data
    readAndFormatServoPosition();
    unwrapServoPosition();
    calculateTrackingErrorChange();
    performSafetyChecks();
}

// --- Main Task Timer Callback (Interrupt context) ---
// This lightweight ISR simple unlocks the main task via a FreeRTOS binary semaphore
void IRAM_ATTR timer_servoCommunication_callback(void* arg) {
    if(s_timer_fireServoCommunication != NULL) {
        xSemaphoreGiveFromISR(s_timer_fireServoCommunication, NULL);
    }
}

// --- Main FreeRTOS Orchestrator Task ---
// This infinite loop runs pinned to a core. It wakes up on precise timer intervals,
// processes high-level requests, and delegates telemetry parsing to sub-routines.
void IRAM_ATTR StepperWithLimits::servoCommunicationTask(void *pvParameters) {
    StepperWithLimits* self = static_cast<StepperWithLimits*>(pvParameters);

    // Create the synchronization primitive
    s_timer_fireServoCommunication = xSemaphoreCreateBinary();

    // Create and start the hardware timer to drive the communication loop frequency
    const esp_timer_create_args_t timer_args_servoCommunication = {
        .callback = &timer_servoCommunication_callback, 
        .name = "servo_communication"    
    };

    esp_timer_handle_t timer_handle_servoCommunication;
    esp_timer_create(&timer_args_servoCommunication, &timer_handle_servoCommunication);
    esp_timer_start_periodic(timer_handle_servoCommunication, REPETITION_INTERVAL_SERVO_COMMUNICATION_TASK_IN_US_I64); 

    // Setup performance profiling
    FunctionProfiler profiler_servoCommunication;
    profiler_servoCommunication.setName("servoCommunication");
    profiler_servoCommunication.setNumberOfCalls(300);

    for(;;) {
        // Yield CPU time blockingly until the hardware timer fires and unblocks the semaphore
        if (s_timer_fireServoCommunication != NULL && xSemaphoreTake(s_timer_fireServoCommunication, portMAX_DELAY) == pdTRUE) {
            
            profiler_servoCommunication.activate(s_printProfilingFlag_b);
            profiler_servoCommunication.start(0);

            self->timeNow_isv57SerialCommunicationTask_l = millis();

            // Execute modular logic steps
            self->processPendingCommands();
            self->updateLifeline();

            if (self->getLifelineSignal()) {
                self->processActiveServo(&profiler_servoCommunication);
            } else {
                self->handleConnectionLoss();
            }

            profiler_servoCommunication.end(0);
        }
        
        // Ensure FreeRTOS can schedule other tasks safely
        taskYIELD();
    }
}

// ==============================================================================
// Additional Environment Getters & Utilities
// ==============================================================================
bool IRAM_ATTR StepperWithLimits::getBrakeResistorState() {
    return brakeResistorState_b;
}

// Triggers the sleep phase of the servo. Uses hardware pin if defined, 
// else sets a software flag for the orchestrator task.
bool IRAM_ATTR StepperWithLimits::servoIdleAction() {
    bool returnValue_b = false;
    #ifdef SERVO_POWER_PIN
        gpio_set_level((gpio_num_t)SERVO_POWER_PIN, 0);
        delay(500);
        returnValue_b = true;
    #else
        setServoToSleep_b = true;
        returnValue_b = true;
    #endif
    return returnValue_b;
}

float IRAM_ATTR StepperWithLimits::getBrakeResistorActivationVoltage(void) {
    return s_servoBusVoltageParameterized_fl32;
}