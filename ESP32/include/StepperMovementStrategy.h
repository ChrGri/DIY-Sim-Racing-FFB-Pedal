#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"

// Stick/slip friction in the pedal admittance model (holds the pedal still against foot
// tremor). Comment out to compare against the previous behaviour.
#define ADMITTANCE_STICTION_ENABLED

// Task dependent structs and variables
typedef struct {
  float travelRange_mm_fl32;
  float stiffnessAtMaxTravel_Npermm_fl32;
} EndstopBehavior_t;

typedef struct {
  float forceOffset_kg_fl32;
  float forceOffset_Steps_fl32;
} EffectOffsets_t;

typedef enum {
  RUDDER_MODE_DISABLED = 0,
  RUDDER_MODE_PLANE = 1,
  RUDDER_MODE_HELICOPTER = 2,
  RUDDER_MODE_TOE_BRAKE = 3
} RudderMode_e;

typedef struct {
  bool isRudderMode;
  uint8_t rudderMode_u8;       // 0: Disabled, 1: Plane, 2: Helicopter, 3: Airplane with Toe Brake
  uint8_t centeringProfile_u8; // 0: Linear, 1: Progressive, 2: S-Curve
  float centerPosition_01;     // normally 0.50 for center alignment
  float trimOffset_01;         // dynamic trim offset (-0.5 to +0.5)
  float deadzone_01;           // deadzone around center (e.g. 0.02)
  float centerForce_kg;        // centering spring breakout / preload force in kg
} RudderOffsets_t;


typedef struct {
  float physicalPos_m;   
  float virtualVel_mps;
  float virtualAcc_mps2;
} AdmittanceStates_t;

/**
 * @brief Selection of the physical model for elastomer hysteresis simulation.
 */
typedef enum {
    ELASTOMER_MODEL_HUNT_CROSSLEY = 0,       // Position-dependent damping (Standard)
    ELASTOMER_MODEL_STRESS_PROPORTIONAL = 1  // Force-dependent damping (Heusinkveld/Stacked style)
} ElastomerModel_t;
// =========================================================
// DEBUG & TELEMETRY STRUCT
// =========================================================
/**
 * @brief Struct to export internal physics and stability states for debugging and tuning.
 */
typedef struct {
  // Energy Tank & Parameters
  float tankEnergy_J;             // Current energy level in the tank (Joules)
  float massAdaptationOffset_kg;  // How much virtual mass was added by the Energy Tank
  float activeVirtualMass_kg;     // Total active virtual mass currently simulated
  float activeDamping_Ns_m;       // Total active damping currently simulated
  
  // Landi et al. Oscillation Detector
  float admittancePsi_N;          // Current deviation from the physics model (Landi Psi)
  float expectedForce_N;          // Expected model force (based on physical kinematics)
  bool isOscillating;             // Boolean flag if the detector triggered in this frame
  
  // Physical Kinematics (Actual Servo State)
  float physicalPos_m;            // Physical position of the pedal in meters
  float physicalVel_mps;          // Physical velocity of the pedal
  float physicalAcc_mps2;         // Physical acceleration of the pedal
  
  // Virtual Kinematics (Admittance Model State)
  float virtualPos_01;            // Normalized virtual model position
  float virtualVel_mps;           // Virtual model velocity
  float virtualAcc_mps2;          // Virtual model acceleration
} AdmittanceDebugState_t;


// --- Global Admittance & Stability Variables ---
// These variables maintain state across control cycles
float g_vModelPos_01 = 0.0f;    // Normalized virtual position [0.0 to 1.0]
float g_vModelVel_mps = 0.0f;   // Physical virtual velocity [meters/second]
float g_tankEnergy_J = 2.0f;    // Energy tank level for passive parameter adaptation [Joules]
float g_massAdaptationOffset_kg = 0.0f; // Dynamic mass offset from Energy Tank framework
float g_lastActiveDamping_Ns_m = 0.0f;  // Track previous frame's damping for power calculation
float g_lastNetForceTustin_N = 0.0f;    // Net force of the previous integration step (force memory)


// Oscillation Detector State (Landi et al.)
#define TARGET_PSI_WINDOW_US 9000 // Timewindow for oscillation-smootginh: 9 ms
#define PSI_BUFFER_SIZE ((TARGET_PSI_WINDOW_US + (REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64 / 2)) / REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64)
float g_psiBuffer[PSI_BUFFER_SIZE] = {0};
uint8_t g_psiBufferIdx = 0;
float g_prevPhysicalPos_m = 0.0f;
bool g_isPsiInitialized = false;

// NEW: Filter states for physical kinematics to suppress numerical derivation noise
float g_filteredPhysicalVel_mps = 0.0f;
float g_filteredPhysicalAcc_mps2 = 0.0f;
float g_prevFilteredPhysicalVel_mps = 0.0f;

// NEW: Filter states for internal Effect Feedforward (Velocity & Acceleration)
float g_smoothedEffectPos_m = 0.0f;
float g_prevSmoothedEffectPos_m = 0.0f;
float g_smoothedEffectVel_mps = 0.0f;
float g_prevSmoothedEffectVel_mps = 0.0f;
float g_smoothedEffectAcc_mps2 = 0.0f;

// =========================================================
// HELPER SUB-FUNCTIONS FOR ENCAPSULATION
// =========================================================

/**
 * @brief Calculates dynamic travel limits based on effect offsets and local stiffness
 */
static inline IRAM_ATTR_FLAG void CalcDynamicTravelLimits(
    float travelSteps_cnt, float localStiffness_kg_step,
    const EffectOffsets_t& effectOffsets_st,
    float& lowerTravelLimit_01, float& upperTravelLimit_01) 
{
    // High-frequency effects (like ABS) can push the pedal slightly beyond the soft limits.
    // We calculate the required limit expansion based on local stiffness.
    float additionalTravelSteps_StepOffset = effectOffsets_st.forceOffset_Steps_fl32;
    float additionalTravelSteps_ForceOffset = 0.0f;
    if (localStiffness_kg_step > 0.0001f) {
        additionalTravelSteps_ForceOffset = (effectOffsets_st.forceOffset_kg_fl32 / localStiffness_kg_step);
    }

    lowerTravelLimit_01 = 0.0f;
    upperTravelLimit_01 = 1.0f;
    
    if (travelSteps_cnt > 0.0f) {
        float ext_A = additionalTravelSteps_StepOffset / travelSteps_cnt;
        float ext_B = additionalTravelSteps_ForceOffset / travelSteps_cnt;
        float ext_steps = ext_A + ext_B;

        lowerTravelLimit_01 = min(0.0f, ext_steps);
        upperTravelLimit_01 = max(1.0f, 1.0f + ext_steps); // Only allow dynamic boundary expansion, never contract below 1.0
    }
}

/**
 * @brief Simulates a heavy spring pushing back once the travel range is exceeded
 */
static inline IRAM_ATTR_FLAG float CalcSoftEndstopForce(
    float vModelPos_01, float totalTravel_m, const EndstopBehavior_t& endstopBehavior_st,
    float& currentStiffness_N_m, float& upperTravelLimit_01) 
{
    float softEndstopForce_N = 0.0f;
    if (endstopBehavior_st.travelRange_mm_fl32 > 0.01f) {
        float softEndstopTravel_m = endstopBehavior_st.travelRange_mm_fl32 * 0.001f;
        
        if (vModelPos_01 > 1.0f) {
            float softEndstopStiffness_N_m = endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 * 1000.0f;
            float deflection_m = (vModelPos_01 - 1.0f) * totalTravel_m;
            
            float penetration_01 = constrain(deflection_m / max(softEndstopTravel_m, 0.001f), 0.0f, 1.0f);
            softEndstopForce_N = (softEndstopStiffness_N_m * deflection_m) * (0.5f + 0.5f * penetration_01);
            
            // NEUER FIX: Dämpfung weich und quadratisch hochfahren!
            // Das absorbiert die Aufprallenergie bei schnellen Tritten perfekt,
            // ohne den Tustin-Filter durch einen plötzlichen Sprung abstürzen zu lassen.
            currentStiffness_N_m += (softEndstopStiffness_N_m * (penetration_01 * penetration_01));
        }
        
        upperTravelLimit_01 += (softEndstopTravel_m / totalTravel_m);
    }
    return softEndstopForce_N;
}

/**
 * @brief Admittance Oscillation Detector (Landi et al.) with Passivity Theory (Power Flow)
 * Compares theoretical admittance force with actual measured force to identify external disturbances.
 * Uses time-dependent EMA filtering to suppress massive noise spikes from digital stepper derivation.
 * Actively attenuates detection using mechanical power flow to prevent false positives.
 * @param externalForce_N The raw force applied by the user in Newtons.
 * @param actualPosFraction_01 Current position of the stepper motor [0.0 to 1.0].
 * @param totalTravel_m Total physical travel of the pedal in meters.
 * @param totalSpringReaction_N Total force from splines and soft endstops.
 * @param baseDamping_Ns_m Ideal base damping calculated for the current mass.
 * @param currentMass_kg Current active virtual mass of the admittance model.
 * @param dt_s Delta time of the current integration step in seconds.
 * @param maxPedalForce_kg The configured max force from the GUI to scale the dynamic threshold.
 * @param debugState_st Optional pointer to output internal physical states for telemetry.
 * @param hasActiveEffect Flag indicating if vibration/ABS effects are currently active.
 * @return true if an unstable limit cycle (oscillation) is detected, false otherwise.
 */
