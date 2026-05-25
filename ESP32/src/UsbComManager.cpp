#include "UsbComManager.h"

#define BAUD_3M_U32 3000000U

#if defined(USE_CDC_INSTEAD_OF_UART)
    #if __has_include("esp32-hal-tinyusb.h")
        #include "esp32-hal-tinyusb.h"
    #endif
    #if __has_include("soc/rtc_cntl_reg.h")
        #include "soc/rtc_cntl_reg.h"
    #endif
#endif

UsbComManager::UsbComManager(uint32_t intervalMs) {
    batchIntervalMs = intervalMs;
    lastBatchTimeMs = 0;
    writeIdx = 0;
    readIdx = 0;
    availableBytes = 0;
    
    // Mutex erstellen, um Thread-Sicherheit zwischen den Tasks zu garantieren
    bufferMutex = xSemaphoreCreateMutex();

    #if defined(USE_CDC_INSTEAD_OF_UART)
        targetStream = &customUsbSerial;
    #else
        targetStream = &Serial;
    #endif
}

void UsbComManager::begin(uint8_t pedalId) {

    #if defined(USE_CDC_INSTEAD_OF_UART)
        customUsbSerial.begin(115200);
        customUsbSerial.setTxTimeoutMs(1); 
        customUsbSerial.enableReboot(false); // Lebenswichtig, damit SimHub die Config empfängt!
    #else
        Serial.begin(BAUD_3M_U32);
    #endif
    
    #ifdef USB_JOYSTICK
      SetupController_USB(pedalId); 
    #endif
}

size_t IRAM_ATTR_FLAG UsbComManager::write(uint8_t c) {
    
    #if defined(USE_CDC_INSTEAD_OF_UART)
    if (!customUsbSerial) return 1;
    #else
    if (!Serial) return 1;
    #endif

    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        if (availableBytes < BUFFER_SIZE) {
            txRingBuffer[writeIdx] = c;
            writeIdx = (writeIdx + 1) % BUFFER_SIZE;
            availableBytes++;
        }
        xSemaphoreGive(bufferMutex);
    }
    return 1;
}

size_t IRAM_ATTR_FLAG UsbComManager::write(const uint8_t *buffer, size_t size) {
    if (size == 0) return 0;
    #if defined(USE_CDC_INSTEAD_OF_UART)
    if (!customUsbSerial) return size;
    #else
    if (!Serial) return size;
    #endif

    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        size_t freeSpace = BUFFER_SIZE - availableBytes;
        
        // Fällt das Paket nicht mehr rein? -> Einfach verwerfen (Drop-Newest)
        // Das erhält den FIFO-Stream absolut lückenlos. Keine 20ms Lücken mehr!
        if (size > 0 && size <= freeSpace) {
            size_t untilEnd = BUFFER_SIZE - writeIdx;
            if (size <= untilEnd) {
                memcpy(&txRingBuffer[writeIdx], buffer, size);
            } else {
                memcpy(&txRingBuffer[writeIdx], buffer, untilEnd);
                memcpy(&txRingBuffer[0], buffer + untilEnd, size - untilEnd);
            }
            writeIdx = (writeIdx + size) % BUFFER_SIZE;
            availableBytes += size;
        }
        xSemaphoreGive(bufferMutex);
    }
    return size; 
}

int IRAM_ATTR_FLAG UsbComManager::availableForWrite() { 
    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        int freeSpace = BUFFER_SIZE - availableBytes;
        xSemaphoreGive(bufferMutex);
        return freeSpace;
    }
    return 0;
}

int IRAM_ATTR_FLAG UsbComManager::available() { return targetStream->available(); }
int IRAM_ATTR_FLAG UsbComManager::read() { return targetStream->read(); }
int IRAM_ATTR_FLAG UsbComManager::peek() { return targetStream->peek(); }
void IRAM_ATTR_FLAG UsbComManager::flush() { targetStream->flush(); }

void IRAM_ATTR_FLAG UsbComManager::processTxBatch() {
    #if defined(USE_CDC_INSTEAD_OF_UART)
    if (!customUsbSerial) {
    #else
    if (!Serial) {
    #endif
        // PC getrennt: Puffer leeren
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            writeIdx = 0;
            readIdx = 0;
            availableBytes = 0;
            xSemaphoreGive(bufferMutex);
        }
        return;
    }

#if defined(USE_CDC_INSTEAD_OF_UART)
    // Manueller 1200bps Upload-Touch für PlatformIO Auto-Flash.
    // Startet erst nach 5s, damit das Öffnen von SimHub/SerialMonitor den ESP nicht resettet!
    static bool rebootEnabled = false;
    if (!rebootEnabled && millis() > 5000) {
        rebootEnabled = true;
    }
    if (rebootEnabled && customUsbSerial.baudRate() == 1200) {
        #if __has_include("esp32-hal-tinyusb.h")
            usb_persist_restart(RESTART_BOOTLOADER);
        #else
            REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
            ESP.restart();
        #endif
    }
#endif

    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        int avail = targetStream->availableForWrite();
        
        // Kontinuierlicher, blockierungsfreier Datenabfluss in die USB-Hardware
        while (avail > 0 && availableBytes > 0) {
            size_t copyLen = (availableBytes < (size_t)avail) ? availableBytes : (size_t)avail;
            
            size_t untilEnd = BUFFER_SIZE - readIdx;
            if (copyLen <= untilEnd) {
                targetStream->write(&txRingBuffer[readIdx], copyLen);
            } else {
                targetStream->write(&txRingBuffer[readIdx], untilEnd);
                targetStream->write(&txRingBuffer[0], copyLen - untilEnd);
            }
            readIdx = (readIdx + copyLen) % BUFFER_SIZE;
            availableBytes -= copyLen;
            
            avail = targetStream->availableForWrite(); // Platz nach dem Write neu abfragen!
        }
        xSemaphoreGive(bufferMutex);
    }
}

#ifdef USB_JOYSTICK
bool IRAM_ATTR_FLAG UsbComManager::isJoystickReady() { return IsControllerReady(); }
void IRAM_ATTR_FLAG UsbComManager::sendJoystickValue(uint16_t val) { SetControllerOutputValue(val); }
#endif