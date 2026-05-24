#pragma once
#include <Arduino.h>
#include "USB.h"
#include "USBCDC.h"
#include "Controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Main.h"

class UsbComManager : public Stream {
private:
    USBCDC usbSerial;
    SemaphoreHandle_t bufferMutex;
    
    static const size_t BUFFER_SIZE = 8192; // Puffer vergrößert für 4kHz Telemetrie (ca. 250 KB/s)
    uint8_t txRingBuffer[BUFFER_SIZE];
    size_t writeIdx;
    size_t readIdx;
    size_t availableBytes;

    unsigned long lastBatchTimeMs;
    uint32_t batchIntervalMs;

public:
    UsbComManager(uint32_t intervalMs = 5);
    void begin(uint8_t pedalId = 1);

    size_t IRAM_ATTR_FLAG write(uint8_t c) override;
    size_t IRAM_ATTR_FLAG write(const uint8_t *buffer, size_t size) override;
    int IRAM_ATTR_FLAG availableForWrite() override;

    int IRAM_ATTR_FLAG available() override;
    int IRAM_ATTR_FLAG read() override;
    int IRAM_ATTR_FLAG peek() override;
    void IRAM_ATTR_FLAG flush() override;

    void IRAM_ATTR_FLAG processTxBatch();

    #ifdef USB_JOYSTICK
    bool IRAM_ATTR_FLAG isJoystickReady();
    void IRAM_ATTR_FLAG sendJoystickValue(uint16_t val);
    #endif
};