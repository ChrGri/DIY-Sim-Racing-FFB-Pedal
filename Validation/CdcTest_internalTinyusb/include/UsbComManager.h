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
    String txBatchBuffer;
    unsigned long lastBatchTimeMs;
    uint32_t batchIntervalMs;

public:
    // Standard-Batch-Intervall: 5 Millisekunden
    UsbComManager(uint32_t intervalMs = 5) {
        batchIntervalMs = intervalMs;
        lastBatchTimeMs = 0;
        
        // Mutex erstellen, um Thread-Sicherheit zwischen den Tasks zu garantieren
        bufferMutex = xSemaphoreCreateMutex();
        
        // Speicher fest reservieren, um Heap-Fragmentierung zu verhindern
        txBatchBuffer.reserve(2048); 
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
            if (txBatchBuffer.length() < 2000) {
                txBatchBuffer += (char)c;
            }
            xSemaphoreGive(bufferMutex);
        }
        return 1;
    }
    
    size_t write(const uint8_t *buffer, size_t size) override {
        if (size == 0) return 0;
        if (!usbSerial) return size; // So tun als wäre gesendet worden

        if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
            if (txBatchBuffer.length() + size < 2000) {
                txBatchBuffer.concat((const char*)buffer, size);
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
    // ====================================================================
    // Multi-Tasking Batch-Processor (Wird von serialCommunicationTaskTx aufgerufen)
    // ====================================================================
    void processTxBatch() {
        if (!usbSerial) {
            // PC getrennt: Puffer leeren, damit kein alter Müll hängenbleibt
            if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
                txBatchBuffer = "";
                xSemaphoreGive(bufferMutex);
            }
            return;
        }

        // Wir feuern, wenn das Zeitintervall abgelaufen ist ODER der Puffer gut gefüllt ist (> 128 Bytes)
        bool timeToBatch = (millis() - lastBatchTimeMs >= batchIntervalMs);
        bool bufferFullEnough = false;
        
        if (xSemaphoreTake(bufferMutex, 0) == pdTRUE) {
            bufferFullEnough = (txBatchBuffer.length() >= 128);
            xSemaphoreGive(bufferMutex);
        }

        if (timeToBatch || bufferFullEnough) {
            lastBatchTimeMs = millis();
            
            size_t avail = usbSerial.availableForWrite();
            if (avail > 0) {
                String dataToSend = "";
                
                if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    size_t bufferLen = txBatchBuffer.length();
                    if (bufferLen > 0) {
                        // DER MAGISCHE FIX: Nimm nur exakt so viele Zeichen, 
                        // wie die USB-Hardware jetzt gerade schlucken kann!
                        size_t copyLen = (bufferLen < avail) ? bufferLen : avail;
                        dataToSend = txBatchBuffer.substring(0, copyLen);
                        
                        // Entferne nur die gesendeten Zeichen, der Rest bleibt sicher im Puffer!
                        txBatchBuffer.remove(0, copyLen); 
                    }
                    xSemaphoreGive(bufferMutex);
                }

                if (dataToSend.length() > 0) {
                    usbSerial.print(dataToSend);
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