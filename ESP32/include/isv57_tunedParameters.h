#pragma once
#include <stdint.h>

// One iSV57 register: class X and number YY of PrX.YY, and its target value.
typedef struct {
  uint8_t class_u8;   // parameter class: the X in PrX.YY
  uint8_t number_u8;  // parameter number: the YY in PrX.YY
  int32_t value_i32;  // target value
} Isv57Parameter_t;

// Description: Array of tuned parameters derived from the file
// 'tuned-130_13_09_2025.h' Size: 305 elements Type: Signed 32-bit integer
//
// "Silent" parameter set (default). The "smooth" set with gain switching is
// isv57_tunedParameters_smooth.h: it overrides a few registers of this file.
// The plugin's Servo tab offers both on "Reset to recommended".
//
// Note: the firmware writes only the registers listed in
// Isv57Communication::sendTunedServoParameters(), always from this file. All
// other values here (loop gains Pr1.xx, filters Pr2.xx, Pr6.xx) reach the servo
// through the plugin ("Reset to recommended") or the Stepperonline app; the servo
// keeps them in its NVM. Signal flow and tuning notes:
// docs/development/servoLoops.md
#define ISV57_NMB_OF_REGISTERS 305

// Array index of the first register of each class (same as pr_X_00 in isv57communication.h)
constexpr uint16_t ISV57_CLASS_FIRST_INDEX[8] = {0, 25, 65, 95, 125, 175, 215, 255};
constexpr Isv57Parameter_t tuned_parameters[ISV57_NMB_OF_REGISTERS] = {
    // class, number, value
    { 0,  0,    500 },  // Pr0.00: Model-following control (MFC) bandwidth, unit 0.1 Hz (0 = off,
                        // 1 = automatic). iSV2 manual: keep >= Pr1.01. Written by the firmware at boot.
    { 0,  1,      0 },  // Pr0.01: Control mode
    { 0,  2,      0 },  // Pr0.02: Real-time auto-gain tuning mode
    { 0,  3,      9 },  // Pr0.03: Selection of machine stiffness at real-time...
    { 0,  4,    110 },  // Pr0.04: Ratio of inertia. Identified via Stepperonline App (on 26th
                        // March 2026)
    { 0,  5,      0 },  // Pr0.05: Command pulse input selection
    { 0,  6,      0 },  // Pr0.06: Motor rotational direction setup
    { 0,  7,      3 },  // Pr0.07: Reserved parameters
    { 0,  8,   3200 },  // Pr0.08: Microstep (3200 steps/rev: 213.3 kHz @ 4000 RPM, 800
                        // steps/pole pair)
    { 0,  9,      1 },  // Pr0.09: 1st numerator of electronic gear
    { 0, 10,      1 },  // Pr0.10: Denominator of electronic gear
    { 0, 11,   4000 },  // Pr0.11: Reserved parameters
    { 0, 12,      0 },  // Pr0.12: Reserved parameters
    { 0, 13,    500 },  // Pr0.13: 1st torque limit
    { 0, 14,    500 },  // Pr0.14: Position deviation setup
    { 0, 15,      0 },  // Pr0.15: Absolute encoder setup
    { 0, 16,     50 },  // Pr0.16: External regenerative resistor setup
    { 0, 17,     50 },  // Pr0.17: Regeneration discharge resistance power
    { 0, 18,      0 },  // Pr0.18: Vibration suppression - N after Stop
    { 0, 19,      0 },  // Pr0.19: Microseismic inhibition
    { 0, 20,      0 },  // Pr0.20: Activated pulse edge
    { 0, 21,  -1137 },  // Pr0.21: Reserved parameter
    { 0, 22,   -729 },  // Pr0.22: Reserved parameter
    { 0, 23,      0 },  // Pr0.23: Reserved parameter
    { 0, 24,      0 },  // Pr0.24: Reserved parameter
    { 1,  0,    600 },  // Pr1.00: 1st position loop gain
    { 1,  1,    400 },  // Pr1.01: 1st velocity loop gain
    { 1,  2,    200 },  // Pr1.02: 1st time constant of velocity loop integration (20 ms,
                        // the usual value for a 40 Hz velocity loop. Note: a shorter Ti
                        // adds phase lag and ringing, it does not reduce it)
    { 1,  3,     27 },  // Pr1.03: 1st filter of velocity detection
    { 1,  4,     20 },  // Pr1.04: 1st torque filter, unit 0.01 ms (0.2 ms). 180 (1.8 ms, against
                        // braking voltage spikes) added ~30 deg lag at 50 Hz to the velocity loop,
                        // which then no longer rejected the motor torque ripple (6 x 4 pole pairs
                        // = 24 per revolution): the pedal felt rough (2026-10-07)
    { 1,  5,    175 },  // Pr1.05: 2nd position loop gain
    { 1,  6,    110 },  // Pr1.06: 2nd velocity loop gain
    { 1,  7,  10000 },  // Pr1.07: 2nd time constant of velocity loop
    { 1,  8,      8 },  // Pr1.08: 2nd filter of velocity detection
    { 1,  9,    200 },  // Pr1.09: 2nd torque filter
    { 1, 10,     35 },  // Pr1.10: Velocity feed forward gain (unit 0.1 %: 35 = 3.5 %,
                        // practically off. Following error e = (1 - VFF) * v / Kp)
    { 1, 11,      0 },  // Pr1.11: Velocity feed forward filter. Had better velocity error decay
                        // with higher values and also less noise. Edit: 31.05.2026: Set to zero
                        // to keep coherency between feed forward and feedback. With non-zero
                        // values, the feed forward is smoothed, but the feedback is not, which
                        // will cause overshoot.
    { 1, 12,      0 },  // Pr1.12: Torque feed forward gain (unit 0.1 %)
    { 1, 13,   1000 },  // Pr1.13: Torque feed forward filter (10 ms; inactive while
                        // Pr1.12 = 0. When enabling torque FF set 0-50, otherwise the FF
                        // lags the feedback path, see docs/development/servoLoops.md)
    { 1, 14,      1 },  // Pr1.14: 2nd gain setup (enables the 2nd set)
    { 1, 15,      0 },  // Pr1.15: Gain switching mode (0 = 1st set fixed; the smooth set uses 10)
    { 1, 16,     50 },  // Pr1.16: Position control switching delay time
    { 1, 17,     50 },  // Pr1.17: Control switching level
    { 1, 18,     33 },  // Pr1.18: Control switch hysteresis
    { 1, 19,     33 },  // Pr1.19: Gain switching time
    { 1, 20,    100 },  // Pr1.20: Reserved parameter
    { 1, 21,    100 },  // Pr1.21: Reserved parameter
    { 1, 22,      0 },  // Pr1.22: Reserved parameter
    { 1, 23,    100 },  // Pr1.23: Speed ??regulator-kr
    { 1, 24,      0 },  // Pr1.24: Speed ??regulator-km
    { 1, 25,      0 },  // Pr1.25: Speed ??regulator-kd
    { 1, 26,     10 },  // Pr1.26: Filter
    { 1, 27,      0 },  // Pr1.27: Reserved parameter
    { 1, 28,  10000 },  // Pr1.28: 1st position loop integral time
    { 1, 29,      0 },  // Pr1.29: 1st position loop differential time
    { 1, 30,  10000 },  // Pr1.30: 2nd position loop integral time
    { 1, 31,      0 },  // Pr1.31: 2nd position loop differential time
    { 1, 32,     10 },  // Pr1.32: Position loop differential filter
    { 1, 33,      0 },  // Pr1.33: Speed given filter
    { 1, 34,      0 },  // Pr1.34: Reserved parameter
    { 1, 35,      0 },  // Pr1.35: Position command digital filter Settings
    { 1, 36,      0 },  // Pr1.36: Encoder feedback pulse digital filter Setting
    { 1, 37,     28 },  // Pr1.37: Special function register (0x04 | 0x08 | 0x10 = 28; Er0D0
                        // undervoltage masked)
    { 1, 38,      0 },  // Pr1.38: Reserved parameter
    { 1, 39,      0 },  // Pr1.39: Reserved parameter
    { 2,  0,      0 },  // Pr2.00: Adaptive filter mode setup (Ausgeschaltet für konstante
                        // Phasenlage ohne "Hunting")
    { 2,  1,     50 },  // Pr2.01: 1st notch frequency
    { 2,  2,     20 },  // Pr2.02: 1st notch width
    { 2,  3,     99 },  // Pr2.03: 1st notch depth
    { 2,  4,     90 },  // Pr2.04: 2nd notch frequency
    { 2,  5,     20 },  // Pr2.05: 2nd notch width
    { 2,  6,     99 },  // Pr2.06: 2nd notch depth
    { 2,  7,   2000 },  // Pr2.07: 3rd notch frequency
    { 2,  8,      0 },  // Pr2.08: 3rd notch width
    { 2,  9,      0 },  // Pr2.09: 3rd notch depth
    { 2, 10,   2000 },  // Pr2.10: 4th notch frequency
    { 2, 11,      0 },  // Pr2.11: 4th notch width
    { 2, 12,      0 },  // Pr2.12: 4th notch depth
    { 2, 13,      0 },  // Pr2.13: Selection of damping filter switching
    { 2, 14,      0 },  // Pr2.14: 1st damping frequency
    { 2, 15,      1 },  // Pr2.15: 1st damping filter
    { 2, 16,      0 },  // Pr2.16: 2nd damping frequency
    { 2, 17,      1 },  // Pr2.17: 2nd damping filter
    { 2, 18,      0 },  // Pr2.18: 3rd damping frequency
    { 2, 19,      0 },  // Pr2.19: 3rd damping filter
    { 2, 20,      0 },  // Pr2.20: 4th damping frequency
    { 2, 21,      0 },  // Pr2.21: 4th damping filter
    { 2, 22,      8 },  // Pr2.22: Positional command smoothing filter (PT1). 0.8ms reduziert
                        // Totzeit/Phasenverzug.
    { 2, 23,      5 },  // Pr2.23: Positional command FIR filter. 0.5ms Moving Average fuer
                        // weichen Motorlauf.
    { 2, 24,      0 },  // Pr2.24: Reserved parameter
    { 2, 25,      0 },  // Pr2.25: Reserved parameter
    { 2, 26,      0 },  // Pr2.26: Reserved parameter
    { 2, 27,      0 },  // Pr2.27: Reserved parameter
    { 2, 28,      0 },  // Pr2.28: Reserved parameter
    { 2, 29,      0 },  // Pr2.29: Reserved parameter
    { 3,  0,      0 },  // Pr3.00: Velocity setup internal and external switching
    { 3,  1,      1 },  // Pr3.01: Speed command rotational direction
    { 3,  2,    500 },  // Pr3.02: Speed command input gain
    { 3,  3,      0 },  // Pr3.03: Speed command reversal input
    { 3,  4,      0 },  // Pr3.04: 1st speed setup
    { 3,  5,      0 },  // Pr3.05: 2nd speed setup
    { 3,  6,      0 },  // Pr3.06: 3rd speed setup
    { 3,  7,      0 },  // Pr3.07: 4th speed setup
    { 3,  8,      0 },  // Pr3.08: 5th speed setup
    { 3,  9,      0 },  // Pr3.09: 6th speed setup
    { 3, 10,      0 },  // Pr3.10: 7th speed setup
    { 3, 11,      0 },  // Pr3.11: 8th speed setup
    { 3, 12,      0 },  // Pr3.12: Time setup acceleration
    { 3, 13,      0 },  // Pr3.13: Time setup deceleration
    { 3, 14,      0 },  // Pr3.14: Sigmoid acceleration/deceleration time se...
    { 3, 15,      0 },  // Pr3.15: Speed zero-clamp function selection
    { 3, 16,     30 },  // Pr3.16: Speed zero-clamp level
    { 3, 17,      0 },  // Pr3.17: Torque command internal and external swi...
    { 3, 18,      0 },  // Pr3.18: Torque command direction selection
    { 3, 19,     30 },  // Pr3.19: Torque command input gain
    { 3, 20,      0 },  // Pr3.20: Torque command input reversal
    { 3, 21,      0 },  // Pr3.21: Speed limit value 1
    { 3, 22,      0 },  // Pr3.22: Speed limit value 2
    { 3, 23,      0 },  // Pr3.23: Reserved parameter
    { 3, 24,   5000 },  // Pr3.24: Maximum speed of motor rotation
    { 3, 25,      0 },  // Pr3.25: Reserved parameter
    { 3, 26,      0 },  // Pr3.26: Reserved parameter
    { 3, 27,      0 },  // Pr3.27: Reserved parameter
    { 3, 28,      0 },  // Pr3.28: Reserved parameter
    { 3, 29,      0 },  // Pr3.29: Reserved parameter
    { 4,  0,   3084 },  // Pr4.00: Input selection SI1
    { 4,  1,   3341 },  // Pr4.01: Input selection SI2
    { 4,  2,   5654 },  // Pr4.02: Input selection SI3
    { 4,  3,   5911 },  // Pr4.03: Input selection SI4
    { 4,  4,   6168 },  // Pr4.04: Input selection SI5
    { 4,  5,     18 },  // Pr4.05: Input selection SI6
    { 4,  6,   4608 },  // Pr4.06: Input selection SI7
    { 4,  7,   3584 },  // Pr4.07: Input selection SI8
    { 4,  8, 0x0303 },  // Pr4.08: Input selection SI9
    { 4,  9,      0 },  // Pr4.09: Input selection SI10
    { 4, 10,  0x101 },  // Pr4.10: Output selection SO1 81: No ALARM ==> 2.4Ohm, (formerly 0x8181)
                        // ALARM ==> inf Ohm.    Connect A: NO. Connect B: NC
    { 4, 11,      0 },  // Pr4.11: Output selection SO2
    { 4, 12,      0 },  // Pr4.12: Output selection SO3
    { 4, 13,      0 },  // Pr4.13: Output selection SO4
    { 4, 14,      0 },  // Pr4.14: Output selection SO5
    { 4, 15,      9 },  // Pr4.15: Output selection SO6
    { 4, 16,     10 },  // Pr4.16: Analog monitor 1 type
    { 4, 17,     11 },  // Pr4.17: Analog monitor 1 output gain
    { 4, 18,     12 },  // Pr4.18: Analog monitor 2 type
    { 4, 19,     13 },  // Pr4.19: Analog monitor 2 output gain
    { 4, 20,     14 },  // Pr4.20: Type of digital monitor
    { 4, 21,     15 },  // Pr4.21: Analog monitor output setup
    { 4, 22,      0 },  // Pr4.22: Analog input 1(AI 1) offset setup
    { 4, 23,      0 },  // Pr4.23: Analog input 1(AI 1) filter
    { 4, 24,      0 },  // Pr4.24: Analog input 1(AI 1) overvoltage setup
    { 4, 25,      0 },  // Pr4.25: Analog input 2(AI 2) offset setup
    { 4, 26,      0 },  // Pr4.26: Analog input 1(AI 2) filter
    { 4, 27,      0 },  // Pr4.27: Analog input 1(AI 2) overvoltage setup
    { 4, 28,      0 },  // Pr4.28: Analog input 3(AI 3) offset setup
    { 4, 29,      0 },  // Pr4.29: Analog input 3(AI 3) filter
    { 4, 30,      0 },  // Pr4.30: Analog input 3(AI 3) overvoltage setup
    { 4, 31,     10 },  // Pr4.31: Positioning complete range
    { 4, 32,      0 },  // Pr4.32: Positioning complete output setup
    { 4, 33,      0 },  // Pr4.33: INP hold time
    { 4, 34,     50 },  // Pr4.34: Zero-speed
    { 4, 35,     50 },  // Pr4.35: Speed coincidence range
    { 4, 36,   1000 },  // Pr4.36: At-speed
    { 4, 37,      0 },  // Pr4.37: Mechanical brake action at stalling setup
    { 4, 38,      0 },  // Pr4.38: Mechanical brake action at running setup
    { 4, 39,     30 },  // Pr4.39: Brake release speed setup
    { 4, 40,      0 },  // Pr4.40: Selection of alarm output 1
    { 4, 41,      0 },  // Pr4.41: Selection of alarm output 2
    { 4, 42,     10 },  // Pr4.42: 2nd positioning complete range
    { 4, 43,      1 },  // Pr4.43: E-stop function selection
    { 4, 44,      0 },  // Pr4.44: Input selection SI11
    { 4, 45,      0 },  // Pr4.45: Input selection SI12
    { 4, 46,      0 },  // Pr4.46: Input selection SI13
    { 4, 47,      0 },  // Pr4.47: Input selection SI14
    { 4, 48,     50 },  // Pr4.48: Reserved parameter
    { 4, 49,    500 },  // Pr4.49: Reserved parameter
    { 5,  0,      1 },  // Pr5.00: 2nd numerator of electronic gear
    { 5,  1,      1 },  // Pr5.01: 3rd numerator of electronic gear
    { 5,  2,      1 },  // Pr5.02: 4th numerator of electronic gear
    { 5,  3,   2500 },  // Pr5.03: Denominator of pulse output division
    { 5,  4,      0 },  // Pr5.04: Over-travel inhibit input setup
    { 5,  5,      0 },  // Pr5.05: Sequence at over-travel inhibit
    { 5,  6,      0 },  // Pr5.06: Sequence at servo-off
    { 5,  7,      0 },  // Pr5.07: Main power off sequence
    { 5,  8,      0 },  // Pr5.08: Main power off LV trip selection
    { 5,  9,     70 },  // Pr5.09: Main power off detection time
    { 5, 10,      0 },  // Pr5.10: Sequence at alarm
    { 5, 11,      0 },  // Pr5.11: Torque setup for emergency stop
    { 5, 12,      0 },  // Pr5.12: Over-load level setup
    { 5, 13,   5000 },  // Pr5.13: Over-speed level setup
    { 5, 14,  -2241 },  // Pr5.14: Motor working range setup
    { 5, 15,  -4117 },  // Pr5.15: I/F reading filter
    { 5, 16,      0 },  // Pr5.16: Alarm clear input setup
    { 5, 17,      3 },  // Pr5.17: Counter clear input setup
    { 5, 18,      0 },  // Pr5.18: Command pulse inhibit input invalidation
    { 5, 19,      0 },  // Pr5.19: Command pulse inhibit input reading setup
    { 5, 20,      1 },  // Pr5.20: Position setup unit select
    { 5, 21,      0 },  // Pr5.21: Selection of torque limit
    { 5, 22,    300 },  // Pr5.22: 2nd torque limit
    { 5, 23,      0 },  // Pr5.23: Torque limit switching setup 1
    { 5, 24,      0 },  // Pr5.24: Torque limit switching setup 2
    { 5, 25,      0 },  // Pr5.25: External input positive direction torque limit
    { 5, 26,      0 },  // Pr5.26: External input negative direction torque limit
    { 5, 27,     30 },  // Pr5.27: Input gain of analog torque limit
    { 5, 28,      1 },  // Pr5.28: LED initial status
    { 5, 29,     21 },  // Pr5.29: RS232 communication baud rate setup
    { 5, 30,      2 },  // Pr5.30: RS485 communication baud rate setup
    { 5, 31,      1 },  // Pr5.31: Axis address
    { 5, 32,      0 },  // Pr5.32: Command pulse input maximum setup
    { 5, 33,      0 },  // Pr5.33: Pulse regenerative output limit setup
    { 5, 34,      0 },  // Pr5.34: Reserved parameter
    { 5, 35,      1 },  // Pr5.35: Front panel lock setup
    { 5, 36,      0 },  // Pr5.36: Reserved parameter
    { 5, 37,    322 },  // Pr5.37: Reserved parameter
    { 5, 38,   5000 },  // Pr5.38: Reserved parameter
    { 5, 39,      0 },  // Pr5.39: Reserved parameter
    { 6,  0,      0 },  // Pr6.00: Analog torque feed forward conversion gain
    { 6,  1,      0 },  // Pr6.01: Encoder zero position compensation
    { 6,  2,      0 },  // Pr6.02: Velocity deviation excess setup
    { 6,  3,      0 },  // Pr6.03: JOG trial run command torque
    { 6,  4,    300 },  // Pr6.04: JOG trial run command speed
    { 6,  5,      0 },  // Pr6.05: Position 3rd gain valid time
    { 6,  6,    100 },  // Pr6.06: Position 3rd gain scale factor
    { 6,  7,      0 },  // Pr6.07: Torque command additional value
    { 6,  8,      0 },  // Pr6.08: Positive direction torque compensation val...
    { 6,  9,      0 },  // Pr6.09: Negative direction torque compensation v...
    { 6, 10,      0 },  // Pr6.10: Function expansion setup
    { 6, 11,     50 },  // Pr6.11: Current response setup
    { 6, 12,     25 },  // Pr6.12: Encoder zero correction torque limiter set
    { 6, 13,      0 },  // Pr6.13: 2nd inertia ratio
    { 6, 14,    200 },  // Pr6.14: Emergency stop time at alarm
    { 6, 15,      0 },  // Pr6.15: 2nd over-speed level setup
    { 6, 16,      0 },  // Pr6.16: Running mode
    { 6, 17,      0 },  // Pr6.17: Front panel parameter writing selection
    { 6, 18,      0 },  // Pr6.18: Power-up wait time
    { 6, 19,      0 },  // Pr6.19: Encoder Z phase setup
    { 6, 20,     10 },  // Pr6.20: Trial running distance
    { 6, 21,    200 },  // Pr6.21: Trial running wait time
    { 6, 22,      1 },  // Pr6.22: Trial running cycle times
    { 6, 23,     30 },  // Pr6.23: Disturbance torque compensating gain (30% Disturbance
                        // Observer for active cogging & stiction rejection)
    { 6, 24,     15 },  // Pr6.24: Disturbance observer filter (unit 0.01 ms in the A5
                        // parameter map: 0.15 ms, not 1.5 ms. To be checked)
    { 6, 25,      0 },  // Pr6.25: Reserved parameter
    { 6, 26,      0 },  // Pr6.26: Reserved parameter
    { 6, 27,      0 },  // Pr6.27: Alarm latch time selection
    { 6, 28,      0 },  // Pr6.28: Reserved parameter
    { 6, 29,      0 },  // Pr6.29: Reserved parameter
    { 6, 30,      0 },  // Pr6.30: Reserved parameter
    { 6, 31,      0 },  // Pr6.31: Real-time auto tuning estimation speed
    { 6, 32,      0 },  // Pr6.32: Real-time auto tuning custom setup
    { 6, 33,     25 },  // Pr6.33: Reserved parameter
    { 6, 34,      0 },  // Pr6.34: Reserved parameter
    { 6, 35,      0 },  // Pr6.35: Reserved parameter
    { 6, 36,      0 },  // Pr6.36: Reserved parameter
    { 6, 37,      0 },  // Pr6.37: Oscillation detection level
    { 6, 38,      0 },  // Pr6.38: Alarm mask setup
    { 6, 39,      0 },  // Pr6.39: Reserved parameter
    { 7,  0,    500 },  // Pr7.00: Current loop gain
    { 7,  1,     50 },  // Pr7.01: Current loop integral time
    { 7,  2,    106 },  // Pr7.02: Motor rotor initial position Angle compensa...
    { 7,  3,      0 },  // Pr7.03: Reserved parameter
    { 7,  4,    360 },  // Pr7.04: Motor rated voltage
    { 7,  5,      4 },  // Pr7.05: Motor pole pairs
    { 7,  6,     82 },  // Pr7.06: Motor phase resistor
    { 7,  7,     84 },  // Pr7.07: Motor D/Q inductance
    { 7,  8,     56 },  // Pr7.08: Motor back EMF coefficient
    { 7,  9,     80 },  // Pr7.09: Motor torque coefficient
    { 7, 10,   3000 },  // Pr7.10: Motor rated speed
    { 7, 11,   4000 },  // Pr7.11: Motor Maximum speed
    { 7, 12,    763 },  // Pr7.12: Motor rated current
    { 7, 13,     40 },  // Pr7.13: Motor rotor inertia
    { 7, 14,    130 },  // Pr7.14: Motor power selection
    { 7, 15,      2 },  // Pr7.15: Motor model input
    { 7, 16,      0 },  // Pr7.16: Encoder selection
    { 7, 17,    250 },  // Pr7.17: Motor maximum current
    { 7, 18,    338 },  // Pr7.18: Encoder Index Angle compensation
    { 7, 19,      0 },  // Pr7.19: Reserved parameter
    { 7, 20,      0 },  // Pr7.20: Drive model input
    { 7, 21,      0 },  // Pr7.21: Servo model input
    { 7, 22,    304 },  // Pr7.22: Reserved parameter
    { 7, 23,     15 },  // Pr7.23: Reserved parameter
    { 7, 24,      0 },  // Pr7.24: Fan control mode setting
    { 7, 25,     50 },  // Pr7.25: Fan open temperature setting
    { 7, 26,      5 },  // Pr7.26: Fan temperature control hysteresis
    { 7, 27,     80 },  // Pr7.27: Drive over-temperature alarm threshold se...
    { 7, 28,     30 },  // Pr7.28: Time of Bleeder alarm window
    { 7, 29,      0 },  // Pr7.29: Dc bus voltage detection filter
    { 7, 30,     16 },  // Pr7.30: Under-voltage point set
    { 7, 31,      1 },  // Pr7.31: Bleeder control mode setting (1 = Enable reactive pump lift
                        // suppression function)
    { 7, 32,     40 },  // Pr7.32: Bleeder open the threshold set
    { 7, 33,      1 },  // Pr7.33: Bleeder control hysteresis
    { 7, 34,     72 },  // Pr7.34: Overvoltage point set
    { 7, 35,      1 },  // Pr7.35: Relay control mode setting
    { 7, 36,     18 },  // Pr7.36: Threshold setting of relay suction
    { 7, 37,   2048 },  // Pr7.37: Analog quantity AI3 hardware zero drift co...
    { 7, 38,   2048 },  // Pr7.38: Analog quantity AI3 hardware zero drift co...
    { 7, 39,   2048 },  // Pr7.39: Analog quantity AI3 hardware zero drift co...
    { 7, 40,     21 },  // Pr7.40: RS232 communication mode settings
    { 7, 41,      4 },  // Pr7.41: RS485 communication baud rate settings
    { 7, 42,     63 },  // Pr7.42: RS232-ID
    { 7, 43,      0 },  // Pr7.43: DC bus voltage hardware zero drift compe...
    { 7, 44,      0 },  // Pr7.44: Temperature measurement hardware zero...
    { 7, 45,  10000 },  // Pr7.45: Dc bus voltage hardware slope coefficient
    { 7, 46,  10000 },  // Pr7.46: Current U phase sampling hardware slope...
    { 7, 47,  10000 },  // Pr7.47: Current V phase sampling hardware slope...
    { 7, 48,     18 },  // Pr7.48: Reserved parameter
    { 7, 49,    500 },  // Pr7.49: Reserved parameter
};

// Every entry must sit at the index of its register: index = ISV57_CLASS_FIRST_INDEX[class] +
// number. A missing or duplicated line would shift all following registers.
constexpr bool isv57TunedParametersInOrder() {
  for (uint16_t i = 0; i < ISV57_NMB_OF_REGISTERS; i++) {
    const Isv57Parameter_t &p = tuned_parameters[i];
    if ((p.class_u8 > 7) || (ISV57_CLASS_FIRST_INDEX[p.class_u8] + p.number_u8 != i)) {
      return false;
    }
  }
  return true;
}
static_assert(isv57TunedParametersInOrder(),
              "isv57_tunedParameters.h: an entry is not at the index of its register");
