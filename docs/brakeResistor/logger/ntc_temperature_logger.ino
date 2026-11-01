// NTC temperature logger used for the brake resistor step-response measurement.
// Prints one CSV line per second: time_s,r_ntc_kohm,temp_c
#include <Arduino.h>
#include <math.h>

// --- Configuration ---
const int NTC_PIN = 4;               // ADC-capable pin (ESP32-S3: e.g. GPIO 4, ESP32: e.g. GPIO 34)
const float R_REF_OHM = 47000.0f;    // exact value of the pull-up resistor in Ohm (e.g. 47k or 100k)
const float V_SUPPLY_MV = 3300.0f;   // ESP32 3.3 V rail in mV (measure at the 3V3 pin if needed)

// NTC 100k parameters (3D printer standard)
const float NTC_R25_OHM = 100000.0f;
const float NTC_BETA = 3950.0f;
const float T25_KELVIN = 298.15f;

uint32_t start_time_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 11 dB attenuation for the full range (up to ~3.1 V / 3.3 V)
  analogSetPinAttenuation(NTC_PIN, ADC_11db);

  start_time_ms = millis();
  Serial.println("time_s,r_ntc_kohm,temp_c");
}

void loop() {
  // multisampling against ADC noise (64 samples)
  uint32_t mv_sum = 0;
  const int SAMPLES = 64;
  for (int i = 0; i < SAMPLES; i++) {
    mv_sum += analogReadMilliVolts(NTC_PIN);
    delayMicroseconds(500);
  }
  float v_out_mv = (float)mv_sum / SAMPLES;

  // voltage divider (NTC to GND, R_REF to 3.3 V):
  // R_NTC = R_REF * (V_out / (V_supply - V_out))
  float r_ntc_ohm = R_REF_OHM * (v_out_mv / (V_SUPPLY_MV - v_out_mv));

  // beta equation solved for temperature (degC)
  float inv_T = (1.0f / T25_KELVIN) + (logf(r_ntc_ohm / NTC_R25_OHM) / NTC_BETA);
  float temp_c = (1.0f / inv_T) - 273.15f;

  float time_s = (millis() - start_time_ms) * 0.001f;

  // CSV line (1 Hz)
  Serial.printf("%.2f,%.2f,%.2f\n", time_s, r_ntc_ohm * 0.001f, temp_c);

  delay(968); // together with 64 * 0.5 ms = 32 ms this gives a ~1 s period
}
