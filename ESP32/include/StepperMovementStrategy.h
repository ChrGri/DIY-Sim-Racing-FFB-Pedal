#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"

// Task dependent structs and variables
typedef struct {
  float travelRange_mm_fl32;
  float stiffnessAtMaxTravel_Npermm_fl32;
} EndstopBehavior_t;

typedef struct {
  float forceOffset_kg_fl32;
  float forceOffset_Steps_fl32;
} EffectOffsets_t;

typedef struct {
  bool isRudderMode;
  float centerPosition_01;     // nomally 0.5 so alignment in center 
  float trimOffset_01;         // dynamical displacement the 0-force psoition
  float deadzone_01;           // small region without force around center
} RudderOffsets_t;


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

// =========================================================
// HELPER SUB-FUNCTIONS FOR ENCAPSULATION
// =========================================================

/**
 * @brief Calculates dynamic travel limits based on effect offsets and local stiffness
 */
static inline void CalcDynamicTravelLimits(
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
        upperTravelLimit_01 = 1.0f + max(0.0f, ext_steps);
    }
}

/**
 * @brief Simulates a heavy spring pushing back once the travel range is exceeded
 */
static inline float CalcSoftEndstopForce(
    float vModelPos_01, float totalTravel_m, const EndstopBehavior_t& endstopBehavior_st,
    float& currentStiffness_N_m, float& upperTravelLimit_01) 
{
    float softEndstopForce_N = 0.0f;
    if (endstopBehavior_st.travelRange_mm_fl32 > 0.01f) {
        if (vModelPos_01 > 1.0f) {
            float softEndstopStiffness_N_m = endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 * 1000.0f;
            float deflection_m = (vModelPos_01 - 1.0f) * totalTravel_m;
            softEndstopForce_N = softEndstopStiffness_N_m * deflection_m;
            
            // Update local stiffness for damping calculation to prevent endstop bouncing
            currentStiffness_N_m = softEndstopStiffness_N_m;
        }
        // Expand upper limit to allow the virtual model to penetrate the soft endstop
        upperTravelLimit_01 += (endstopBehavior_st.travelRange_mm_fl32 / 1000.0f) / totalTravel_m;
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
static inline bool DetectAdmittanceOscillation(
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
static inline void AdaptVirtualMass(
    bool isOscillating, float oscillationIntensity_01, float dt_s, 
    float baseMass_kg, float& virtualMass_kg, bool hasActiveEffect, float actualPosFraction_01)
{
    // Freeze adaptation if an effect is active
    if (hasActiveEffect) {
        virtualMass_kg = baseMass_kg + g_massAdaptationOffset_kg;
        return;
    }

    const float M_MAX_KG = 2.5f;              // Maximum allowed virtual mass during oscillation (kg)
    const float M_INCREASE_RATE_KG_S = 15.0f; // How fast mass increases when unstable
    const float M_DECREASE_RATE_KG_S = 3.0f;  // How fast mass recovers when near endstop

    // 1. Ramp up mass during oscillation
    if (isOscillating || oscillationIntensity_01 > 0.15f) {
        float intensity = isOscillating ? 1.0f : oscillationIntensity_01;
        g_massAdaptationOffset_kg += M_INCREASE_RATE_KG_S * intensity * dt_s;
    } 
    // 2. Reduce mass ONLY when the pedal is safely near an endstop (< 5% or > 95%)
    else if (actualPosFraction_01 < 0.05f || actualPosFraction_01 > 0.95f) {
        g_massAdaptationOffset_kg -= M_DECREASE_RATE_KG_S * dt_s;
    }

    // Clamp values safely
    if (g_massAdaptationOffset_kg < 0.0f) {
        g_massAdaptationOffset_kg = 0.0f;
    }
    if ((baseMass_kg + g_massAdaptationOffset_kg) > M_MAX_KG) {
        g_massAdaptationOffset_kg = M_MAX_KG - baseMass_kg;
    }

    virtualMass_kg = baseMass_kg + g_massAdaptationOffset_kg;
}

/**
 * @brief Calculates active damping including AOM Boost, Trajectory Shaping, and Elastomer Hysteresis.
 */
static inline float CalcActiveDamping(
    float dampingRatio_zeta, float virtualMass_kg, float currentStiffness_N_m,
    float oscillationIntensity_01, float vModelPos_01, float actualPosFraction_01,
    int32_t actualServoTrackingError_i32, float travelSteps_cnt, float effectForceOffset_fl32,
    uint8_t dampingProgression_u8, float springForce_N, float vModelVel_mps, uint8_t elastomerModelSelection, float maxPedalForce_kg) 
{
    // Calculate Base Damping based on mass and current stiffness: c_c = 2 * sqrt(m * k)
    float criticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
    float baseDamping_Ns_m = dampingRatio_zeta * criticalDamping_Ns_m;
    
    float dampingMultiplier = 1.0f;

    // Adaptive damping (AOM & Tracking Error) only when no effect is applied
    if (effectForceOffset_fl32 == 0.0f) 
    {
        // AOM BOOST: If oscillation is detected, inject massive damping to freeze the system and bleed energy.
        dampingMultiplier = 1.0f + (oscillationIntensity_01 * 8.0f);

        // =========================================================
        // Tracking-Error dependent damping (Trajectory Shaping) to reduce EMF spikes. 
        // It was observed that voltage spikes occur at tracking error zero crossings. 
        // It was assumed that the servo could not keep up with its target position, then overshoots, 
        // producing large EMF. To reduce the lag, we increase the virtual damping so the servo can keep up.
        // =========================================================
        // actualPosFraction_01 is the physical position, vModelPos_01 is the target model position.
        float trackingError_01 = fabsf(vModelPos_01 - actualPosFraction_01);
        if (travelSteps_cnt > 0.0001f) {
            trackingError_01 = fabsf((float)actualServoTrackingError_i32 / travelSteps_cnt);
        }

        // If tracking errors exceed 0.5%, the models damping is dynamically increased proportional
        // so that the servo can catch up.
        if (trackingError_01 > 0.005f) {
            // Tuning: the multiplier (here 20.0f) determines how much the model decelerates.
            dampingMultiplier += (trackingError_01 * 20.0f); 
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
            float reboundFactor = (vModelVel_mps < 0.0f) ? 0.7f : 1.0f;

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
 * @brief Predictive EMF Reduction (Regenerative Power Clamping)
 */
static inline void ApplyRegenPowerClamping(
    float virtualMass_kg, float vModelVel_mps, float& acceleration_mps2)
{
    // If acceleration and velocity have opposite signs, the system is braking (Generator Mode).
    if ((acceleration_mps2 > 0.0f && vModelVel_mps < 0.0f) || 
        (acceleration_mps2 < 0.0f && vModelVel_mps > 0.0f)) {
        
        // Mechanical braking power P = |m * a * v| (in Watts)
        float predictedRegenPower_W = fabsf((virtualMass_kg * acceleration_mps2) * vModelVel_mps);
        
        // Max. allowed regenerative power (Tuning-Parameter! Default 1.0W)
        const float MAX_REGEN_POWER_W = 1.0f; 

        if (predictedRegenPower_W > MAX_REGEN_POWER_W) {
            // Clamp acceleration to softly cut off the power peak
            float powerScale = MAX_REGEN_POWER_W / predictedRegenPower_W;
            acceleration_mps2 *= powerScale;
        }
    }
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
 * @param rudderOffsets_st Rudder specific offset parameters.
 * @param oscillationDetectionLevel_fl32 Level of oscillation detected (0.0 to 1.0).
 * @param debugState_st Optional pointer to a struct to output internal variables for debugging. Default is nullptr.
 * @return int32_t The next absolute target position in steps for the stepper motor driver.
 */
int32_t IRAM_ATTR_FLAG MoveByAdmittanceStrategy(
  float loadCellReadingKg_fl32, 
  StepperWithLimits* stepper, 
  ForceCurveInterpolated* forceCurve, 
  const DapCalculationVariables_t* calc_st, 
  DapConfig_t* config_st, 
  EffectOffsets_t effectOffsets_st, 
  EndstopBehavior_t endstopBehavior_st, 
  RudderOffsets_t rudderOffsets_st,
  float oscillationDetectionLevel_fl32,
  AdmittanceDebugState_t* debugState_st = nullptr)
{

  

  // Time step for integration (seconds), but use actual loop time for better performance and stability.
  // But make sure to protect against wrapsound of the timer by using uint64_t and checking for large dt values.
  /*uint64_t currentTimeUs = esp_timer_get_time();
  static uint64_t lastTimeUs = 0;
  if (lastTimeUs == 0) {
    lastTimeUs = currentTimeUs;
  }
  float dt_s = ((float)currentTimeUs - (float)lastTimeUs) * 1e-6f;
  if (dt_s > 1.0f || dt_s <= 0.0f) {
    // If the time step is too large (e.g., due to timer wraparound or long loop delay), reset it to a default value to prevent instability.
    dt_s = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
  }
  lastTimeUs = currentTimeUs;
  */


  // --- 1. PHYSICAL PARAMETERS & CONFIGURATION ---
  // Time step for integration (seconds). We use a constant interval for improved numerical stability.
  float dt_s = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
  const float GRAVITY_N_KG = 9.81f; // Conversion constant for Kg to Newtons

  // Convert virtual mass and damping from user configuration percentages
  float virtualMass_kg = ((float)config_st->payloadPedalConfig_st.virtualPedalMassInPercent_u8) * 0.01f;
  float dampingRatio_zeta = ((float)config_st->payloadPedalConfig_st.virtualPedalDampingInPercent_u8) * 0.01f;
  
  // Constrain parameters to safe operational ranges
  virtualMass_kg = constrain(virtualMass_kg, 0.2f, 5.0f);
  dampingRatio_zeta = constrain(dampingRatio_zeta, 0.5f, 5.0f); 



  // --- 2. RUDDER CONFIG ---
  float rudderForce_N = 0.0f;
  if (rudderOffsets_st.isRudderMode) 
  { 
    float rudderCenter = rudderOffsets_st.centerPosition_01 + rudderOffsets_st.trimOffset_01;

    float rudderForceRaw_kg = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(rudderCenter, 0.0f, 1.0f));
    rudderForce_N = rudderForceRaw_kg * GRAVITY_N_KG;

  }


  // --- 3. OSCILLATION DETECTION (Active Oscillation Mitigation - AOM) ---
  // The oscillation detection level is computed in the main loop and passed as a parameter.
  // It is a value between 0.0 (no oscillation) and 1.0 (heavy oscillation detected) that indicates the current intensity of oscillations in the system.
  float g_oscillationIntensity_01 = constrain(oscillationDetectionLevel_fl32, 0.0f, 1.0f);

  // --- 4. PHYSICAL GEOMETRY ---
  // travelSteps_cnt: total steps from min to max soft endstop
  float travelSteps_cnt = (float)(calc_st->softEndstopMaxStepperPos_i32 - calc_st->softEndstopMinStepperPos_i32);
  
  // totalTravel_m: physical length of the pedal stroke in meters
  float motorRevolutionsPerSteps_lcl_fl32 = 1.0f / dap_calculationVariables_st.stepsPerMotorRevolution_u32;
  float totalTravel_m = travelSteps_cnt * motorRevolutionsPerSteps_lcl_fl32 * config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 * 0.001f;
  
  // actualPosFraction_01: The current command position of the ESP stepper [0.0, 1.0]
  float actualPosFraction_01 = stepper->getCurrentPositionFraction();

  // --- 5. ELASTOMER PHYSICS & SPRING REACTION (Hunt-Crossley Model) ---
  float displacement_01 = constrain(g_vModelPos_01, 0.0f, 1.0f);

  // 1. Static force from the Cubic Spline (Non-linear stiffness)
  float springForceRaw_kg = forceCurve->EvalForceCubicSpline(config_st, calc_st, displacement_01);
  float staticSpringForce_N = springForceRaw_kg * GRAVITY_N_KG;

  // Subtract rudder spring force (if active)
  if (rudderOffsets_st.isRudderMode) {
      staticSpringForce_N -= rudderForce_N;
  }

  // The spring force is purely static from the spline now (hysteresis is handled via damping)
  float springForce_N = staticSpringForce_N;

  // Ensure springForce_N is not negative just in case rudder offsets push it below 0
  if (springForce_N < 0.0f) {
      springForce_N = 0.0f;
  }
  // =========================================================

  // Calculate local physical spring stiffness (N/m) for dynamic damping tuning (AOM and Tustin).
  // We strictly use the gradient of the static spline here to ensure controller stability.
  float localStiffness_kg_step = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, displacement_01, false);
  float localStiffness_N_m = max(localStiffness_kg_step * (travelSteps_cnt / max(totalTravel_m, 0.0001f)) * GRAVITY_N_KG, 1.0f);

  // =========================================================
  // 3. VISCOELASTIC ELASTOMER HYSTERESIS (Hunt-Crossley model)
  // =========================================================
  // Normalize the GUI parameter from [0, 100] to [0.0, 1.0].
  // We square it (ratio * ratio) so the slider provides much finer control 
  // at the lower end, where small damping changes are felt the most.
  float progressionRatio = (float)constrain( config_st->payloadPedalConfig_st.dampingProgression_u8, 0, 100) ;
  float progression_01 = progressionRatio * progressionRatio; 

  // Calculate the critical damping of the system at this exact position: C_c = 2 * sqrt(m * k)
  // This brilliantly scales the rubber friction to the user's chosen mass and local spline stiffness!
  float localCriticalDamping_Ns_m = 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);

  // Tuning parameter: How many times the critical damping can the elastomer add 
  // at 100% slider value and full pedal compression?
  // 4.0x is a great sweet spot for simulating extremely heavy, dense elastomers.
  const float MAX_ELASTOMER_MULTIPLIER = 4.0f; 

  // Final elastomer viscosity coefficient
  float ELASTOMER_VISCOSITY_COEFFICIENT = progression_01 * (MAX_ELASTOMER_MULTIPLIER * localCriticalDamping_Ns_m);

  // Calculate equivalent damping: C_eq = C_elastomer * x
  // This will be perfectly integrated by the Tustin solver later.
  float elastomerDamping_Ns_m = ELASTOMER_VISCOSITY_COEFFICIENT * displacement_01;
  // =========================================================

  // --- 6. EFFECT OFFSETS & TOTAL FORCE ---
  // convert position offset to force offset using the local stiffness at the current position on the force curve
  float effectPositionToForceConversion_kg = effectOffsets_st.forceOffset_Steps_fl32 * localStiffness_kg_step;
  float effectForceOffset_fl32 = effectOffsets_st.forceOffset_kg_fl32;// + effectPositionToForceConversion_kg;
  float externalForce_N = (loadCellReadingKg_fl32 + effectForceOffset_fl32) * GRAVITY_N_KG;

  // --- 7. DYNAMIC TRAVEL LIMITS ---
  float lowerTravelLimit_01 = 0.0f;
  float upperTravelLimit_01 = 1.0f;
  CalcDynamicTravelLimits(travelSteps_cnt, localStiffness_kg_step, effectOffsets_st, lowerTravelLimit_01, upperTravelLimit_01);

  // --- 8. SOFT ENDSTOP CALCULATION ---
  float currentStiffness_N_m = localStiffness_N_m;
  float softEndstopForce_N = CalcSoftEndstopForce(g_vModelPos_01, totalTravel_m, endstopBehavior_st, currentStiffness_N_m, upperTravelLimit_01);

  // --- 9. ADMITTANCE OSCILLATION DETECTOR (Landi et al.) ---
  // We calculate base damping with un-adapted mass for the physics ideal-model reference
  float idealBaseDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
  
  // Use the exact restoring force (spline + endstop) instead of linear stiffness assumption
  float totalSpringReaction_N = springForce_N + softEndstopForce_N;
  
  bool hasActiveEffect = (effectOffsets_st.forceOffset_kg_fl32 != 0.0f) && (effectOffsets_st.forceOffset_Steps_fl32 != 0);

  // Call the detector with max force from config to calculate dynamic threshold
  bool isOscillating = DetectAdmittanceOscillation(
      externalForce_N, actualPosFraction_01, totalTravel_m, 
      totalSpringReaction_N, idealBaseDamping_Ns_m, virtualMass_kg, 
      dt_s, config_st->payloadPedalConfig_st.maxForce_fl32, debugState_st, hasActiveEffect
  );

    // --- 10. PASSIVE PARAMETER ADAPTATION (Position Gated) ---
  AdaptVirtualMass(isOscillating
    , g_oscillationIntensity_01
    , dt_s
    , virtualMass_kg
    , virtualMass_kg
    , hasActiveEffect
    , actualPosFraction_01);

  // --- 11. DYNAMIC ADAPTIVE DAMPING ---
  // (Re-calculate active damping with the new adapted mass)
  float activeDamping_Ns_m = CalcActiveDamping(dampingRatio_zeta
    , virtualMass_kg
    , currentStiffness_N_m
    , g_oscillationIntensity_01
    , g_vModelPos_01
    , actualPosFraction_01
    , stepper->getServosPosError()
    , travelSteps_cnt
    , effectForceOffset_fl32
    , config_st->payloadPedalConfig_st.dampingProgression_u8
    , springForce_N
    , g_vModelVel_mps
    , ELASTOMER_MODEL_HUNT_CROSSLEY//ELASTOMER_MODEL_STRESS_PROPORTIONAL // Defaulting to the new advanced model
    , config_st->payloadPedalConfig_st.maxForce_fl32); 

  g_lastActiveDamping_Ns_m = activeDamping_Ns_m;

  // --- 12. INTEGRATION (MASS-SPRING-DAMPER-ENDSTOP) ---
  // F_net = F_human - F_spring - F_softEndstop - F_damping
  float dampingForce_N = activeDamping_Ns_m * g_vModelVel_mps;
  float netForce_N = externalForce_N - springForce_N - softEndstopForce_N - dampingForce_N;
  
  // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
  const float FRICTION_N = 3.0f;
  if (fabsf(g_vModelVel_mps) > 0.001f) {
      netForce_N -= (g_vModelVel_mps > 0 ? FRICTION_N : -FRICTION_N);
  }

  // =========================================================
  // Integration approaches (Start)
  // =========================================================
  // Update virtual acceleration
  float acceleration_mps2 = 0.0f;
  
  // 0 = Explicit Euler (Legacy)
  // 1 = Implicit Backward Euler
  // 2 = Tustin (Bilinear Transform)
  int integration_method = 2; 
  
  switch(integration_method)
  {
    case 1: // IMPLICIT EULER
    {
      // We calculate the net force WITHOUT damping. 
      // Damping will be handled implicitly in the algebraic derivation below.
      float netForce_without_damping_N = externalForce_N - springForce_N - softEndstopForce_N;

      // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
      if (fabsf(g_vModelVel_mps) > 0.001f) {
          netForce_without_damping_N -= (g_vModelVel_mps > 0 ? FRICTION_N : -FRICTION_N);
      }

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

    case 2: // TUSTIN (BILINEAR TRANSFORM)
    {
      // We calculate the net force WITHOUT damping. 
      // Damping will be mathematically modeled perfectly within the IIR filter.
      float netForce_without_damping_N = externalForce_N - springForce_N - softEndstopForce_N;

      // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
      if (fabsf(g_vModelVel_mps) > 0.001f) {
          netForce_without_damping_N -= (g_vModelVel_mps > 0 ? FRICTION_N : -FRICTION_N);
      }

      // =========================================================
      // TUSTIN (BILINEAR TRANSFORM) MATHEMATICS
      // =========================================================
      // Derivation of the discrete IIR Filter for a Mass-Damper system:
      // We treat the spring as an external force to keep the non-linear
      // splines intact. Our continuous system is: M*a + C*v = F_netto
      //
      // 1. Laplace Transform (S-Domain) to find the transfer function:
      //    M * s * V(s) + C * V(s) = F(s)
      //    H(s) = V(s) / F(s) = 1 / (M * s + C)
      //
      // 2. Tustin Substitution (Mapping S-Domain to Z-Domain):
      //    s ≈ (2 / dt) * (1 - z^-1) / (1 + z^-1)
      //    Let c = 2 / dt.
      //
      // 3. Substitute 's' into H(s):
      //    H(z) = 1 / ( M * c * [(1 - z^-1)/(1 + z^-1)] + C )
      //    Multiply numerator and denominator by (1 + z^-1):
      //    H(z) = (1 + z^-1) / ( M * c * (1 - z^-1) + C * (1 + z^-1) )
      //
      // 4. Group by z^-1 to form the difference equation denominator:
      //    H(z) = (1 + z^-1) / ( (M*c + C) + (C - M*c)*z^-1 )
      //
      // 5. Define IIR Filter Coefficients (a1, b0, b1):
      //    A0 = M*c + C
      //    A1 = C - M*c
      //    b0 = 1 / A0,  b1 = 1 / A0,  a1 = A1 / A0
      //
      // 6. Final Time-Domain Difference Equation:
      //    v_new = b0 * F_netto_new + b1 * F_netto_old - a1 * v_old
      // =========================================================

      // Static variable to store the previous force for the IIR filter (z^-1 delay)
      // Note: If you instantiate this class multiple times (e.g. clutch + brake), 
      // you should move this variable into the class header or calculation struct!
      static float g_lastNetForceTustin_N = 0.0f;

      // Calculate the Tustin constant 'c'
      float c_tustin = 2.0f / dt_s;
  
      // Calculate denominator terms A0 and A1
      float A0 = (virtualMass_kg * c_tustin) + activeDamping_Ns_m;
      float A1 = activeDamping_Ns_m - (virtualMass_kg * c_tustin);
  
      // Calculate final IIR filter coefficients
      // Safeguard against division by zero just in case
      float new_vModelVel_mps = g_vModelVel_mps;
      if (fabsf(A0) > 1e-5) 
      {
          float b0 = 1.0f / A0;
          float b1 = 1.0f / A0;
          float a1 = A1 / A0;

          // Compute the new ideal velocity using the difference equation
          new_vModelVel_mps = (b0 * netForce_without_damping_N) + 
                              (b1 * g_lastNetForceTustin_N) - 
                              (a1 * g_vModelVel_mps);
      }

      // Store the current net force for the next integration cycle (z^-1)
      g_lastNetForceTustin_N = netForce_without_damping_N;

      // Reconstruct the effective acceleration for this time step.
      // This is necessary so the existing limits (Regen clamping, hard max accel) 
      // can operate unmodified on the Tustin output.
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
  const float MAX_ACCEL_MPS2 = 50.0f; 
  acceleration_mps2 = constrain(acceleration_mps2, -MAX_ACCEL_MPS2, MAX_ACCEL_MPS2);

  // Predictive EMF Reduction (Regenerative Power Clamping)
  ApplyRegenPowerClamping(virtualMass_kg
    , g_vModelVel_mps
    , acceleration_mps2);

  // Velocity Integration
  g_vModelVel_mps += acceleration_mps2 * dt_s;

  // --- 13. VELOCITY CHOKING (STABILITY PROTECTION) ---
  // Limit the movement speed if the system becomes unstable.
  float velocityLimit_01 = 1.0f - (g_oscillationIntensity_01 * 0.7f); // Up to 70% speed reduction
  
  float maxPhysicalVel_mps = 0.8f; 
  if (calc_st->stepsPerMotorRevolution_u32 > 0) {
      maxPhysicalVel_mps = (float)MAXIMUM_STEPPER_SPEED_U32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 / (float)calc_st->stepsPerMotorRevolution_u32 * 0.001f;
  }
  float dynamicSpeedLimit = maxPhysicalVel_mps * velocityLimit_01;
  g_vModelVel_mps = constrain(g_vModelVel_mps, -dynamicSpeedLimit, dynamicSpeedLimit);

  // --- 14. POSITION INTEGRATION, BOUNDARY CONSTRAINTS & DRIFT CORRECTION ---
  // Update virtual position based on velocity
  float currentPos_m = (g_vModelPos_01 * totalTravel_m) + (g_vModelVel_mps * dt_s);
  g_vModelPos_01 = (totalTravel_m > 0.0001f) ? (currentPos_m / totalTravel_m) : 0.0f;

  // Apply boundary constraints (including dynamic expansion for effects and endstops)
  // Hard constraint at mechanical boundaries (inelastic collision)
  if (g_vModelPos_01 <= lowerTravelLimit_01) {
      g_vModelPos_01 = lowerTravelLimit_01;
      if (g_vModelVel_mps < 0.0f) g_vModelVel_mps = 0.0f;
  } else if (g_vModelPos_01 >= upperTravelLimit_01) {
      g_vModelPos_01 = upperTravelLimit_01;
      if (g_vModelVel_mps > 0.0f) g_vModelVel_mps = 0.0f;
  }
  
  // SOFT LEASH: Synchronize the virtual model with the actual stepper command position
  // to prevent divergence due to numerical drift without corrupting second-order dynamics.
  float divergence_01 = actualPosFraction_01 - g_vModelPos_01;
  const float LEASH_RATE = 1.5f;
  g_vModelPos_01 += divergence_01 * (LEASH_RATE * dt_s); // 0.05% correction per cycle. 

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

  // --- 15. STEP CONVERSION & OUTPUT ---
  // Convert normalized virtual position back to absolute stepper steps
  float targetPosSteps_fl32 = (g_vModelPos_01 * travelSteps_cnt) + (float)calc_st->softEndstopMinStepperPos_i32;

  // Bypass effect position offset
  targetPosSteps_fl32 += effectOffsets_st.forceOffset_Steps_fl32;

  // Final safety clamp to hard hardware limits
  int32_t finalTargetPos_i32 = (int32_t)floor(targetPosSteps_fl32);
  return (int32_t)constrain(finalTargetPos_i32, stepper->getHardEndstopMinPosition(), stepper->getHardEndstopMaxPosition());
}