static inline IRAM_ATTR_FLAG bool DetectAdmittanceOscillation(
    float externalForce_N, float actualPosFraction_01, float totalTravel_m, 
    float totalSpringReaction_N, float baseDamping_Ns_m, float currentMass_kg, float dt_s,
    float maxPedalForce_kg, AdmittanceDebugState_t* debugState_st, bool hasActiveEffect)
{
    float physicalPos_m = actualPosFraction_01 * totalTravel_m;
    
    // Static filter states for the DSP envelope and high-pass filters
    static float s_psi_lowpass = 0.0f;
    static float s_power_envelope_W = 0.0f;

    if (!g_isPsiInitialized) {
        g_prevPhysicalPos_m = physicalPos_m;
        g_filteredPhysicalVel_mps = 0.0f;
        g_filteredPhysicalAcc_mps2 = 0.0f;
        g_prevFilteredPhysicalVel_mps = 0.0f;
        
        s_psi_lowpass = 0.0f;
        s_power_envelope_W = 0.0f;
        
        g_isPsiInitialized = true;
        
        if (debugState_st != nullptr) {
            debugState_st->admittancePsi_N = 0.0f;
            debugState_st->expectedForce_N = 0.0f;
            debugState_st->physicalPos_m = physicalPos_m;
            debugState_st->physicalVel_mps = 0.0f;
            debugState_st->physicalAcc_mps2 = 0.0f;
        }
        return false;
    }

    // Safety check to prevent division by zero or negative time steps
    if (dt_s < 0.0001f) {
        if (debugState_st != nullptr) debugState_st->admittancePsi_N = 0.0f;
        return false;
    }

    // Desired smoothing time constants in seconds (independent of the loop rate!)
    const float TAU_VEL = 0.002f; // 2 ms smoothing for velocity
    const float TAU_ACC = 0.006f; // 6 ms smoothing for acceleration

    // Calculate alpha dynamically based on the exact loop time (dt_s):
    float ALPHA_VEL = 1.0f - expf(-dt_s / TAU_VEL); 
    float ALPHA_ACC = 1.0f - expf(-dt_s / TAU_ACC);

    // 1. Raw Derivation of physical position
    float rawPhysicalVel_mps = (physicalPos_m - g_prevPhysicalPos_m) / dt_s;
    g_prevPhysicalPos_m = physicalPos_m;

    // 2. Low-Pass Filter Velocity (EMA)
    g_filteredPhysicalVel_mps = (ALPHA_VEL * rawPhysicalVel_mps) + ((1.0f - ALPHA_VEL) * g_filteredPhysicalVel_mps);

    // 3. Raw Acceleration from Filtered Velocity
    float rawPhysicalAcc_mps2 = (g_filteredPhysicalVel_mps - g_prevFilteredPhysicalVel_mps) / dt_s;
    g_prevFilteredPhysicalVel_mps = g_filteredPhysicalVel_mps;

    // 4. Low-Pass Filter Acceleration (EMA to suppress extreme stepper derivation noise)
    g_filteredPhysicalAcc_mps2 = (ALPHA_ACC * rawPhysicalAcc_mps2) + ((1.0f - ALPHA_ACC) * g_filteredPhysicalAcc_mps2);

    // Expected force based on nominal admittance model (Eq. 2 from Landi et al.)
    // We use totalSpringReaction_N instead of (Stiffness * Pos) to perfectly account for 
    // non-linear spline biases and soft endstop forces.
    float expectedForce_N = (currentMass_kg * g_filteredPhysicalAcc_mps2) + 
                            (baseDamping_Ns_m * g_filteredPhysicalVel_mps) + 
                            totalSpringReaction_N;
    
    // =========================================================
    // ENDSTOP MASKING (Detector Suppression)
    // =========================================================
    // Suppress detection near the physical limits to avoid false positives from hard stops
    if (actualPosFraction_01 > 0.95f || actualPosFraction_01 < 0.05f) {
        expectedForce_N = externalForce_N;
    }
    
    // 1. Calculate the absolute error (deviation from the ideal admittance model)
    float psi_raw = fabsf(externalForce_N - expectedForce_N);

    // Reset when effects are active, since the rigid admittance model 
    // cannot properly track high-frequency injected vibrations.
    if( hasActiveEffect )
    {
        psi_raw = 0.0f;
    }

    // =========================================================
    // DSP HIGH-PASS FILTER (AC-COUPLING) TO PREVENT FALSE POSITIVES
    // =========================================================
    // We separate slow deviations (normal human braking, < 3Hz) 
    // from fast deviations (high-frequency limit cycles/judder).
    
    // 50ms time constant (~3 Hz Cutoff). Everything slower ends up in the low-pass.
    float alpha_hp = 1.0f - expf(-dt_s / 0.05f); 
    s_psi_lowpass = (alpha_hp * psi_raw) + ((1.0f - alpha_hp) * s_psi_lowpass);
    
    // The high-pass is simply the original signal minus the slow components
    float psi_high_freq = fabsf(psi_raw - s_psi_lowpass);


    // =========================================================
    // ENVELOPE FOLLOWER FOR PASSIVITY THEORY (POWER FLOW)
    // =========================================================
    // Calculate mechanical power P = F * v. 
    // If the pedal pushes the foot back (F > 0, v < 0), power flows into the user.
    float mechanical_power_W = externalForce_N * g_filteredPhysicalVel_mps;
    float power_to_user_W = (mechanical_power_W < 0.0f) ? -mechanical_power_W : 0.0f;

    // Envelope logic: 
    // - If power increases, we react IMMEDIATELY (Instant Attack = 0)
    // - If power drops (zero-crossing), the curve decays very slowly (Release)
    if (power_to_user_W > s_power_envelope_W) {
        s_power_envelope_W = power_to_user_W; // Instant Attack
    } else {
        // 100ms Release time constant: Smooths over the 0-Watt pulsing of the oscillation perfectly
        float release_alpha = 1.0f - expf(-dt_s / 0.100f); 
        s_power_envelope_W = s_power_envelope_W - (s_power_envelope_W * release_alpha); 
    }

    // Convert the envelope into a continuous weight [0.0 to 1.0].
    // If reverse power reaches ~1.5W, we open the gate completely (1.0).
    // Base-Weight is 0.0. The detector is completely silent during stable operation.
    float power_weight = constrain(s_power_envelope_W / 1.5f, 0.0f, 1.0f);
    
    // Attenuate the high-frequency Psi deviation based on the power flow envelope
    float psi_final = psi_high_freq * power_weight;

    // Output the internal physical values for telemetry
    if (debugState_st != nullptr) {
        debugState_st->admittancePsi_N = psi_final;
        debugState_st->expectedForce_N = expectedForce_N;
        debugState_st->physicalPos_m = physicalPos_m;
        debugState_st->physicalVel_mps = g_filteredPhysicalVel_mps;
        debugState_st->physicalAcc_mps2 = g_filteredPhysicalAcc_mps2;
    }

    // =========================================================
    // DYNAMIC THRESHOLD TUNING
    // =========================================================
    // Since we eliminated false positives (via High-Pass) and zero-crossing pulses 
    // (via Envelope), the psi_final signal is extremely clean.
    // We can use a static, much lower threshold (e.g., 25 Newtons).
    const float EPSILON_THRESHOLD_N = 25.0f; 
    
    return (psi_final > EPSILON_THRESHOLD_N);
}

/**
 * @brief Position-gated Parameter Adaptation
 * Increases virtual mass during oscillations and only releases it when the pedal is near an endstop.
 */
static inline IRAM_ATTR_FLAG void AdaptVirtualMass(
    bool isOscillating, float dt_s, 
    float baseMass_kg, float& virtualMass_kg, bool hasActiveEffect, float actualPosFraction_01)
{
    // Freeze adaptation if an effect is active
    if (hasActiveEffect) {
        virtualMass_kg = baseMass_kg + g_massAdaptationOffset_kg;
        return;
    }

    const float MAX_ADAPTATION_OFFSET_KG = 0.6f; // Max +0.6kg dynamic mass adaptation (prevents sluggish lockup)
    const float M_INCREASE_RATE_KG_S = 4.0f;     // Gradual mass increase during genuine limit cycles
    const float M_ENDSTOP_DECREASE_RATE_KG_S = 4.0f; // Fast recovery near endstops
    const float M_LEAKY_DECREASE_RATE_KG_S = 1.0f;   // Continuous recovery anywhere in travel

    // 1. Ramp up mass during oscillation
    if (isOscillating) {
        float intensity = 1.0f;
        g_massAdaptationOffset_kg += M_INCREASE_RATE_KG_S * intensity * dt_s;
    } 
    // 2. Continuous decay when stable: faster near endstops, gentle everywhere else
    else {
        float decayRate = (actualPosFraction_01 < 0.05f || actualPosFraction_01 > 0.95f) 
                          ? M_ENDSTOP_DECREASE_RATE_KG_S 
                          : M_LEAKY_DECREASE_RATE_KG_S;
        g_massAdaptationOffset_kg -= decayRate * dt_s;
    }

    // Clamp values safely
    g_massAdaptationOffset_kg = constrain(g_massAdaptationOffset_kg, 0.0f, MAX_ADAPTATION_OFFSET_KG);
    virtualMass_kg = baseMass_kg + g_massAdaptationOffset_kg;
}

// Contact oscillation detector state (see UpdateContactOscillationDamping)
#define CONTACT_OSC_CROSSING_BUFFER_SIZE 8
float g_contactOscHighPass_N = 0.0f;
float g_contactOscBandPass_N = 0.0f;
float g_contactOscPrevForce_N = 0.0f;
float g_contactOscPeak_N = 0.0f;
float g_contactOscLastHalfWavePeak_N = 0.0f;
int8_t g_contactOscSign_i8 = 0;
// integer microseconds: a float seconds clock stops advancing after a few hours (600 us steps
// round to zero above 16384 s), which froze the crossing ages and latched the detection
int64_t g_contactOscCrossingTimes_us[CONTACT_OSC_CROSSING_BUFFER_SIZE] = {0};
uint8_t g_contactOscCrossingIdx_u8 = 0;
int64_t g_contactOscTime_us = 0;
float g_contactOscForceLowPass_N = 0.0f;
float g_contactOscForceAtDetection_N = 0.0f;
float g_contactOscDampingMultiplier = 1.0f;

/**
 * @brief Contact oscillation detector with adaptive damping.
 *
 * A stiff contact (e.g. holding the pedal with the heel, ~10 N/mm) closes a loop
 * force -> admittance model -> position -> contact -> force that oscillates at ~13-16 Hz
 * with the servo and force-filter delays. More damping stabilizes it (simulation: ~3x the
 * base damping), more virtual mass does not.
 *
 * Detection: band-pass (5-40 Hz) of the pilot force; oscillation = at least
 * CONTACT_OSC_MIN_CROSSINGS sign changes with a half-wave peak above the threshold
 * (4 % of the force, at least 1 N) within CONTACT_OSC_WINDOW_S (~2.5 periods). Presses and
 * single stabs produce only one or two such crossings and do not trigger.
 * Reaction: damping multiplier rises to CONTACT_OSC_DAMPING_MAX within ~50 ms. It decays
 * slowly (10 s) while the contact persists, so the oscillation does not grow back, and quickly
 * (0.3 s) once the pedal moves or the force drops below half the detection level (released).
 *
 * @return damping multiplier (>= 1)
 */
