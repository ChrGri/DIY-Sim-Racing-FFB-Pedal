#pragma once
#include "isv57_tunedParameters.h"

// "Smooth" parameter set: the registers that differ from the "silent" default in
// isv57_tunedParameters.h. All other registers are the same as there.
//
// The plugin's Servo tab offers both sets on "Reset to recommended". The firmware does
// not use this file: it is parsed by SimHubPlugin/Generated/ServoTunedParameters.tt
// (one register per line: { class, number, value }, e.g. { 1, 6, 800 } is Pr1.06 = 800,
// the same row format as tuned_parameters[] in isv57_tunedParameters.h).
//
// Gain switching (2026-10-23): the 1st gain set (Pr1.00-1.04) is used at standstill and
// while holding, the 2nd set (Pr1.05-1.09) while the pedal moves (Pr1.15 = 10). The
// pedal's roughness is the iSV57's cogging (24 detents per revolution, felt with the motor
// unpowered). A fast velocity loop rejects it but turns the encoder quantization noise
// into an audible whine. The 1st set is quiet, the 2nd fast: detent ripple at 4-12 kHz
// step rate -35 to -42 %, louder while moving. See docs/development/servoLoops.md.
//
// Pr0.00 (MFC bandwidth) is not overridden: the firmware writes it at boot from
// isv57_tunedParameters.h, so a different value here would be reset at every boot.

constexpr Isv57Parameter_t tuned_parameters_smooth_overrides[] = {
    // class, number, value
    { 1,  1,   200 },  // 1st velocity loop gain (20 Hz, quiet at standstill)
    { 1,  5,   600 },  // 2nd position loop gain (as the 1st set)
    { 1,  6,   800 },  // 2nd velocity loop gain (80 Hz while moving)
    { 1,  7,   200 },  // 2nd time constant of velocity loop integration (20 ms)
    { 1,  8,     5 },  // 2nd filter of velocity detection (5 = 1600 Hz low-pass)
    { 1,  9,    10 },  // 2nd torque filter (0.1 ms)
    { 1, 15,    10 },  // Gain switching mode: 10 = 2nd set while position commands arrive
    { 1, 16,  1000 },  // Gain switching delay time (keeps the 2nd set through stick-slip pauses)
    { 1, 17,    20 },  // Gain switching level [rpm] (must be >= Pr1.18)
    { 1, 18,    10 },  // Gain switching hysteresis [rpm]: back to the 1st set below 10 rpm
    { 1, 19,   100 },  // Gain switching time (10 ms blend between the sets)
    { 6, 23,     0 },  // Disturbance torque compensating gain (off, no measurable effect)
    { 6, 24,     0 },  // Disturbance observer filter
};
