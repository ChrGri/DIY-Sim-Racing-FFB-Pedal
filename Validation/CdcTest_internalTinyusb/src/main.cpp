#include <Arduino.h>
#include "UsbComManager.h" // Deine neue, Thread-sichere Klasse

// 1. Manager instanzieren (5 Millisekunden Batch-Intervall)
UsbComManager usbManager(5);

// 2. Pointer global bereitstellen (wie in deinem Hauptprojekt)
Stream *ActiveSerial = &usbManager;

// Globale Variablen für den Puffer (Echo/RX)
String currentInput = "";
String lastReceivedString = "Warte auf erste Eingabe...";

// ====================================================================
// Der simulierte TX/RX Task (Consumer - läuft z.B. auf Core 0)
// ====================================================================
void communicationTask(void * pvParameters) {
    for(;;) {
        // 1. RX simulieren: Zeichen lesen und String bauen
        while (ActiveSerial->available()) {
            char c = ActiveSerial->read();
            if (c == '\n' || c == '\r') {
                if (currentInput.length() > 0) {
                    lastReceivedString = currentInput;
                    currentInput = "";
                }
            } else {
                currentInput += c;
            }
        }

        // 2. TX simulieren: Batch-Puffer prüfen und ggf. per USB abfeuern
        // Hier greift der Mutex, sodass der String sicher aus dem RAM gelesen wird,
        // ohne dass die loop() währenddessen dazwischenfunkt.
        usbManager.processTxBatch();

        // 3. Task pausieren (Simuliert die Queue-Timeouts aus deinem Hauptprojekt)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ====================================================================
// Setup
// ====================================================================
void setup() {
    // Controller und CDC initialisieren (Pedal ID 1)
    usbManager.begin(1);

    // Dem PC kurz Zeit geben, die USB-Deskriptoren zu laden
    delay(2000);
    ActiveSerial->println("System gestartet: Thread-Safe CDC + HID aktiv!");

    // Starte den Consumer-Task auf Core 0. 
    // Dadurch kann die loop() auf Core 1 völlig ungebremst feuern.
    xTaskCreatePinnedToCore(
        communicationTask,   /* Task-Funktion */
        "ComTask",           /* Name */
        4096,                /* Stack-Größe */
        NULL,                /* Parameter */
        1,                   /* Priorität */
        NULL,                /* Task Handle */
        0                    /* Core ID (Core 0) */
    );
}

// ====================================================================
// Main Loop (Producer - läuft auf Core 1)
// ====================================================================
void loop() {
    unsigned long currentMicros = micros();
    
    static unsigned long lastSerialUpdate = 0;
    static unsigned long lastHidUpdate = 0;

    // 1. Hochfrequente Serial-Ausgaben (z.B. alle 250 µs = 4000 Hz)
    if (currentMicros - lastSerialUpdate >= 250) { 
        lastSerialUpdate = currentMicros;
        
        // Zykluszähler initialisieren und bei jedem Aufruf inkrementieren
        static uint32_t cycleCounter = 0;
        cycleCounter++;
        
        // Füllt die Daten in den RAM-Puffer (Eimer)
        ActiveSerial->print("Zyklus: ");
        ActiveSerial->print(cycleCounter);
        ActiveSerial->print(" | Letzter String: ");
        ActiveSerial->println(lastReceivedString);
    }

    // 2. Joystick Ausgabe (Alle 1000 µs = 1000 Hz)
    if (currentMicros - lastHidUpdate >= 1000) { 
        lastHidUpdate = currentMicros;
        
        if (usbManager.isJoystickReady()) {
            uint16_t val = ( (10 * millis()) % 65535);
            usbManager.sendJoystickValue(val);
        }
    }

    // 3. Den Batch-Puffer abarbeiten!
    // Leert den Eimer alle 5ms und sendet den Inhalt an den PC
    usbManager.processTxBatch();
}