static inline IRAM_ATTR_FLAG float UpdateContactOscillationDamping(
    float pilotForce_N, float vModelVel_mps, float dt_s, bool hasActiveEffect,
    bool& isOscillating, float& bandPeak_N)
{
    const float CONTACT_OSC_HIGH_PASS_HZ = 5.0f;
    const float CONTACT_OSC_LOW_PASS_HZ = 40.0f;
    // Amplitude threshold relative to the force: leg tremor scales with the force (~1 % measured
    // while holding), a contact oscillation reached ~20 % (throttle, heel). With an absolute
    // threshold a brake at high force would trigger on tremor alone.
    const float CONTACT_OSC_AMPLITUDE_MIN_N = 1.0f;
    const float CONTACT_OSC_AMPLITUDE_RELATIVE = 0.04f;
    // a contact oscillation needs contact: no detection below this mean force (load cell noise
    // at rest, e.g. with a high-rated brake load cell)
    const float CONTACT_OSC_MIN_FORCE_N = 3.0f;
    const float CONTACT_OSC_WINDOW_S = 0.3f;
    const uint8_t CONTACT_OSC_MIN_CROSSINGS = 5;
    const float CONTACT_OSC_DAMPING_MAX = 3.0f;
    const float CONTACT_OSC_RISE_TIME_S = 0.03f;
    const float CONTACT_OSC_DECAY_HOLD_S = 10.0f;
    const float CONTACT_OSC_DECAY_RELEASED_S = 0.3f;
    const float CONTACT_OSC_MOVING_VELOCITY_MPS = 0.005f;
    const float CONTACT_OSC_RELEASE_FORCE_RATIO = 0.5f;

    g_contactOscTime_us += (int64_t)(dt_s * 1e6f + 0.5f);

    // band-pass: first order high-pass followed by first order low-pass
    float highPassTau_s = 1.0f / (2.0f * PI * CONTACT_OSC_HIGH_PASS_HZ);
    float highPassAlpha = highPassTau_s / (highPassTau_s + dt_s);
    g_contactOscHighPass_N = highPassAlpha * (g_contactOscHighPass_N + pilotForce_N - g_contactOscPrevForce_N);
    g_contactOscPrevForce_N = pilotForce_N;
    g_contactOscBandPass_N += (1.0f - expf(-dt_s * 2.0f * PI * CONTACT_OSC_LOW_PASS_HZ)) * (g_contactOscHighPass_N - g_contactOscBandPass_N);

    // mean contact force: relative amplitude threshold, and release detection below
    g_contactOscForceLowPass_N += (1.0f - expf(-dt_s / 0.05f)) * (pilotForce_N - g_contactOscForceLowPass_N);
    float amplitudeThreshold_N = max(CONTACT_OSC_AMPLITUDE_MIN_N,
                                     CONTACT_OSC_AMPLITUDE_RELATIVE * g_contactOscForceLowPass_N);

    // sign changes with sufficient half-wave amplitude
    g_contactOscPeak_N = max(g_contactOscPeak_N, fabsf(g_contactOscBandPass_N));
    int8_t sign_i8 = (g_contactOscBandPass_N > 0.0f) ? 1 : -1;
    if (sign_i8 != g_contactOscSign_i8) {
        if ((g_contactOscPeak_N >= amplitudeThreshold_N) &&
            (g_contactOscForceLowPass_N >= CONTACT_OSC_MIN_FORCE_N) && !hasActiveEffect) {
            g_contactOscCrossingTimes_us[g_contactOscCrossingIdx_u8] = g_contactOscTime_us;
            g_contactOscCrossingIdx_u8 = (g_contactOscCrossingIdx_u8 + 1) % CONTACT_OSC_CROSSING_BUFFER_SIZE;
        }
        g_contactOscLastHalfWavePeak_N = g_contactOscPeak_N;
        g_contactOscPeak_N = 0.0f;
        g_contactOscSign_i8 = sign_i8;
    }
    uint8_t recentCrossings_u8 = 0;
    const int64_t windowLength_us = (int64_t)(CONTACT_OSC_WINDOW_S * 1e6f);
    for (uint8_t i = 0; i < CONTACT_OSC_CROSSING_BUFFER_SIZE; i++) {
        int64_t age_us = g_contactOscTime_us - g_contactOscCrossingTimes_us[i];
        if ((g_contactOscCrossingTimes_us[i] > 0) && (age_us <= windowLength_us)) {
            recentCrossings_u8++;
        }
    }
    isOscillating = (recentCrossings_u8 >= CONTACT_OSC_MIN_CROSSINGS) && !hasActiveEffect;
    bandPeak_N = g_contactOscLastHalfWavePeak_N;

    // contact force level: remember it at detection, to recognize a release
    if (isOscillating) {
        g_contactOscForceAtDetection_N = (g_contactOscDampingMultiplier > 1.01f)
            ? max(g_contactOscForceAtDetection_N, g_contactOscForceLowPass_N)
            : g_contactOscForceLowPass_N;
    }

    // damping multiplier: fast rise, slow decay while the contact persists
    if (isOscillating) {
        g_contactOscDampingMultiplier += (1.0f - expf(-dt_s / CONTACT_OSC_RISE_TIME_S))
                                         * (CONTACT_OSC_DAMPING_MAX - g_contactOscDampingMultiplier);
    } else {
        bool released_b = (fabsf(vModelVel_mps) > CONTACT_OSC_MOVING_VELOCITY_MPS) ||
                          (g_contactOscForceLowPass_N < CONTACT_OSC_RELEASE_FORCE_RATIO * g_contactOscForceAtDetection_N);
        float decay_s = released_b ? CONTACT_OSC_DECAY_RELEASED_S : CONTACT_OSC_DECAY_HOLD_S;
        g_contactOscDampingMultiplier += (1.0f - expf(-dt_s / decay_s)) * (1.0f - g_contactOscDampingMultiplier);
    }
    return g_contactOscDampingMultiplier;
}

/**
 * @brief Calculates active damping including AOM Boost, Trajectory Shaping, and Elastomer Hysteresis.
 */
static inline IRAM_ATTR_FLAG float CalcActiveDamping(
    float dampingRatio_zeta, float virtualMass_kg, float currentStiffness_N_m,
    float vModelPos_01, float actualPosFraction_01,
    int32_t actualServoTrackingError_i32, float travelSteps_cnt, bool hasActiveEffect,
    uint8_t dampingProgression_u8, float springForce_N, float vModelVel_mps, uint8_t elastomerModelSelection, float maxPedalForce_kg,
    float dt_s, float totalTravel_m)
{
    // Calculate Base Damping based on mass and current stiffness: c_c = 2 * sqrt(m * k)
    float criticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
    float baseDamping_Ns_m = dampingRatio_zeta * criticalDamping_Ns_m;
    
    float dampingMultiplier = 1.0f;

    // Adaptive damping (AOM & Tracking Error) only when no effect is applied
    if (!hasActiveEffect)
    {

        // =========================================================
        // Tracking-Error dependent damping (Trajectory Shaping) to reduce EMF spikes. 
        // It was observed that voltage spikes occur at tracking error zero crossings. 
        // It was assumed that the servo could not keep up with its target position, then overshoots, 
        // producing large EMF. To reduce the lag, we increase the virtual damping so the servo can keep up.
        // =========================================================
        // actualPosFraction_01 is the physical position, vModelPos_01 is the target model position.
        // Signed: pressing (v > 0) gives a negative servo tracking error.
        float trackingError_01 = vModelPos_01 - actualPosFraction_01;
        if (travelSteps_cnt > 0.0001f) {
            trackingError_01 = (float)actualServoTrackingError_i32 / travelSteps_cnt;
        }

        // The servo lags its target by a speed-proportional amount even when it tracks perfectly
        // well (following error ~ (1 - velocity feed-forward) * v / position gain; ~14 ms measured).
        // Only the part beyond that expected lag indicates that the servo cannot keep up.
        // Expected lag estimated online (least squares with forgetting), adapts to servo retuning.
        static float s_sumErrorVel_fl32 = 0.0f;
        static float s_sumVelSq_fl32 = 0.0f;
        static float s_servoLag_s_fl32 = 0.0f;
        // counts only while moving: 0.3 s of motion is a few presses, so the estimate follows a
        // live servo retune quickly (2 s needed many presses, see trace 20260929_061357)
        const float LAG_ESTIMATION_TIME_CONSTANT_S = 0.3f;
        const float LAG_ESTIMATION_MIN_VELOCITY_MPS = 0.03f;
        const float LAG_MAX_S = 0.04f;
        float vModelVel_01ps = (totalTravel_m > 0.0001f) ? (vModelVel_mps / totalTravel_m) : 0.0f;
        if (fabsf(vModelVel_mps) > LAG_ESTIMATION_MIN_VELOCITY_MPS) {
            float forgetting_fl32 = expf(-dt_s / LAG_ESTIMATION_TIME_CONSTANT_S);
            s_sumErrorVel_fl32 = forgetting_fl32 * s_sumErrorVel_fl32 - trackingError_01 * vModelVel_01ps;
            s_sumVelSq_fl32 = forgetting_fl32 * s_sumVelSq_fl32 + vModelVel_01ps * vModelVel_01ps;
            if (s_sumVelSq_fl32 > 1e-6f) {
                s_servoLag_s_fl32 = constrain(s_sumErrorVel_fl32 / s_sumVelSq_fl32, 0.0f, LAG_MAX_S);
            }
        }
        float unexpectedTrackingError_01 = fabsf(trackingError_01 + s_servoLag_s_fl32 * vModelVel_01ps);

        // Low-pass filter the tracking error to eliminate 100Hz Modbus step discontinuities
        static float s_smoothedTrackingError_01 = 0.0f;
        const float TAU_TRACKING_ERR = 0.015f; // 15ms smoothing
        float alpha_err = 1.0f - expf(-dt_s / TAU_TRACKING_ERR);
        s_smoothedTrackingError_01 = (alpha_err * unexpectedTrackingError_01) + ((1.0f - alpha_err) * s_smoothedTrackingError_01);

        // If the smoothed unexpected tracking error exceeds 0.5% (~0.5-1mm), the model damping is
        // increased proportionally so that the servo can catch up without oscillating.
        // Faded in between 20 and 40 mm/s instead of switched, so the damping does not jump.
        float velocityWeight_01 = constrain((fabsf(vModelVel_mps) - 0.02f) / 0.02f, 0.0f, 1.0f);
        if (s_smoothedTrackingError_01 > 0.005f) {
            float excessError = s_smoothedTrackingError_01 - 0.005f;
            dampingMultiplier += velocityWeight_01 * constrain(excessError * 35.0f, 0.0f, 2.5f);
        }
    }
    

    float totalDamping_Ns_m = baseDamping_Ns_m * dampingMultiplier;

    // =========================================================
    // ELASTOMER HYSTERESIS MODELS (Selectable via Switch)
    // =========================================================
    // Normalize the GUI parameter from [0, 100] to [0.0, 1.0].
    // Squared ratio provides finer control at the lower end (steel spring -> soft rubber).
    float progressionRatio = (float)constrain( dampingProgression_u8, 0, 100 ) / 100.0f;
    float progression_01 = progressionRatio * progressionRatio; 
    
    float elastomerDamping_Ns_m = 0.0f;

    switch(elastomerModelSelection) 
    {
        case ELASTOMER_MODEL_STRESS_PROPORTIONAL: 
        {
            // ADVANCED STACKED ELASTOMER HYSTERESIS (Heusinkveld-style)
            // Instead of using pure displacement (x), we use the actual spring force (Stress)
            // to scale the friction. This perfectly mimics a multi-stage elastomer stack
            // separated by metal washers.
            
            // =========================================================
            // DYNAMIC SCALING BASED ON MAXIMUM FORCE
            // =========================================================
            // We define "Heavy Load" as 50% of the maximum configured pedal force.
            // For a 50kg pedal, the reference is 25kg (~245N).
            float referenceLoad_N = (maxPedalForce_kg * 9.81f) * 0.5f;
            
            // Fallback in case maxForce in the GUI is extremely small or 0
            if (referenceLoad_N < 10.0f) referenceLoad_N = 100.0f; 

            // Now scales perfectly with the user's specific force curve and max force!
            float forceLoad_01 = constrain(springForce_N / referenceLoad_N, 0.0f, 2.0f);

            // REBOUND ASYMMETRY: Real elastomers return faster than they compress.
            // We reduce the hysteresis during release (v < 0) for a snappier feel.
            // FIXED CODE: Smooth Rebound Transition
            // Blend over a small velocity window (-20 mm/s to +20 mm/s)
            // When v is very negative, blend goes to 0. When v is positive, blend goes to 1.
            float blend_01 = constrain((vModelVel_mps + 0.02f) / 0.04f, 0.0f, 1.0f);
            float reboundFactor = 0.7f + (0.3f * blend_01);

            const float MAX_ELASTOMER_MULTIPLIER = 1.5f; 
            float ELASTOMER_VISCOSITY_COEFFICIENT = progression_01 * (MAX_ELASTOMER_MULTIPLIER * criticalDamping_Ns_m);

            // Resulting Elastomer Damping: Damping follows the force curve's shape!
            elastomerDamping_Ns_m = ELASTOMER_VISCOSITY_COEFFICIENT * forceLoad_01 * reboundFactor;
            break;
        }

        case ELASTOMER_MODEL_HUNT_CROSSLEY: 
        default:
        {
            // ELASTOMER HYSTERESIS (Standard Hunt-Crossley Model)
            // Max Elastomer Multiplier defines how many times the critical damping 
            // is added at 100% slider value and full pedal compression (displacement = 1.0).
            const float MAX_ELASTOMER_MULTIPLIER = 1.5f; 
            float ELASTOMER_VISCOSITY_COEFFICIENT = progression_01 * (MAX_ELASTOMER_MULTIPLIER * criticalDamping_Ns_m);

            // Calculate equivalent elastomer damping based on linear displacement: C_eq = C_elastomer * x
            float displacement_01 = constrain(vModelPos_01, 0.0f, 1.0f);
            elastomerDamping_Ns_m = ELASTOMER_VISCOSITY_COEFFICIENT * displacement_01;
            break;
        }
    }

    // Add Elastomer Hysteresis to the global system damping
    return totalDamping_Ns_m + elastomerDamping_Ns_m;
}

