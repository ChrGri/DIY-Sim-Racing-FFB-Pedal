#include <Arduino.h>

static int counter = 0;

void greet() {
    Serial.printf("Hello World! counter = %d\n", counter);
    counter++;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== ESP32-S3 Debug Test ===");
}

void loop() {
    greet();
    delay(1000);
}
