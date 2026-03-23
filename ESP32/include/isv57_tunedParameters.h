#include <stdint.h>

// Description: Array of tuned parameters derived from the file 'tuned-130_13_09_2025.h'
// Size: 305 elements
// Type: Signed 32-bit integer
#define ISV57_NMB_OF_REGISTERS 305
const int32_t tuned_parameters[ISV57_NMB_OF_REGISTERS] = {
    1,     // Pr0.00: Reserved parameters
    0,     // Pr0.01: Control mode
    0,     // Pr0.02: Real-time auto-gain tuning mode
    9,    // Pr0.03: Selection of machine stiffness at real-time...
    1,    // Pr0.04: Ratio of inertia
    0,     // Pr0.05: Command pulse input selection
    0,     // Pr0.06: Motor rotational direction setup
    3,     // Pr0.07: Reserved parameters
    3780,  // Pr0.08: Microstep
    1,     // Pr0.09: 1st numerator of electronic gear
    1,     // Pr0.10: Denominator of electronic gear
    4000,  // Pr0.11: Reserved parameters
    0,     // Pr0.12: Reserved parameters
    500,   // Pr0.13: 1st torque limit
    500,   // Pr0.14: Position deviation setup
    0,     // Pr0.15: Absolute encoder setup
    50,    // Pr0.16: External regenerative resistor setup
    50,    // Pr0.17: Regeneration discharge resistance power
    0,     // Pr0.18: Vibration suppression - N after Stop
    0,     // Pr0.19: Microseismic inhibition
    0,     // Pr0.20: Activated pulse edge
    -1137, // Pr0.21: Reserved parameter
    -729,  // Pr0.22: Reserved parameter
    0,     // Pr0.23: Reserved parameter
    0,     // Pr0.24: Reserved parameter
    900,   // Pr1.00: 1st position loop gain
    900,   // Pr1.01: 1st velocity loop gain
    4000,   // Pr1.02: 1st time constant of velocity loop integration
    9,    // Pr1.03: 1st filter of velocity detection
    200,   // Pr1.04: 1st torque filter
    175,   // Pr1.05: 2nd position loop gain
    110,   // Pr1.06: 2nd velocity loop gain
    10000, // Pr1.07: 2nd time constant of velocity loop
    9,     // Pr1.08: 2nd filter of velocity detection
    200,   // Pr1.09: 2nd torque filter
    999,   // Pr1.10: Velocity feed forward gain
    1000,  // Pr1.11: Velocity feed forward filter
    999,     // Pr1.12: Torque feed forward gain
    1000,     // Pr1.13: Torque feed forward filter
    1,     // Pr1.14: 2nd gain setup
    0,     // Pr1.15: Control switching mode
    50,    // Pr1.16: Position control switching delay time
    50,    // Pr1.17: Control switching level
    33,    // Pr1.18: Control switch hysteresis
    33,    // Pr1.19: Gain switching time
    100,   // Pr1.20: Reserved parameter
    100,   // Pr1.21: Reserved parameter
    0,     // Pr1.22: Reserved parameter
    100,   // Pr1.23: Speed ??regulator-kr
    0,     // Pr1.24: Speed ??regulator-km
    0,     // Pr1.25: Speed ??regulator-kd
    10,    // Pr1.26: Filter
    0,     // Pr1.27: Reserved parameter
    10000, // Pr1.28: 1st position loop integral time
    0,     // Pr1.29: 1st position loop differential time
    10000, // Pr1.30: 2nd position loop integral time
    0,     // Pr1.31: 2nd position loop differential time
    10,    // Pr1.32: Position loop differential filter
    0,     // Pr1.33: Speed given filter
    0,     // Pr1.34: Reserved parameter
    0,     // Pr1.35: Position command digital filter Settings
    0,     // Pr1.36: Encoder feedback pulse digital filter Setting
    1052,  // Pr1.37: Special function register
    0,     // Pr1.38: Reserved parameter
    0,     // Pr1.39: Reserved parameter
    2,     // Pr2.00: Adaptive filter mode setup
    50,    // Pr2.01: 1st notch frequency
    20,    // Pr2.02: 1st notch width
    99,    // Pr2.03: 1st notch depth
    90,    // Pr2.04: 2nd notch frequency
    20,    // Pr2.05: 2nd notch width
    99,    // Pr2.06: 2nd notch depth
    2000,  // Pr2.07: 3rd notch frequency
    0,     // Pr2.08: 3rd notch width
    0,     // Pr2.09: 3rd notch depth
    2000,  // Pr2.10: 4th notch frequency
    0,     // Pr2.11: 4th notch width
    0,     // Pr2.12: 4th notch depth
    0,     // Pr2.13: Selection of damping filter switching
    0,     // Pr2.14: 1st damping frequency
    1,     // Pr2.15: 1st damping filter
    0,     // Pr2.16: 2nd damping frequency
    1,     // Pr2.17: 2nd damping filter
    0,     // Pr2.18: 3rd damping frequency
    0,     // Pr2.19: 3rd damping filter
    0,     // Pr2.20: 4th damping frequency
    0,     // Pr2.21: 4th damping filter
    0,    // Pr2.22: Positional command smoothing filter
    0,     // Pr2.23: Positional command FIR filter
    0,     // Pr2.24: Reserved parameter
    0,     // Pr2.25: Reserved parameter
    0,     // Pr2.26: Reserved parameter
    0,     // Pr2.27: Reserved parameter
    0,     // Pr2.28: Reserved parameter
    0,     // Pr2.29: Reserved parameter
    0,     // Pr3.00: Velocity setup internal and external switching
    1,     // Pr3.01: Speed command rotational direction
    500,   // Pr3.02: Speed command input gain
    0,     // Pr3.03: Speed command reversal input
    0,     // Pr3.04: 1st speed setup
    0,     // Pr3.05: 2nd speed setup
    0,     // Pr3.06: 3rd speed setup
    0,     // Pr3.07: 4th speed setup
    0,     // Pr3.08: 5th speed setup
    0,     // Pr3.09: 6th speed setup
    0,     // Pr3.10: 7th speed setup
    0,     // Pr3.11: 8th speed setup
    0,     // Pr3.12: Time setup acceleration
    0,     // Pr3.13: Time setup deceleration
    0,     // Pr3.14: Sigmoid acceleration/deceleration time se...
    0,     // Pr3.15: Speed zero-clamp function selection
    30,    // Pr3.16: Speed zero-clamp level
    0,     // Pr3.17: Torque command internal and external swi...
    0,     // Pr3.18: Torque command direction selection
    30,    // Pr3.19: Torque command input gain
    0,     // Pr3.20: Torque command input reversal
    0,     // Pr3.21: Speed limit value 1
    0,     // Pr3.22: Speed limit value 2
    0,     // Pr3.23: Reserved parameter
    5000,  // Pr3.24: Maximum speed of motor rotation
    0,     // Pr3.25: Reserved parameter
    0,     // Pr3.26: Reserved parameter
    0,     // Pr3.27: Reserved parameter
    0,     // Pr3.28: Reserved parameter
    0,     // Pr3.29: Reserved parameter
    3084,  // Pr4.00: Input selection SI1
    3341,  // Pr4.01: Input selection SI2
    5654,  // Pr4.02: Input selection SI3
    5911,  // Pr4.03: Input selection SI4
    6168,  // Pr4.04: Input selection SI5
    18,    // Pr4.05: Input selection SI6
    4608,  // Pr4.06: Input selection SI7
    3584,  // Pr4.07: Input selection SI8
    0x0303, // Pr4.08: Input selection SI9
    0,     // Pr4.09: Input selection SI10
    4369,  // Pr4.10: Output selection SO1
    0,     // Pr4.11: Output selection SO2
    0,     // Pr4.12: Output selection SO3
    0,     // Pr4.13: Output selection SO4
    0,     // Pr4.14: Output selection SO5
    9,     // Pr4.15: Output selection SO6
    10,    // Pr4.16: Analog monitor 1 type
    11,    // Pr4.17: Analog monitor 1 output gain
    12,    // Pr4.18: Analog monitor 2 type
    13,    // Pr4.19: Analog monitor 2 output gain
    14,    // Pr4.20: Type of digital monitor
    15,    // Pr4.21: Analog monitor output setup
    0,     // Pr4.22: Analog input 1(AI 1) offset setup
    0,     // Pr4.23: Analog input 1(AI 1) filter
    0,     // Pr4.24: Analog input 1(AI 1) overvoltage setup
    0,     // Pr4.25: Analog input 2(AI 2) offset setup
    0,     // Pr4.26: Analog input 1(AI 2) filter
    0,     // Pr4.27: Analog input 1(AI 2) overvoltage setup
    0,     // Pr4.28: Analog input 3(AI 3) offset setup
    0,     // Pr4.29: Analog input 3(AI 3) filter
    0,     // Pr4.30: Analog input 3(AI 3) overvoltage setup
    10,    // Pr4.31: Positioning complete range
    0,     // Pr4.32: Positioning complete output setup
    0,     // Pr4.33: INP hold time
    50,    // Pr4.34: Zero-speed
    50,    // Pr4.35: Speed coincidence range
    1000,  // Pr4.36: At-speed
    0,     // Pr4.37: Mechanical brake action at stalling setup
    0,     // Pr4.38: Mechanical brake action at running setup
    30,    // Pr4.39: Brake release speed setup
    0,     // Pr4.40: Selection of alarm output 1
    0,     // Pr4.41: Selection of alarm output 2
    10,    // Pr4.42: 2nd positioning complete range
    1,     // Pr4.43: E-stop function selection
    0,     // Pr4.44: Input selection SI11
    0,     // Pr4.45: Input selection SI12
    0,     // Pr4.46: Input selection SI13
    0,     // Pr4.47: Input selection SI14
    50,    // Pr4.48: Reserved parameter
    500,   // Pr4.49: Reserved parameter
    1,     // Pr5.00: 2nd numerator of electronic gear
    1,     // Pr5.01: 3rd numerator of electronic gear
    1,     // Pr5.02: 4th numerator of electronic gear
    2500,  // Pr5.03: Denominator of pulse output division
    0,     // Pr5.04: Over-travel inhibit input setup
    0,     // Pr5.05: Sequence at over-travel inhibit
    0,     // Pr5.06: Sequence at servo-off
    0,     // Pr5.07: Main power off sequence
    1,     // Pr5.08: Main power off LV trip selection
    70,    // Pr5.09: Main power off detection time
    0,     // Pr5.10: Sequence at alarm
    0,     // Pr5.11: Torque setup for emergency stop
    0,     // Pr5.12: Over-load level setup
    5000,  // Pr5.13: Over-speed level setup
    -2241, // Pr5.14: Motor working range setup
    -4117, // Pr5.15: I/F reading filter
    0,     // Pr5.16: Alarm clear input setup
    3,     // Pr5.17: Counter clear input setup
    0,     // Pr5.18: Command pulse inhibit input invalidation
    0,     // Pr5.19: Command pulse inhibit input reading setup
    1,     // Pr5.20: Position setup unit select
    0,     // Pr5.21: Selection of torque limit
    300,   // Pr5.22: 2nd torque limit
    0,     // Pr5.23: Torque limit switching setup 1
    0,     // Pr5.24: Torque limit switching setup 2
    0,     // Pr5.25: External input positive direction torque limit
    0,     // Pr5.26: External input negative direction torque limit
    30,    // Pr5.27: Input gain of analog torque limit
    1,     // Pr5.28: LED initial status
    21,    // Pr5.29: RS232 communication baud rate setup
    2,     // Pr5.30: RS485 communication baud rate setup
    1,     // Pr5.31: Axis address
    0,     // Pr5.32: Command pulse input maximum setup
    0,     // Pr5.33: Pulse regenerative output limit setup
    0,     // Pr5.34: Reserved parameter
    1,     // Pr5.35: Front panel lock setup
    0,     // Pr5.36: Reserved parameter
    322,   // Pr5.37: Reserved parameter
    5000,  // Pr5.38: Reserved parameter
    0,     // Pr5.39: Reserved parameter
    0,     // Pr6.00: Analog torque feed forward conversion gain
    0,     // Pr6.01: Encoder zero position compensation
    0,     // Pr6.02: Velocity deviation excess setup
    0,     // Pr6.03: JOG trial run command torque
    300,   // Pr6.04: JOG trial run command speed
    0,     // Pr6.05: Position 3rd gain valid time
    100,   // Pr6.06: Position 3rd gain scale factor
    0,     // Pr6.07: Torque command additional value
    0,     // Pr6.08: Positive direction torque compensation val...
    0,     // Pr6.09: Negative direction torque compensation v...
    0,     // Pr6.10: Function expansion setup
    100,   // Pr6.11: Current response setup
    25,    // Pr6.12: Encoder zero correction torque limiter set
    0,     // Pr6.13: 2nd inertia ratio
    200,   // Pr6.14: Emergency stop time at alarm
    0,     // Pr6.15: 2nd over-speed level setup
    0,     // Pr6.16: Running mode
    0,     // Pr6.17: Front panel parameter writing selection
    0,     // Pr6.18: Power-up wait time
    0,     // Pr6.19: Encoder Z phase setup
    10,    // Pr6.20: Trial running distance
    200,   // Pr6.21: Trial running wait time
    1,     // Pr6.22: Trial running cycle times
    0,     // Pr6.23: Disturbance torque compensating gain
    0,     // Pr6.24: Disturbance observer filter
    0,     // Pr6.25: Reserved parameter
    0,     // Pr6.26: Reserved parameter
    0,     // Pr6.27: Alarm latch time selection
    0,     // Pr6.28: Reserved parameter
    0,     // Pr6.29: Reserved parameter
    0,     // Pr6.30: Reserved parameter
    0,     // Pr6.31: Real-time auto tuning estimation speed
    0,     // Pr6.32: Real-time auto tuning custom setup
    25,    // Pr6.33: Reserved parameter
    0,     // Pr6.34: Reserved parameter
    0,     // Pr6.35: Reserved parameter
    0,     // Pr6.36: Reserved parameter
    0,     // Pr6.37: Oscillation detection level
    0,     // Pr6.38: Alarm mask setup
    0,     // Pr6.39: Reserved parameter
    200,   // Pr7.00: Current loop gain
    260,   // Pr7.01: Current loop integral time
    106,   // Pr7.02: Motor rotor initial position Angle compensa...
    0,     // Pr7.03: Reserved parameter
    360,   // Pr7.04: Motor rated voltage
    4,     // Pr7.05: Motor pole pairs
    82,    // Pr7.06: Motor phase resistor
    84,    // Pr7.07: Motor D/Q inductance
    56,    // Pr7.08: Motor back EMF coefficient
    80,    // Pr7.09: Motor torque coefficient
    3000,  // Pr7.10: Motor rated speed
    4000,  // Pr7.11: Motor Maximum speed
    763,   // Pr7.12: Motor rated current
    40,    // Pr7.13: Motor rotor inertia
    130,   // Pr7.14: Motor power selection
    2,     // Pr7.15: Motor model input
    0,     // Pr7.16: Encoder selection
    250,   // Pr7.17: Motor maximum current
    338,   // Pr7.18: Encoder Index Angle compensation
    0,     // Pr7.19: Reserved parameter
    0,     // Pr7.20: Drive model input
    0,     // Pr7.21: Servo model input
    304,   // Pr7.22: Reserved parameter
    15,    // Pr7.23: Reserved parameter
    0,     // Pr7.24: Fan control mode setting
    50,    // Pr7.25: Fan open temperature setting
    5,     // Pr7.26: Fan temperature control hysteresis
    80,    // Pr7.27: Drive over-temperature alarm threshold se...
    30,    // Pr7.28: Time of Bleeder alarm window
    0,     // Pr7.29: Dc bus voltage detection filter
    16,    // Pr7.30: Under-voltage point set
    0,     // Pr7.31: Bleeder control mode setting
    40,    // Pr7.32: Bleeder open the threshold set
    1,     // Pr7.33: Bleeder control hysteresis
    72,    // Pr7.34: Overvoltage point set
    1,     // Pr7.35: Relay control mode setting
    18,    // Pr7.36: Threshold setting of relay suction
    2048,  // Pr7.37: Analog quantity AI3 hardware zero drift co...
    2048,  // Pr7.38: Analog quantity AI3 hardware zero drift co...
    2048,  // Pr7.39: Analog quantity AI3 hardware zero drift co...
    21,    // Pr7.40: RS232 communication mode settings
    4,     // Pr7.41: RS485 communication baud rate settings
    63,    // Pr7.42: RS232-ID
    0,     // Pr7.43: DC bus voltage hardware zero drift compe...
    0,     // Pr7.44: Temperature measurement hardware zero...
    10000, // Pr7.45: Dc bus voltage hardware slope coefficient
    10000, // Pr7.46: Current U phase sampling hardware slope...
    10000, // Pr7.47: Current V phase sampling hardware slope...
    18,    // Pr7.48: Reserved parameter
    500    // Pr7.49: Reserved parameter
};