/**
 * @brief Regenerative Velocity and Power Governor
 * 
 * =========================================================================================
 * PHYSICAL BACKGROUND & MATHEMATICAL RATIONALE:
 * =========================================================================================
 * 1. ENERGY CONSERVATION (Mechanical-to-Electrical Regeneration):
 *    When a driver presses down hard on the pedal against the resisting force (spring, 
 *    viscoelastic elastomer damping, and soft endstop), the pedal moves forward (v > 0)
 *    while the actuator applies an opposing braking force (F_oppose > 0).
 *    In this quadrant, the servo acts as a generator:
 *        P_mech = F_oppose * v_pedal = tau_motor * omega_motor
 *    Neglecting winding copper losses (I^2 * R), this mechanical work is converted directly 
 *    into electrical current fed back through the inverter bridge diodes into the DC power bus:
 *        P_elec ≈ P_mech
 * 
 * 2. WHY 40 WATTS (DC BUS CAPACITANCE & INTERNAL BLEEDER CIRCUIT):
 *    - Capacitive Absorption Limit:
 *      The internal DC bus capacitor bank inside the Leadshine iSV57 is C ≈ 470 - 1000 uF (63V rating).
 *      The energy absorbed during a voltage rise from nominal supply (e.g. 36V) to the 
 *      internal bleeder trigger threshold (Pr7.32 = 40V) is:
 *          ΔE_C = 0.5 * C * (V_trip^2 - V_nom^2) = 0.5 * 1000uF * (40^2 - 36^2) ≈ 0.15 Joules.
 *      At a sudden foot stomp delivering 150 W (e.g. 300 N at 0.5 m/s), this capacitor buffer 
 *      fills and saturates in less than 1 millisecond!
 * 
 *    - Internal Bleeder (Braking Resistor) Dissipation:
 *      The iSV57 integrated servo features an internal dynamic braking switch (Pr7.31 = 1) 
 *      that dumps excess voltage into an internal resistor when V_bus >= 40V (or 52V in 48V mode).
 *      Due to the compact 57mm frame size and lack of active cooling, the internal bleeder 
 *      can continuously dissipate ~10-15W, with a safe transient pulse capacity of ~35W - 40W.
 *      Exceeding ~35-40W during high-speed stomps causes the bus voltage to climb uncontrollably, 
 *      tripping the overvoltage protection alarm (Er.020 / Er.021 at ~60V) or shutting down 
 *      external SMPS (Switch-Mode Power Supplies) lacking reverse-current sinks.
 * 
 *    - Velocity Governor:
 *      By setting P_max_regen = 40.0 W (MAX_REGEN_POWER_W), we dynamically enforce:
 *          v_max_power = P_max_regen / F_oppose
 *      - At low pedal resistance (e.g. 25 N): v_max = 40 / 25 = 1.6 m/s (no restriction, completely transparent).
 *      - At heavy braking (e.g. 250 N): v_max = 40 / 250 = 0.16 m/s (160 mm/s, halts overvoltage spikes).
 *      - At extreme endstop hit (e.g. 500 N): v_max = 40 / 500 = 0.08 m/s (80 mm/s, smooth deceleration).
 * 
 * 3. BACK-EMF & MOTOR RPM LIMIT (Pr7.08 = 56):
 *    - Pr7.08 is the Back-EMF constant: 5.6 V_rms / 1000 rpm (line-to-line).
 *    - Peak AC voltage is 5.6 * sqrt(2) ≈ 7.9 V / 1000 rpm.
 *    - Rectified DC voltage through the body diodes is V_rectified ≈ 1.35 * 5.6 V ≈ 7.56 V / 1000 rpm.
 *    - The safe RPM ceiling is derived from the actual configured max step rate
 *      (MAXIMUM_STEPPER_SPEED_U32 / stepsPerMotorRevolution_u32) rather than a fixed
 *      number, so it always matches what the stepper can actually be commanded to do.
 *      E.g. at 3200 steps/rev and a 250000 Hz step-rate ceiling, that works out to
 *      ~4687 rpm, whose rectified BEMF (~35.4V) still stays below the 36V/48V bus rails.
 * =========================================================================================
 */
static inline IRAM_ATTR_FLAG float CalcRegenVelocityLimit(
    float totalOpposingForce_N,
    float totalTravel_m,
    float maxSledPos_m,
    float spindlePitch_mm,
    float vModelPos_01,
    float softEndstopTravel_m,
    uint32_t stepsPerMotorRevolution_u32)
{
    // 1. SAFE REGENERATIVE POWER LIMIT
    // P_regen = F_oppose * v_pedal. Safe dissipation limit before DC bus overvoltage.
    // iSV57 internal bleeder + capacitance handles up to ~35-40W continuous/burst safely.
    const float MAX_REGEN_POWER_W = 40.0f;
    float safeOpposingForce_N = max(totalOpposingForce_N, 1.0f);
    float vMaxPower_mps = MAX_REGEN_POWER_W / safeOpposingForce_N;

    // 2. BACK-EMF VOLTAGE / MOTOR RPM LIMIT
    // Pr7.08 = 56 (5.6 V_rms/krpm). Derive the safe RPM ceiling from the same
    // step-rate limit the rest of the motion pipeline actually uses
    // (MAXIMUM_STEPPER_SPEED_U32, see maxPhysicalSledVel_mps above), instead of a
    // separate hardcoded number that can silently drift out of sync with it.
    const float MAX_SAFE_MOTOR_RPM =
        (stepsPerMotorRevolution_u32 > 0)
            ? (float)MAXIMUM_STEPPER_SPEED_U32 * 60.0f /
                  (float)stepsPerMotorRevolution_u32
            : 3600.0f; // conservative fallback if steps/rev isn't configured yet
    float safePitch_mm = (spindlePitch_mm > 0.0f) ? spindlePitch_mm : 5.0f;
    float vSledMaxBemf_mps = (MAX_SAFE_MOTOR_RPM * safePitch_mm) / 60000.0f; // mm/min to m/s
    float vPedalMaxBemf_mps = vSledMaxBemf_mps * (totalTravel_m / max(maxSledPos_m, 0.0001f));

    // Combine power limit and BEMF limit
    float maxRegenVel_mps = min(vMaxPower_mps, vPedalMaxBemf_mps);

    // 3. SOFT ENDSTOP CUSHIONING / TAPER
    // When pressing into the soft endstop (> 1.0), progressively taper down allowable forward speed
    // proportionally to penetration depth, smoothly arresting motion without abrupt power dumping.
    if (vModelPos_01 > 1.0f) {
        float penetration_m = (vModelPos_01 - 1.0f) * totalTravel_m;
        float safeEndstopTravel_m = max(softEndstopTravel_m, 0.001f);
        float penetration_01 = constrain(penetration_m / safeEndstopTravel_m, 0.0f, 1.0f);
        // Taper down to 15% of max velocity at full endstop penetration
        float taper = 1.0f - (0.85f * penetration_01);
        maxRegenVel_mps *= taper;
    }

    return maxRegenVel_mps;
}

// =========================================================
// MAIN CONTROL STRATEGY
// =========================================================

/**
 * @brief Executes the Admittance Control strategy for the active pedal with AOM and Soft Endstops.
 * * * Admittance control is a branch of robotics and haptics where the system measures 
 * an external force (user's foot) and outputs a position or velocity based on a 
 * simulated virtual environment. This function simulates a 1-Degree-of-Freedom (1-DOF) 
 * Mass-Spring-Damper system.
 * * * The control loop works by calculating the net force on a "Virtual Mass":
 * F_net = F_human - F_spring - F_softEndstop - F_damping
 * * * It then integrates acceleration (F_net / Mass) to find virtual velocity, and 
 * integrates velocity to find the new virtual position. The physical stepper motor 
 * is then commanded to track this virtual position.
 * * * @note CRUCIAL ARCHITECTURAL WARNING regarding `g_vModelPos_01`:
 * DO NOT overwrite the virtual model position (`g_vModelPos_01`) with the physical 
 * motor's current position (`stepper->getCurrentPositionFraction()`) at the start 
 * of the loop. 
 * In admittance control, the virtual model is the "Ground Truth" of the physics 
 * engine. The physical motor is merely a slave trying to follow this model. 
 * If you overwrite the virtual position with the physical position:
 * 1. You destroy the second-order physics (Mass and Damping). The system degrades 
 * into a jittery first-order system because the velocity integrator loses its 
 * historical state.
 * 2. You introduce a "Unity-Gain Loopback". The physics engine will calculate a 
 * reaction based on a delayed physical state, command a new physical state, 
 * and then snap back to the delayed state on the next frame. This destroys 
 * phase margin and causes violent, high-frequency oscillations (hunting).
 * * Instead, we use a "Soft Leash" (divergence correction) at the end of the loop 
 * to gently pull the virtual model toward reality, correcting floating-point drift 
 * without corrupting the inertia and damping integrators.
 * * * @note INERTIAL COUPLING & PARASITIC MASS:
 * Because the load cell is physically sandwiched between the motor and the pedal 
 * faceplate, it measures the force required to accelerate the physical faceplate 
 * in addition to the user's input (F_measured = F_human + m_physical * a).
 * When solving the admittance equation with this measured force, the physical mass 
 * of the faceplate mathematically adds itself to the virtual mass parameter: 
 * m_effective = m_virtual + m_physical.
 * Therefore, the absolute lowest effective mass the user can feel is the physical 
 * weight of the faceplate itself (e.g., 500g). Setting `virtualMass_kg` lower than 
 * this simply means the perceived mass approaches the physical mass.
 * * * @see "Impedance and Admittance Control" - Siciliano & Khatib, Springer Handbook of Robotics.
 * @see "Virtual Model Control" - Pratt et al.
 * @see "Control System Design: An Introduction to State-Space Methods" - Friedland.
 * * @param loadCellReadingKg_fl32 Raw force applied by the user in Kg.
 * @param stepper Pointer to the stepper motor control interface.
 * @param forceCurve Pointer to the interpolated force/travel curve.
 * @param calc_st Pointer to the pedal's static calculation variables.
 * @param config_st Pointer to the pedal's configuration structure.
 * @param effectOffsets_st High-frequency offsets (ABS vibrations, etc.).
 * @param endstopBehavior_st Configuration for the soft endstop feel.
 * @param debugState_st Optional pointer to a struct to output internal variables for debugging. Default is nullptr.
 * @param admittanceStates_pst Optional pointer to state tracking struct.
 * @return float The next absolute target position in steps for the stepper motor driver.
 */
