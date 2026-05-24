#include "UsbComManager.h"

UsbComManager::UsbComManager(uint32_t intervalMs) {
    batchIntervalMs = intervalMs;
    lastBatchTimeMs = 0;
    writeIdx = 0;
    readIdx = 0;
    availableBytes = 0;
    
    // Mutex erstellen, um Thread-Sicherheit zwischen den Tasks zu garantieren
    bufferMutex = xSemaphoreCreateMutex();
}

void UsbComManager::begin(uint8_t pedalId) {
    usbSerial.begin(115200);
    
    #ifdef USB_JOYSTICK
      SetupController_USB(pedalId); 
    #endif
}

size_t IRAM_ATTR_FLAG UsbComManager::write(uint8_t c) {
    if (!usbSerial) return 1; // PC liest nicht mit -> Daten verwerfen

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
    if (!usbSerial) return size; // So tun als wäre gesendet worden

    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        size_t freeSpace = BUFFER_SIZE - availableBytes;
        
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
    if (!usbSerial) return 0;
    return usbSerial.availableForWrite(); 
}

int IRAM_ATTR_FLAG UsbComManager::available() { return usbSerial.available(); }
int IRAM_ATTR_FLAG UsbComManager::read() { return usbSerial.read(); }
int IRAM_ATTR_FLAG UsbComManager::peek() { return usbSerial.peek(); }
void IRAM_ATTR_FLAG UsbComManager::flush() { usbSerial.flush(); }

void IRAM_ATTR_FLAG UsbComManager::processTxBatch() {
    if (!usbSerial) {
        // PC getrennt: Puffer leeren
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            writeIdx = 0;
            readIdx = 0;
            availableBytes = 0;
            xSemaphoreGive(bufferMutex);
        }
        return;
    }

    bool timeToBatch = (millis() - lastBatchTimeMs >= batchIntervalMs);
    bool bufferFullEnough = false;
    
    if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
        bufferFullEnough = (availableBytes >= 128);
        xSemaphoreGive(bufferMutex);
    }

    if (timeToBatch || bufferFullEnough) {
        lastBatchTimeMs = millis();
        
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            size_t copyLen = availableBytes;
            
            if (copyLen > 0) {
                size_t untilEnd = BUFFER_SIZE - readIdx;
                if (copyLen <= untilEnd) {
                    usbSerial.write(&txRingBuffer[readIdx], copyLen);
                } else {
                    usbSerial.write(&txRingBuffer[readIdx], untilEnd);
                    usbSerial.write(&txRingBuffer[0], copyLen - untilEnd);
                }
                readIdx = (readIdx + copyLen) % BUFFER_SIZE;
                availableBytes -= copyLen;
            }
            xSemaphoreGive(bufferMutex);
        }
    }
}

#ifdef USB_JOYSTICK
bool IRAM_ATTR_FLAG UsbComManager::isJoystickReady() { return IsControllerReady(); }
void IRAM_ATTR_FLAG UsbComManager::sendJoystickValue(uint16_t val) { SetControllerOutputValue(val); }
#endif