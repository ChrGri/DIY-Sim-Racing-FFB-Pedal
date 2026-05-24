#pragma once
#include <Arduino.h>
#include "USB.h"
#include "USBCDC.h"
#include "Controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class UsbComManager : public Stream {
private:
    USBCDC usbSerial;
    SemaphoreHandle_t bufferMutex;
    
    static const size_t BUFFER_SIZE = 2048;
    uint8_t txRingBuffer[BUFFER_SIZE];
    size_t writeIdx;
    size_t readIdx;
    size_t availableBytes;

    unsigned long lastBatchTimeMs;
    uint32_t batchIntervalMs;

public:
    // Standard-Batch-Intervall: 5 Millisekunden
    UsbComManager(uint32_t intervalMs = 5) {
        batchIntervalMs = intervalMs;
        lastBatchTimeMs = 0;
        writeIdx = 0;
        readIdx = 0;
        availableBytes = 0;
        
        // Mutex erstellen, um Thread-Sicherheit zwischen den Tasks zu garantieren
        bufferMutex = xSemaphoreCreateMutex();
    }

    void begin(uint8_t pedalId = 1) {
        usbSerial.begin(115200);
        
        #ifdef USB_JOYSTICK
          SetupController_USB(pedalId); 
        #endif
    }

    // ====================================================================
    // Überschriebene Stream-Methoden (Producer: JEDER Task darf schreiben)
    // ====================================================================
    
    size_t write(uint8_t c) override {
        if (!usbSerial) return 1; // PC liest nicht mit -> Daten verwerfen

        // Versuche den Mutex zu bekommen (Wartezeit 0, um Tasks wie pedalUpdateTask niemals zu blockieren!)
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
    
    size_t write(const uint8_t *buffer, size_t size) override {
        if (size == 0) return 0;
        if (!usbSerial) return size; // So tun als wäre gesendet worden

        if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
            size_t freeSpace = BUFFER_SIZE - availableBytes;
            
            // WICHTIG: Nur schreiben, wenn das KOMPLETTE Paket in den Puffer passt!
            // Sonst zerschießen wir das Protokoll mit halben Paketen, bei denen das EOF fehlt.
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

    int availableForWrite() override { 
        if (!usbSerial) return 0;
        return usbSerial.availableForWrite(); 
    }

    // ====================================================================
    // Lesemethoden (Consumer: serialCommunicationTaskRx)
    // ====================================================================
    int available() override { return usbSerial.available(); }
    int read() override { return usbSerial.read(); }
    int peek() override { return usbSerial.peek(); }
    void flush() override { usbSerial.flush(); }

    // ====================================================================
    // Multi-Tasking Batch-Processor (Wird von serialCommunicationTaskTx aufgerufen)
    // ====================================================================
    void processTxBatch() {
        if (!usbSerial) {
            // PC getrennt: Puffer leeren, damit kein alter Müll hängenbleibt
            if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                writeIdx = 0;
                readIdx = 0;
                availableBytes = 0;
                xSemaphoreGive(bufferMutex);
            }
            return;
        }

        // Wir feuern, wenn das Zeitintervall abgelaufen ist ODER der Puffer gut gefüllt ist (> 128 Bytes)
        bool timeToBatch = (millis() - lastBatchTimeMs >= batchIntervalMs);
        bool bufferFullEnough = false;
        
        if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
            bufferFullEnough = (availableBytes >= 128);
            xSemaphoreGive(bufferMutex);
        }

        if (timeToBatch || bufferFullEnough) {
            lastBatchTimeMs = millis();
            
            size_t avail = usbSerial.availableForWrite();
            if (avail > 0) {
                if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    size_t copyLen = (availableBytes < avail) ? availableBytes : avail;
                    
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
    }

    // ====================================================================
    // Joystick Wrapper
    // ====================================================================
    #ifdef USB_JOYSTICK
    bool isJoystickReady() {
        return IsControllerReady();
    }

    void sendJoystickValue(uint16_t val) {
        SetControllerOutputValue(val);
    }
    #endif
};