float IRAM_ATTR_FLAG MoveByAdmittanceStrategy(
  float loadCellReadingKg_fl32, 
  StepperWithLimits* stepper, 
  ForceCurveInterpolated* forceCurve, 
  DapCalculationVariables_t* calc_st, 
  DapConfig_t* config_st, 
  EffectOffsets_t effectOffsets_st, 
  EndstopBehavior_t endstopBehavior_st, 
  AdmittanceDebugState_t* debugState_st = nullptr,
  AdmittanceStates_t *admittanceStates_pst = nullptr,
  float holdProbability_01 = 0.0f,
  float cycleTime_s = 0.0f)
{
  // --- 1. PHYSICAL PARAMETERS & CONFIGURATION ---
  // Time step for integration (seconds): the measured cycle time when given (already clamped by
  // the caller), so the model advances in real time. With a fixed step, every late cycle slowed the
  // setpoint in real time and the servo moved start-stop ("grain"). The mass-damper integrator is stable
  // for any step length; otherwise fall back to the nominal interval.
  float dt_s = (cycleTime_s > 0.0f) ? cycleTime_s
                                    : ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
  const float GRAVITY_N_KG = 9.81f; // Conversion constant for Kg to Newtons

  // Convert virtual mass and damping from user configuration percentages
  float virtualMass_kg = ((float)config_st->payloadPedalConfig_st.virtualPedalMassInPercent_u8) * 0.01f;
  float dampingRatio_zeta = ((float)config_st->payloadPedalConfig_st.virtualPedalDampingInPercent_u8) * 0.01f;
  
  // Constrain parameters to safe operational ranges
  virtualMass_kg = constrain(virtualMass_kg, 0.2f, 5.0f);
  dampingRatio_zeta = constrain(dampingRatio_zeta, 0.5f, 5.0f); 
  const float baseMass_kg = virtualMass_kg; 

  // =========================================================
  // --- 2. PHYSICAL GEOMETRY (FORWARD KINEMATICS & TASK SPACE) ---
  // =========================================================
  // We transform the Actuator Space (Linear Sled Position) into Task Space 
  // (Rotational Pedal Arc Length). This allows the entire Admittance Model 
  // to run natively on the pedal face plate, eliminating non-linear instability.
  
  // travelSteps_cnt: total steps from min to max soft endstop
  float travelSteps_cnt = (float)(calc_st->softEndstopMaxStepperPos_i32 - calc_st->softEndstopMinStepperPos_i32);
  
  // Calculate spindle pitch and steps per revolution parameters
  float motorRevolutionsPerSteps_lcl_fl32 = 1.0f / (float)calc_st->stepsPerMotorRevolution_u32;
  float pitch_mm = (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
  
  // 1. Calculate physical sled boundaries in mm (relative to soft endstop min)
  float minSledPos_mm = 0.0f;
  float maxSledPos_mm = travelSteps_cnt * motorRevolutionsPerSteps_lcl_fl32 * pitch_mm;
  
  // actualSledPos_mm: The current physical position of the ESP stepper in mm
  float actualSledPosFraction_01 = stepper->getCurrentPositionFraction();
  float actualSledPos_mm = actualSledPosFraction_01 * maxSledPos_mm;

  // 2. Forward Kinematics: Angles at the boundaries and current physical state
  // travel-end angles only change with the configuration: cached, recomputed exactly whenever
  // the sled travel or a pedal length changes (identical values to a direct call)
  static PedalAngleCache_t s_angleAtMinSledCache_st;
  static PedalAngleCache_t s_angleAtMaxSledCache_st;
  float angleAtMinSled_deg = pedalInclineAngleDegCached(minSledPos_mm, config_st, s_angleAtMinSledCache_st);
  float angleAtMaxSled_deg = pedalInclineAngleDegCached(maxSledPos_mm, config_st, s_angleAtMaxSledCache_st);
  float currentAngle_deg = pedalInclineAngleDeg(actualSledPos_mm, config_st);

  // 3. Convert Angles to Arc Length (Task Space in meters)
  // Lever arm is lower pedal plate (B) + upper pedal plate (D)
  float leverArm_m = ((float)config_st->payloadPedalConfig_st.lengthPedalB_i16 + (float)config_st->payloadPedalConfig_st.lengthPedalD_i16) * 0.001f;
  
  // totalTravel_m: The actual length of the curve the foot travels (replaces linear sled travel)
  float totalTravel_m = fabsf(angleAtMaxSled_deg - angleAtMinSled_deg) * DEG_TO_RAD_FL32 * leverArm_m;
  
  // Normalized physical position [0.0, 1.0] based natively on the pedal angle
  float actualPosFraction_01 = 0.0f;
  if (fabsf(angleAtMaxSled_deg - angleAtMinSled_deg) > 0.001f) {
      actualPosFraction_01 = (currentAngle_deg - angleAtMinSled_deg) / (angleAtMaxSled_deg - angleAtMinSled_deg);
  }
  // unclamped copy for the leash: in the soft endstop and with effect offsets the sled is
  // legitimately outside [0, 1]
  const float actualPosFractionRaw_01 = actualPosFraction_01;
  actualPosFraction_01 = constrain(actualPosFraction_01, 0.0f, 1.0f);

  // Re-anchor the model when the travel changes (pedal start/end position, endstops): the model
  // position is a fraction of the travel, so the old fraction would map to a different sled
  // position and the first command would jump by up to the hard leash.
  static int32_t s_lastSoftEndstopMin_i32 = 0;
  static int32_t s_lastSoftEndstopMax_i32 = 0;
  if ((calc_st->softEndstopMinStepperPos_i32 != s_lastSoftEndstopMin_i32) ||
      (calc_st->softEndstopMaxStepperPos_i32 != s_lastSoftEndstopMax_i32)) {
    s_lastSoftEndstopMin_i32 = calc_st->softEndstopMinStepperPos_i32;
    s_lastSoftEndstopMax_i32 = calc_st->softEndstopMaxStepperPos_i32;
    g_vModelPos_01 = actualPosFraction_01;
    g_vModelVel_mps = 0.0f;
    g_lastNetForceTustin_N = 0.0f;
  }

  // --- 3. ELASTOMER PHYSICS & SPRING REACTION (Hunt-Crossley Model) ---
  // Coupled Spring Displacement: We blend the virtual target position with the actual
  // physical mechanism position. This prevents the virtual model from prematurely hitting
  // a steep force wall (e.g. 40kg) and bouncing back violently while the physical servo is
  // still traveling forward to catch up.
  float effectiveDisplacement_01 = constrain(0.65f * g_vModelPos_01 + 0.35f * actualPosFraction_01, 0.0f, 1.0f);
  float avgStiffness_N_m = (calc_st->forceRange_fl32 * GRAVITY_N_KG) / max(totalTravel_m, 0.0001f);

  // Standard Sim Racing Pedal (Throttle / Brake / Clutch)
  float springForceRaw_kg = forceCurve->EvalForceCubicSpline(config_st, calc_st, effectiveDisplacement_01);
  float springForce_N = max(springForceRaw_kg * GRAVITY_N_KG, 0.0f);

  float localStiffness_kg_step = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, effectiveDisplacement_01, false);
  float gradStiffness_N_m = fabsf(localStiffness_kg_step) * (travelSteps_cnt / max(totalTravel_m, 0.0001f)) * GRAVITY_N_KG;
  float localStiffness_N_m = max(gradStiffness_N_m, max(0.3f * avgStiffness_N_m, 1.0f));

  // =========================================================
  // --- 4. VISCOELASTIC ELASTOMER HYSTERESIS (Hunt-Crossley model) ---
  // =========================================================
  float progressionRatio = (float)constrain( config_st->payloadPedalConfig_st.dampingProgression_u8, 0, 100) ;
  float progression_01 = progressionRatio * progressionRatio; 
  float localCriticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);
  const float MAX_ELASTOMER_MULTIPLIER = 4.0f; 
  float ELASTOMER_VISCOSITY_COEFFICIENT = progression_01 * (MAX_ELASTOMER_MULTIPLIER * localCriticalDamping_Ns_m);
  float elastomerDamping_Ns_m = ELASTOMER_VISCOSITY_COEFFICIENT * effectiveDisplacement_01;

  // --- 5. EFFECT OFFSETS & TOTAL FORCE ---
  // Feedforward Control (Inverse Dynamics)
  // We derive velocity and acceleration internally from the effect step offset
  // using cascaded EMA filters to avoid numerical explosions.
  
  // 1. Convert effect steps to task-space meters
  float metersPerStep = 0.0f;
  if (travelSteps_cnt > 0.0001f) {
      metersPerStep = totalTravel_m / travelSteps_cnt;
  }
  float rawEffectPos_m = effectOffsets_st.forceOffset_Steps_fl32 * metersPerStep;

  // 2. Smooth the position to avoid 60Hz staircase Dirac delta spikes
  const float EFFECT_TAU_POS = 0.005f; // 5ms smoothing
  float alpha_pos = 1.0f - expf(-dt_s / EFFECT_TAU_POS);
  g_smoothedEffectPos_m = (alpha_pos * rawEffectPos_m) + ((1.0f - alpha_pos) * g_smoothedEffectPos_m);

  // 3. Derive and smooth Velocity
  float rawEffectVel_mps = (g_smoothedEffectPos_m - g_prevSmoothedEffectPos_m) / dt_s;
  g_prevSmoothedEffectPos_m = g_smoothedEffectPos_m;
  
  const float EFFECT_TAU_VEL = 0.005f; // 5ms smoothing
  float alpha_vel = 1.0f - expf(-dt_s / EFFECT_TAU_VEL);
  g_smoothedEffectVel_mps = (alpha_vel * rawEffectVel_mps) + ((1.0f - alpha_vel) * g_smoothedEffectVel_mps);

  // 4. Derive and smooth Acceleration
  float rawEffectAcc_mps2 = (g_smoothedEffectVel_mps - g_prevSmoothedEffectVel_mps) / dt_s;
  g_prevSmoothedEffectVel_mps = g_smoothedEffectVel_mps;

  const float EFFECT_TAU_ACC = 0.010f; // 10ms smoothing
  float alpha_acc = 1.0f - expf(-dt_s / EFFECT_TAU_ACC);
  g_smoothedEffectAcc_mps2 = (alpha_acc * rawEffectAcc_mps2) + ((1.0f - alpha_acc) * g_smoothedEffectAcc_mps2);

  // 5. Calculate Inverse Dynamics Feedforward Force (Newton)
  // F_effekt = K*x + C*v + M*a
  float idealBaseDamping_Ns_m_ff = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);
  
  float effectInjectedForce_N = (g_smoothedEffectPos_m * localStiffness_N_m) 
                              + (g_smoothedEffectVel_mps * idealBaseDamping_Ns_m_ff)
                              + (g_smoothedEffectAcc_mps2 * virtualMass_kg);

  // 6. Final total force (Loadcell + Static Effect Weight + Dynamic Effect Force)
  float rawPilotForce_N = (loadCellReadingKg_fl32 * GRAVITY_N_KG);
  // no deadzone here: the loadcell calibration already removes mean + 3 sigma of the idle reading
  float cleanPilotForce_N = max(rawPilotForce_N, 0.0f);
  static float s_filteredPilotForce_N = 0.0f;
  const float PILOT_FORCE_TAU = 0.025f; // 25ms smoothing
  float pilot_alpha = 1.0f - expf(-dt_s / PILOT_FORCE_TAU);
  s_filteredPilotForce_N = (pilot_alpha * cleanPilotForce_N) + ((1.0f - pilot_alpha) * s_filteredPilotForce_N);

  calc_st->currentPedalForce_N_fl32 = s_filteredPilotForce_N;

  // Contact Impedance Damping (Force Derivative Feedback):
  // When the servo accelerates into the user's stiff foot/shoe, high-frequency force spikes
  // occur (dF/dt > 5000 N/s). A virtual damper at the foot-pedal contact adds tau * dF/dt:
  // against a held foot F = k_c * (x_foot - x), so dF/dt = -k_c * v and the term acts as
  // damping tau * k_c. (Subtracting it, F - tau * dF/dt ~ F(t - tau), was a 4 ms delay and
  // gave negative damping of tau * k_c, ~40 Ns/m at a 10 N/mm heel contact.)
  static float s_prevPilotForce_N = 0.0f;
  static float s_filteredForceRate_Nps = 0.0f;
  float rawForceRate_Nps = (cleanPilotForce_N - s_prevPilotForce_N) / dt_s;
  s_prevPilotForce_N = cleanPilotForce_N;

  const float FORCE_RATE_TAU = 0.008f; // 8ms smoothing for derivative
  float alpha_rate = 1.0f - expf(-dt_s / FORCE_RATE_TAU);
  s_filteredForceRate_Nps = (alpha_rate * rawForceRate_Nps) + ((1.0f - alpha_rate) * s_filteredForceRate_Nps);

  const float K_FORCE_DERIV_S = 0.004f; // 4ms contact damping time
  float contactDampedPilotForce_N = cleanPilotForce_N + (K_FORCE_DERIV_S * s_filteredForceRate_Nps);
  if (contactDampedPilotForce_N < 0.0f) {
    contactDampedPilotForce_N = 0.0f;
  }

  float externalForce_N = contactDampedPilotForce_N + (effectOffsets_st.forceOffset_kg_fl32 * GRAVITY_N_KG) + effectInjectedForce_N;

  // --- 7. DYNAMIC TRAVEL LIMITS ---
  float lowerTravelLimit_01 = 0.0f;
  float upperTravelLimit_01 = 1.0f;
  CalcDynamicTravelLimits(travelSteps_cnt, localStiffness_kg_step, effectOffsets_st, lowerTravelLimit_01, upperTravelLimit_01);

  // --- 8. SOFT ENDSTOP CALCULATION ---
  float currentStiffness_N_m = localStiffness_N_m;
  float softEndstopForce_N = CalcSoftEndstopForce(g_vModelPos_01, totalTravel_m, endstopBehavior_st, currentStiffness_N_m, upperTravelLimit_01);

  // --- 9. ADMITTANCE OSCILLATION DETECTOR (Landi et al.) ---
  // We calculate base damping with un-adapted mass for the physics ideal-model reference
  float idealBaseDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(baseMass_kg * currentStiffness_N_m);
  
  // Use the exact restoring force (spline + endstop) instead of linear stiffness assumption
  float totalSpringReaction_N = springForce_N + softEndstopForce_N;
  
  // Effect active: held for EFFECT_ACTIVE_HOLD_S after the last non-zero offset. The vibration
  // effects pass through exactly zero twice per period (the RPM offset is truncated to int), so
  // the instantaneous test dropped out for ~1 ms each time and let stiction, the contact
  // oscillation detector and the tracking-error damping switch on mid-vibration.
  const float EFFECT_ACTIVE_HOLD_S = 0.1f;
  static float s_effectActiveHoldRemaining_s = 0.0f;
  if ((effectOffsets_st.forceOffset_kg_fl32 != 0.0f) || (effectOffsets_st.forceOffset_Steps_fl32 != 0.0f)) {
    s_effectActiveHoldRemaining_s = EFFECT_ACTIVE_HOLD_S;
  } else {
    s_effectActiveHoldRemaining_s = max(0.0f, s_effectActiveHoldRemaining_s - dt_s);
  }
  bool hasActiveEffect = (s_effectActiveHoldRemaining_s > 0.0f);

  // Landi detector: only for telemetry (physical kinematics, expected force). Its threshold (25 N)
  // and power gating missed the heel-contact oscillation (~3.4 N at 16 Hz), and the mass
  // adaptation it drove does not stabilize a stiff contact (simulation).
  // Only evaluated when telemetry is requested (debugState_st given), to save CPU time.
  if (debugState_st != nullptr) {
    DetectAdmittanceOscillation(
        externalForce_N, actualPosFraction_01, totalTravel_m,
        totalSpringReaction_N, idealBaseDamping_Ns_m, baseMass_kg,
        dt_s, config_st->payloadPedalConfig_st.maxForce_fl32, debugState_st, hasActiveEffect
    );
  }

  // --- 10. CONTACT OSCILLATION DAMPING (replaces the virtual mass adaptation) ---
  bool isOscillating = false;
  float contactOscBandPeak_N = 0.0f;
  float contactOscDampingMultiplier = UpdateContactOscillationDamping(
      cleanPilotForce_N, g_vModelVel_mps, dt_s, hasActiveEffect, isOscillating, contactOscBandPeak_N);
  if (debugState_st != nullptr) {
      debugState_st->admittancePsi_N = contactOscBandPeak_N; // telemetry: band-pass half-wave peak
  }

  // --- 11. DYNAMIC ADAPTIVE DAMPING ---
  // (base damping, tracking-error and elastomer damping; the virtual mass is not adapted)
  float activeDamping_Ns_m = CalcActiveDamping(dampingRatio_zeta
    , virtualMass_kg
    , currentStiffness_N_m
    , g_vModelPos_01
    , actualPosFraction_01
    , stepper->getServosPosError()
    , travelSteps_cnt
    , hasActiveEffect
    , config_st->payloadPedalConfig_st.dampingProgression_u8
    , springForce_N
    , g_vModelVel_mps
    , ELASTOMER_MODEL_HUNT_CROSSLEY
    , config_st->payloadPedalConfig_st.maxForce_fl32
    , dt_s
    , totalTravel_m);

  // raise the damping while a contact oscillation is present (heel held against the pedal)
  activeDamping_Ns_m *= contactOscDampingMultiplier;

  // =========================================================
  // NEUER FIX: Bump-Stop Hysteresis (Kinetische Energie absorbieren)
  // =========================================================
  // Wenn das Pedal tief im Soft-Endstop ist, müssen wir die massive gespeicherte 
  // Federenergie dämpfen, um den "Trampolin-Kickback" zu verhindern.
  if (g_vModelPos_01 > 1.0f && endstopBehavior_st.travelRange_mm_fl32 > 0.01f) {
      
      // Wie tief sind wir im Endanschlag? (0.0 = Start, 1.0 = absolutes Limit)
      float softEndstopTravel_m = endstopBehavior_st.travelRange_mm_fl32 * 0.001f;
      float deflection_m = (g_vModelPos_01 - 1.0f) * totalTravel_m;
      float endstopDepth_01 = constrain(deflection_m / max(softEndstopTravel_m, 0.001f), 0.0f, 1.0f);
      
      // Berechne die kritische Dämpfung exakt für die harte Endanschlag-Feder
      float endstopStiffness_N_m = endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 * 1000.0f;
      float endstopCriticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * endstopStiffness_N_m);
      
      // ASYMMETRISCHE DÄMPFUNG (Das Geheimnis gegen Kickback):
      // Beim Hineintreten (v > 0) dämpfen wir leicht (Zeta 0.3), damit der Aufprall nicht wie eine Mauer wirkt.
      // Beim Zurückfedern (v < 0) dämpfen wir extrem stark (Zeta 1.5 -> überdämpft).
      // Dadurch kriecht das Pedal sanft aus dem Endanschlag heraus, anstatt zurückzufeuern.
      float targetZeta = (g_vModelVel_mps < 0.0f) ? 1.5f : 0.3f; 
      
      float requiredBumpDamping_Ns_m = targetZeta * endstopCriticalDamping_Ns_m * endstopDepth_01;
      
      // Überschreibe die globale Dämpfung, wenn der Bump-Stop mehr Dämpfung erfordert
      if (activeDamping_Ns_m < requiredBumpDamping_Ns_m) {
          activeDamping_Ns_m = requiredBumpDamping_Ns_m;
      }
  }

  // =========================================================
  // STATISTICAL STICTION (HOLD probability from the IMM force filter)
  // =========================================================
  // While the force filter is confident the foot is holding still, extra damping near
  // zero velocity keeps residual noise from moving the pedal. It fades out with speed,
  // so presses are unaffected, and enters the Tustin integrator like all other damping.
  if (holdProbability_01 > 0.0f) {
    // EXPERIMENTAL, off by default: mixed results in simulation (more jitter after a press)
    const float HOLD_DAMPING_RATIO = 0.0f;
    const float HOLD_DAMPING_VELOCITY_BAND_MPS = 0.02f;
    float velocityWeight_01 = max(0.0f, 1.0f - fabsf(g_vModelVel_mps) / HOLD_DAMPING_VELOCITY_BAND_MPS);
    float holdCriticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
    activeDamping_Ns_m += holdProbability_01 * HOLD_DAMPING_RATIO * holdCriticalDamping_Ns_m * velocityWeight_01;
  }

  g_lastActiveDamping_Ns_m = activeDamping_Ns_m;

  // --- 12. INTEGRATION (MASS-SPRING-DAMPER-ENDSTOP) ---
  // F_net = F_human - F_spring - F_softEndstop - F_damping
  float dampingForce_N = activeDamping_Ns_m * g_vModelVel_mps;
  float netForce_N = externalForce_N - springForce_N - softEndstopForce_N - dampingForce_N;
  
  // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
  const float FRICTION_N = config_st->payloadPedalConfig_st.coulombFrictionIn0p1N_u8 * 0.1f;
  // FIXED CODE: Smooth Coulomb Friction
  // Create a narrow "fade" band around 0 velocity (+/- VELOCITY_BAND_MPS), shared by all
  // integration methods below
  // This smoothly ramps the friction from -1 to +1 across the zero point
  const float VELOCITY_BAND_MPS = 0.030f; // Verbreitert für weicheren Nulldurchgang (verhindert Ruckeln/Schläge bei Richtungswechsel)
  float frictionBlend = constrain(g_vModelVel_mps / VELOCITY_BAND_MPS, -1.0f, 1.0f);
  netForce_N -= (FRICTION_N * frictionBlend);

  // =========================================================
  // STICTION (stick/slip, Karnopp model)
  // =========================================================
  // While holding, the foot's natural tremor (~0.2-0.3 N, 1-15 Hz) moved the pedal back and
  // forth ~15 times per second; that motion is felt as vibration (a pedal held still is calm).
  // Damping and the Coulomb term above only scale this motion, since both vanish at v = 0.
  // Stiction holds the pedal still until the force imbalance exceeds the static friction,
  // then it slides with the slightly lower kinetic friction. Replay of logged holds: reversals
  // 17/s -> 0.2-0.5/s, press onset +1.6 ms, hold offset <= F_static / k (~0.3 mm). A kinetic
  // ratio close to 1 keeps the stick-slip steps of very slow creeping (< 1 N/s) small.
  // Static friction scales with the foot force (tremor grows with force), but never exceeds
  // half of it, so the pedal always returns to its rest position when released.
  // Disabled while effects are active, so small effect forces are never swallowed.
  const float STICTION_MIN_N = 0.5f;               // static friction at low force
  const float STICTION_RELATIVE_01 = 0.02f;        // ... growing to 2 % of the foot force
  const float STICTION_KINETIC_RATIO_01 = 0.85f;   // kinetic / static friction
  const float STICTION_VELOCITY_MPS = 0.0005f;     // below this the pedal may stick
  static bool s_isStuck_b = false;
  float staticFriction_N = 0.0f;
