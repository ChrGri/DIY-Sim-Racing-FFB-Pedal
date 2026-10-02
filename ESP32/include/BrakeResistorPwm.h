#pragma once

#include <Arduino.h>
#include "esp_timer.h"
#include "Main.h"

// PWM output for the external brake resistor (LEDC).
//
// Low frequency on purpose: the MOSFET (FR120N) is specified for 10 V gate drive and
// is driven from the 3.3 V GPIO, so its switching edges are slow. At 1 kHz the
// switching losses stay negligible; the bus ripple of a 10 Ohm / ~4 A pulse train is
// ~2 V at 50 % duty with ~1000 uF bus capacitance.
#define BRAKE_RESISTOR_PWM_FREQUENCY_HZ 1000U
#define BRAKE_RESISTOR_PWM_RESOLUTION_BITS 8U
// LEDC channel 0 (timer 0) is used by the buzzer through the IDF driver; channel 2
// runs on timer 1.
#define BRAKE_RESISTOR_PWM_LEDC_CHANNEL 2U

// Dead-man timer: the LEDC hardware keeps the last duty on its own, so a stalled or
// blocked writer (pedal task in a blocking delay, homing, crash of the task) would
// leave the resistor on, and the thermal model (same task) would stop as well. Every
// non-zero write re-arms a one-shot esp_timer; if no write follows within the timeout,
// the timer task switches the resistor off. The pedal task writes every 250-600 us,
// so it never fires in normal operation. Stuck-on energy is bounded to
// ~V^2/R * timeout (~1.6 J at 40 V / 10 Ohm).
#define BRAKE_RESISTOR_DEADMAN_TIMEOUT_US 10000U

// inline (not static): one shared instance across all translation units
inline esp_timer_handle_t g_brakeResistorDeadmanTimer = nullptr;
inline bool g_brakeResistorPwmAttached_b = false;

inline void brakeResistorDeadmanCallback(void *arg) {
#if defined(BRAKE_RESISTOR_PIN_U8) && (BRAKE_RESISTOR_PIN_U8 >= 0)
  if (g_brakeResistorPwmAttached_b) {
    ledcWrite(BRAKE_RESISTOR_PIN_U8, 0);
  } else {
    digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);
  }
#endif
}

inline void brakeResistorPwmInit() {
#if defined(BRAKE_RESISTOR_PIN_U8) && (BRAKE_RESISTOR_PIN_U8 >= 0)
  if (!g_brakeResistorPwmAttached_b) {
    pinMode(BRAKE_RESISTOR_PIN_U8, OUTPUT);
    digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);
    g_brakeResistorPwmAttached_b = ledcAttachChannel(
        BRAKE_RESISTOR_PIN_U8, BRAKE_RESISTOR_PWM_FREQUENCY_HZ,
        BRAKE_RESISTOR_PWM_RESOLUTION_BITS, BRAKE_RESISTOR_PWM_LEDC_CHANNEL);
  }
  if (g_brakeResistorDeadmanTimer == nullptr) {
    esp_timer_create_args_t timerArgs_st = {};
    timerArgs_st.callback = &brakeResistorDeadmanCallback;
    timerArgs_st.name = "brake_res_deadman";
    if (esp_timer_create(&timerArgs_st, &g_brakeResistorDeadmanTimer) != ESP_OK) {
      g_brakeResistorDeadmanTimer = nullptr;
    }
  }
  if (g_brakeResistorPwmAttached_b) {
    ledcWrite(BRAKE_RESISTOR_PIN_U8, 0);
  }
#endif
}

// duty in [0, 1]
inline void brakeResistorPwmWrite(float duty_01) {
#if defined(BRAKE_RESISTOR_PIN_U8) && (BRAKE_RESISTOR_PIN_U8 >= 0)
  const uint32_t maxDuty_u32 = (1U << BRAKE_RESISTOR_PWM_RESOLUTION_BITS) - 1U;
  uint32_t duty_u32 = (uint32_t)lroundf(constrain(duty_01, 0.0f, 1.0f) * (float)maxDuty_u32);

  // Fail safe: without PWM or without the dead-man timer the resistor stays off.
  if (!g_brakeResistorPwmAttached_b || (g_brakeResistorDeadmanTimer == nullptr)) {
    duty_u32 = 0;
  }

  if (duty_u32 > 0) {
    // arm before switching on, so a stall right after this write is covered
    if (esp_timer_is_active(g_brakeResistorDeadmanTimer)) {
      esp_timer_restart(g_brakeResistorDeadmanTimer, BRAKE_RESISTOR_DEADMAN_TIMEOUT_US);
    } else {
      esp_timer_start_once(g_brakeResistorDeadmanTimer, BRAKE_RESISTOR_DEADMAN_TIMEOUT_US);
    }
  }

  if (g_brakeResistorPwmAttached_b) {
    ledcWrite(BRAKE_RESISTOR_PIN_U8, duty_u32);
  } else {
    digitalWrite(BRAKE_RESISTOR_PIN_U8, LOW);
  }
#endif
}