#ifdef ADMITTANCE_STICTION_ENABLED
  if (!hasActiveEffect) {
    staticFriction_N = min(max(STICTION_MIN_N, STICTION_RELATIVE_01 * s_filteredPilotForce_N),
                           0.5f * s_filteredPilotForce_N);
  }
#endif
  // force that would move the pedal if it were free (without damping and friction)
  float stictionDriveForce_N = externalForce_N - springForce_N - softEndstopForce_N;
  if ((staticFriction_N <= 0.0f) || (fabsf(stictionDriveForce_N) > staticFriction_N)) {
    s_isStuck_b = false; // break away
  }

  // =========================================================
  // Integration approaches (Start)
  // =========================================================
  // Update virtual acceleration
  float acceleration_mps2 = 0.0f;
  
  // 0 = Explicit Euler (Legacy)
  // 1 = Implicit Backward Euler
  // 2 = Exact mass-damper step (exponential integrator, formerly Tustin)
  int integration_method = 2; 
  
  switch(integration_method)
  {
    case 1: // IMPLICIT EULER
    {
      // We calculate the net force WITHOUT damping. 
      // Damping will be handled implicitly in the algebraic derivation below.
      float netForce_without_damping_N = externalForce_N - springForce_N - softEndstopForce_N;

      // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
      // FIXED CODE: Smooth Coulomb Friction
      float frictionBlendImplicit = constrain(g_vModelVel_mps / VELOCITY_BAND_MPS, -1.0f, 1.0f);
      netForce_without_damping_N -= (FRICTION_N * frictionBlendImplicit);

      // =========================================================
      // IMPLICIT EULER (BACKWARD EULER) MATHEMATICS
      // =========================================================
      // Derivation of the unconditionally stable velocity formula:
      //
      // 1. Newton's second law: 
      //    M * a = F_net
      //
      // 2. Implicit Euler requires calculating forces at the END of the timestep (future state):
      //    a = (v_new - v_old) / dt
      //    F_damping_new = C * v_new
      //    F_spring_new  = F_spring_old + (K * v_new * dt)
      //
      // 3. Substitute into Newton's law:
      //    M * (v_new - v_old) / dt = F_ext - F_spring_new - F_damping_new
      //    M * (v_new - v_old) / dt = F_ext - (F_spring_old + K * v_new * dt) - (C * v_new)
      //
      // 4. Let F_basis = F_ext - F_spring_old (this is 'netForce_without_damping_N').
      //    Multiply the entire equation by 'dt' to clear the fraction:
      //    M * v_new - M * v_old = F_basis * dt - K * v_new * dt^2 - C * v_new * dt
      //
      // 5. Group all terms containing 'v_new' on the left side:
      //    M * v_new + C * v_new * dt + K * v_new * dt^2 = M * v_old + F_basis * dt
      //
      // 6. Factor out 'v_new' and divide to solve for 'v_new':
      //    v_new * (M + C * dt + K * dt^2) = M * v_old + F_basis * dt
      //    v_new = (M * v_old + F_basis * dt) / (M + C * dt + K * dt^2)
      // =========================================================
      
      // Denominator of the derived equation: (M + C*dt + K*dt^2)
      // Notice how K*dt^2 prevents division by near-zero when the virtual mass is very small.
      float implicit_denominator = virtualMass_kg + 
                                   (activeDamping_Ns_m * dt_s) + 
                                   (currentStiffness_N_m * dt_s * dt_s);
  
      // Numerator of the derived equation: (M*v_old) + (F_basis * dt)
      float implicit_numerator = (virtualMass_kg * g_vModelVel_mps) + 
                                 (netForce_without_damping_N * dt_s);
  
      // 1. Calculate the NEW velocity directly and stably
      float new_vModelVel_mps = g_vModelVel_mps;
      if(fabsf(implicit_denominator) > 1e-5)
      {
          new_vModelVel_mps = implicit_numerator / implicit_denominator;
      }
  
      // 2. Reconstruct the effective acceleration for this time step.
      // We reverse-engineer the acceleration so we don't have to rewrite 
      // subsequent safety features (like Regen-Power-Clamping or Hard-Limits).
      acceleration_mps2 = (new_vModelVel_mps - g_vModelVel_mps) / dt_s;
      
      break;
    }

    case 2: // EXACT MASS-DAMPER STEP (formerly TUSTIN)
    {
      // We calculate the net force WITHOUT damping. 
      // Damping will be mathematically modeled perfectly within the IIR filter.
      float netForce_without_damping_N = externalForce_N - springForce_N - softEndstopForce_N;

      // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
      // FIXED CODE: Smooth Coulomb Friction
      float frictionBlendTustin = constrain(g_vModelVel_mps / VELOCITY_BAND_MPS, -1.0f, 1.0f);
      netForce_without_damping_N -= (FRICTION_N * frictionBlendTustin);

      // kinetic friction while sliding (direction of motion, or of the drive force at breakaway)
      if ((staticFriction_N > 0.0f) && !s_isStuck_b) {
        float slipDirection = (fabsf(g_vModelVel_mps) > STICTION_VELOCITY_MPS)
                                  ? copysignf(1.0f, g_vModelVel_mps)
                                  : copysignf(1.0f, stictionDriveForce_N);
        netForce_without_damping_N -= STICTION_KINETIC_RATIO_01 * staticFriction_N * slipDirection;
      }

      // =========================================================
      // EXACT MASS-DAMPER STEP (exponential integrator)
      // =========================================================
      // The spring is treated as an external force to keep the non-linear splines intact.
      // Continuous system: M*a + C*v = F_netto, with F_netto averaged over the step
      // (F_avg = (F_new + F_old) / 2, the same force memory as the former Tustin filter).
      // Exact solution over one step for a constant F_avg:
      //    v_new = e * v_old + (1 - e) * F_avg / C,   e = exp(-C * dt / M)
      // The pole e is always in (0, 1). Tustin's pole (1 - r) / (1 + r), r = C * dt / (2M),
      // turns negative for r > 1 (high damping, low mass, long cycle): the velocity then
      // flipped sign every cycle (Nyquist buzz). For small r both agree to second order.
      // For C * dt / M -> 0 the limit is v_old + F_avg * dt / M.
      // =========================================================
      float new_vModelVel_mps = g_vModelVel_mps;
      float averageNetForce_N = 0.5f * (netForce_without_damping_N + g_lastNetForceTustin_N);
      if (virtualMass_kg > 1e-5f)
      {
          float dampingRate_1 = activeDamping_Ns_m * dt_s / virtualMass_kg;
          if (dampingRate_1 > 1e-4f) {
              float decay_01 = expf(-dampingRate_1);
              new_vModelVel_mps = decay_01 * g_vModelVel_mps
                                + (1.0f - decay_01) * averageNetForce_N / activeDamping_Ns_m;
          } else {
              new_vModelVel_mps = g_vModelVel_mps + averageNetForce_N * dt_s / virtualMass_kg;
          }
      }

      // Stick: stay stuck, or stick when the motion stops or reverses while the drive force
      // is below the static friction. Friction then balances the drive force exactly, so the
      // pedal holds still and the integrator memory sees a net force of zero.
      if (staticFriction_N > 0.0f) {
        bool isStoppingOrReversing = ((new_vModelVel_mps * g_vModelVel_mps) <= 0.0f) ||
                                     (fabsf(new_vModelVel_mps) < STICTION_VELOCITY_MPS);
        if (s_isStuck_b ||
            (isStoppingOrReversing && (fabsf(stictionDriveForce_N) <= staticFriction_N))) {
          s_isStuck_b = true;
          new_vModelVel_mps = 0.0f;
          netForce_without_damping_N = 0.0f;
        }
      }

      // Store the current net force for the next integration cycle (z^-1)
      g_lastNetForceTustin_N = netForce_without_damping_N;

      // Reconstruct the effective acceleration for this time step.
      // This is necessary so the existing limits (Regen clamping, hard max accel) 
      // can operate unmodified on the integrator output.
      acceleration_mps2 = (new_vModelVel_mps - g_vModelVel_mps) / dt_s;

      break;
    }

    case 0: // EXPLICIT EULER
    default:
    {
      // Explicit Forward Euler (legacy, prone to instability with small masses)
      acceleration_mps2 = netForce_N / virtualMass_kg;
      break;
    }
  }

  // =========================================================
  // Integration approaches (End)
  // =========================================================

  // Hard Clamping of acceleration to prevent limit cycles (Hunting)
  const float MAX_ACCEL_MPS2 = 30.0f; 
  acceleration_mps2 = constrain(acceleration_mps2, -MAX_ACCEL_MPS2, MAX_ACCEL_MPS2);

  // Velocity Integration
  g_vModelVel_mps += acceleration_mps2 * dt_s;

  // --- 13. VELOCITY CHOKING & REGENERATIVE EMF GOVERNOR ---
  // Limit the movement speed if the system becomes unstable or generates excessive regen power.
  float velocityLimit_01 = 1.0f; // speed scale (currently no reduction)
  
  float maxPhysicalSledVel_mps = 0.8f; 
  if (calc_st->stepsPerMotorRevolution_u32 > 0) {
      maxPhysicalSledVel_mps = (float)MAXIMUM_STEPPER_SPEED_U32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 / (float)calc_st->stepsPerMotorRevolution_u32 * 0.001f;
  }
  
  // Scale sled velocity limit to Task Space (Pedal Arc) velocity limit
  // FIXED: Converted maxSledPos_mm to meters (* 0.001f) to prevent dividing m by mm!
  float maxSledPos_m = max(maxSledPos_mm * 0.001f, 0.0001f);
  float maxPedalArcVel_mps = maxPhysicalSledVel_mps * (totalTravel_m / maxSledPos_m);
  float dynamicSpeedLimit = maxPedalArcVel_mps * velocityLimit_01;
  
  // Regenerative Power & Back-EMF Clamping:
  // Moving forward (v > 0) against the spring, endstop, and damping forces converts foot mechanical power
  // into electrical energy (P = F_oppose * v). We limit forward velocity so P <= MAX_REGEN_POWER_W (40 W) and
  // motor RPM stays within the stepper's actual configured max (see CalcRegenVelocityLimit).
  float totalOpposingForce_N = springForce_N + softEndstopForce_N + fabsf(dampingForce_N);
  float softEndstopTravel_m = endstopBehavior_st.travelRange_mm_fl32 * 0.001f;
  float spindlePitch_mm = (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;

  float maxRegenVel_mps = CalcRegenVelocityLimit(
      totalOpposingForce_N,
      totalTravel_m,
      maxSledPos_m,
      spindlePitch_mm,
      g_vModelPos_01,
      softEndstopTravel_m,
      calc_st->stepsPerMotorRevolution_u32
  );

  float forwardSpeedLimit = min(dynamicSpeedLimit, maxRegenVel_mps);
  g_vModelVel_mps = constrain(g_vModelVel_mps, -dynamicSpeedLimit, forwardSpeedLimit);

  // --- 14. POSITION INTEGRATION, BOUNDARY CONSTRAINTS & DRIFT CORRECTION ---
  // Update virtual position based on velocity
  float currentPos_m = (g_vModelPos_01 * totalTravel_m) + (g_vModelVel_mps * dt_s);
  g_vModelPos_01 = (totalTravel_m > 0.0001f) ? (currentPos_m / totalTravel_m) : 0.0f;

  // Apply boundary constraints (with soft wall instead of hard velocity zeroing)
  if (g_vModelPos_01 < lowerTravelLimit_01) {
      g_vModelPos_01 = lowerTravelLimit_01;
      if (g_vModelVel_mps < 0.0f) g_vModelVel_mps = 0.0f;
  } else if (g_vModelPos_01 > upperTravelLimit_01) {
      g_vModelPos_01 = upperTravelLimit_01;
      // FIX: Dampen velocity rather than hard zeroing to prevent step excitation in the integrator
      if (g_vModelVel_mps > 0.0f) g_vModelVel_mps *= 0.5f; 
  }
  
  // SOFT LEASH: Synchronize the virtual model with the actual stepper command position
  // to prevent divergence due to numerical drift without corrupting second-order dynamics.
  // Uses the unclamped sled position (limited to the dynamic travel limits): with the clamped
  // value the leash pulled the model out of the soft endstop and the hard leash cut its last mm.
  float actualPosLeash_01 = constrain(actualPosFractionRaw_01, lowerTravelLimit_01, upperTravelLimit_01);
  float divergence_01 = actualPosLeash_01 - g_vModelPos_01;
  
  // SOFT LEASH DEADBAND: Verhindert, dass Sensorrauschen Schläge ins Physikmodell überträgt
  if (fabsf(divergence_01) < 0.005f) { // Auf 0.5% Toleranz erhöht (ca. 0.5 mm Pufferzone)
      divergence_01 = 0.0f;
  } else {
      divergence_01 += (divergence_01 > 0.0f) ? -0.005f : 0.005f;
  }
  const float LEASH_RATE = 0.5f; // Noch sanfterer Rückzug (0.5 statt 1.0)
  g_vModelPos_01 += divergence_01 * (LEASH_RATE * dt_s);

  // HARD LEASH CLAMP (Anti-Runaway):
  // The virtual model is strictly constrained so it cannot outrun the physical sled by more than
  // 8 % of the travel (~8 mm at ~100 mm pedal travel).
  const float MAX_LEAD_01 = 0.08f;
  if ((g_vModelPos_01 - actualPosLeash_01) > MAX_LEAD_01) {
      g_vModelPos_01 = actualPosLeash_01 + MAX_LEAD_01;
      if (g_vModelVel_mps > 0.0f) {
          g_vModelVel_mps *= 0.5f; // Damp forward velocity when actuator speed is saturated
      }
  } else if ((actualPosLeash_01 - g_vModelPos_01) > MAX_LEAD_01) {
      g_vModelPos_01 = actualPosLeash_01 - MAX_LEAD_01;
      if (g_vModelVel_mps < 0.0f) {
          g_vModelVel_mps *= 0.5f;
      }
  }

  // =========================================================
  // POPULATE REMAINDER OF DEBUG TELEMETRY STRUCT
  // =========================================================
  if (debugState_st != nullptr) {
      debugState_st->tankEnergy_J = g_tankEnergy_J;
      debugState_st->massAdaptationOffset_kg = g_massAdaptationOffset_kg;
      debugState_st->activeVirtualMass_kg = virtualMass_kg;
      debugState_st->activeDamping_Ns_m = activeDamping_Ns_m;
      debugState_st->isOscillating = isOscillating;
      
      debugState_st->virtualPos_01 = g_vModelPos_01;
      debugState_st->virtualVel_mps = g_vModelVel_mps;
      debugState_st->virtualAcc_mps2 = acceleration_mps2;
  }

  // =========================================================
  // --- 15. INVERSE KINEMATICS (ANALYTICAL) & STEP CONVERSION ---
  // =========================================================
  // We now have the target virtual position fraction (g_vModelPos_01) which lives 
  // natively in the Pedal Task Space (Arc length). We must translate this 
  // target back into non-linear physical stepper steps via Inverse Kinematics.

  // 1. Map the normalized task space target [0.0, 1.0] to a target pedal angle
  float targetPedalAngle_deg = angleAtMinSled_deg + g_vModelPos_01 * (angleAtMaxSled_deg - angleAtMinSled_deg);
  float targetPedalAngle_rad = targetPedalAngle_deg * DEG_TO_RAD_FL32;

  // 2. Analytical Inverse Kinematics (O(1), perfect precision, absolutely zero hysteresis)
  // The geometry is a triangle A-B-C where:
  // A = Lower pedal pivot (origin 0,0)
  // C = Loadcell connection to pedal = (b * cos(phi), b * sin(phi))
  // B = Sled pivot = (c_hor_0 + x, c_vert)
  // Distance between B and C is the loadcell rod length 'a'.
  
  float a_mm = (float)config_st->payloadPedalConfig_st.lengthPedalA_i16;
  float b_mm = (float)config_st->payloadPedalConfig_st.lengthPedalB_i16;
  float c_vert_mm = (float)config_st->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float c_hor_0_mm = (float)config_st->payloadPedalConfig_st.lengthPedalCHorizontal_i16;

  // Calculate coordinates of the pedal pivot C
  float Cx_mm = b_mm * cosf(targetPedalAngle_rad);
  float Cy_mm = b_mm * sinf(targetPedalAngle_rad);

  // Solve for target sled position using the circle equation: (B_x - C_x)^2 + (B_y - C_y)^2 = a^2
  // -> (c_hor - Cx)^2 + (c_vert - Cy)^2 = a^2
  float dy_mm = c_vert_mm - Cy_mm;
  float dx_squared_mm2 = (a_mm * a_mm) - (dy_mm * dy_mm);

  float targetSledPos_mm = actualSledPos_mm; // Fallback in case of kinematic impossibility
  
  if (dx_squared_mm2 > 0.0f) {
      // We use the positive root because the sled (B) is horizontally further away than the pedal arm (C)
      float c_hor_mm = Cx_mm + sqrtf(dx_squared_mm2);
      targetSledPos_mm = c_hor_mm - c_hor_0_mm;
  }

  // Limit dynamic expansion to a maximum of +/- 5% of total travel for safety
  float limitExt = travelSteps_cnt * 0.05f;
  float maxExt = constrain(max(0.0f, effectOffsets_st.forceOffset_Steps_fl32), 0.0f, limitExt);
  float minExt = constrain(min(0.0f, effectOffsets_st.forceOffset_Steps_fl32), -limitExt, 0.0f);

  // Calculate equivalent expansion in millimeters for the sled position clamp
  float maxExt_mm = maxExt * (motorRevolutionsPerSteps_lcl_fl32 * pitch_mm);
  float minExt_mm = minExt * (motorRevolutionsPerSteps_lcl_fl32 * pitch_mm);

  // Add the soft endstop travel allowance to the physical bounds. The soft endstop travel is
  // pedal arc length (task space, see CalcSoftEndstopForce): convert it to sled mm with the
  // mean sled-per-arc ratio of the travel.
  float endstopTravelArc_mm = (endstopBehavior_st.travelRange_mm_fl32 > 0.01f) ? endstopBehavior_st.travelRange_mm_fl32 : 0.0f;
  float sledPerArc_01 = (totalTravel_m > 0.0001f) ? (maxSledPos_m / totalTravel_m) : 1.0f;
  float endstopTravel_mm = endstopTravelArc_mm * sledPerArc_01;
  float endstopTravel_steps = endstopTravel_mm / (motorRevolutionsPerSteps_lcl_fl32 * pitch_mm);

  // Clamp the solved sled position to safe physical bounds (with dynamic expansion AND soft endstop)
  targetSledPos_mm = constrain(targetSledPos_mm, 0.0f + minExt_mm, maxSledPos_mm + maxExt_mm + endstopTravel_mm);
  
  // 3. Convert target sled position (mm) back to absolute stepper steps
  float targetPosSteps_fl32 = (targetSledPos_mm / (motorRevolutionsPerSteps_lcl_fl32 * pitch_mm)) + (float)calc_st->softEndstopMinStepperPos_i32;

  // Clamp to software limits (with dynamic expansion for high-frequency effects AND soft endstops)
  float finalTargetPos_fl32 = targetPosSteps_fl32;
  finalTargetPos_fl32 = constrain(finalTargetPos_fl32, calc_st->softEndstopMinStepperPos_i32 + minExt, calc_st->softEndstopMaxStepperPos_i32 + maxExt + endstopTravel_steps);

  if (admittanceStates_pst != nullptr) {
    admittanceStates_pst->physicalPos_m = g_vModelPos_01 * totalTravel_m; // Logged natively in Task Space
    admittanceStates_pst->virtualVel_mps = g_vModelVel_mps;
    admittanceStates_pst->virtualAcc_mps2 = acceleration_mps2;
  }
  
  return finalTargetPos_fl32;
}