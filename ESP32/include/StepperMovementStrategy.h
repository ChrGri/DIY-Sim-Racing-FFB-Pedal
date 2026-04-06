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
  float positionOffset_Steps_fl32;
} RudderOffsets_t;

// =========================================================
// NEW: DEBUG & TELEMETRY STRUCT
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
#define PSI_BUFFER_SIZE 30
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
 * @brief Admittance Oscillation Detector (Landi et al.)
 * Compares theoretical admittance force with actual measured force to identify external disturbances.
 * Uses the exact non-linear spring and endstop reaction force to avoid bias errors.
 * Uses EMA filtering to suppress massive noise spikes from digital stepper derivation.
 * @param debugState_st Optional pointer to output internal physical states for telemetry.
 */
static inline bool DetectAdmittanceOscillation(
    float externalForce_N, float actualPosFraction_01, float totalTravel_m, 
    float totalSpringReaction_N, float baseDamping_Ns_m, float currentMass_kg, float dt_s,
    AdmittanceDebugState_t* debugState_st, float maxForceInKg_fl32)
{
    float physicalPos_m = actualPosFraction_01 * totalTravel_m;
    
    if (!g_isPsiInitialized) {
        g_prevPhysicalPos_m = physicalPos_m;
        g_filteredPhysicalVel_mps = 0.0f;
        g_filteredPhysicalAcc_mps2 = 0.0f;
        g_prevFilteredPhysicalVel_mps = 0.0f;
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

    if (dt_s < 0.0001f) {
        if (debugState_st != nullptr) debugState_st->admittancePsi_N = 0.0f;
        return false;
    }

    // 1. Raw Derivation
    float rawPhysicalVel_mps = (physicalPos_m - g_prevPhysicalPos_m) / dt_s;
    g_prevPhysicalPos_m = physicalPos_m;

    // 2. Low-Pass Filter Velocity (EMA, Alpha ~0.15 for smooth but responsive tracking)
    const float ALPHA_VEL = 0.15f; 
    g_filteredPhysicalVel_mps = (ALPHA_VEL * rawPhysicalVel_mps) + ((1.0f - ALPHA_VEL) * g_filteredPhysicalVel_mps);

    // 3. Raw Acceleration from Filtered Velocity
    float rawPhysicalAcc_mps2 = (g_filteredPhysicalVel_mps - g_prevFilteredPhysicalVel_mps) / dt_s;
    g_prevFilteredPhysicalVel_mps = g_filteredPhysicalVel_mps;

    // 4. Low-Pass Filter Acceleration (EMA, Alpha ~0.05 to suppress extreme derivation noise)
    const float ALPHA_ACC = 0.05f; 
    g_filteredPhysicalAcc_mps2 = (ALPHA_ACC * rawPhysicalAcc_mps2) + ((1.0f - ALPHA_ACC) * g_filteredPhysicalAcc_mps2);

    // Expected force based on nominal admittance model (Eq. 2)
    // If the system is stable, the measured external force should perfectly match the physics model.
    // We use totalSpringReaction_N instead of (Stiffness * Pos) to perfectly account for 
    // spline biases and soft endstop forces.
    float expectedForce_N = (currentMass_kg * g_filteredPhysicalAcc_mps2) + 
                            (baseDamping_Ns_m * g_filteredPhysicalVel_mps) + 
                            totalSpringReaction_N;
    
    // =========================================================
    // NEW: ENDSTOP MASKING (Detector Suppression)
    // The mechanical realities near the limits (bouncing against hard stops, 
    // extreme elastomer compression) deviate heavily from the ideal mathematical model.
    // To prevent the Energy Tank from draining due to false-positive limit-cycle 
    // detections, we force the expected force to equal the external force near the limits.
    // This effectively sets Psi to 0.0 in these zones.
    // =========================================================
    if (actualPosFraction_01 > 0.95f || actualPosFraction_01 < 0.05f) {
        expectedForce_N = externalForce_N;
    }
    
    float psi_raw = fabsf(externalForce_N - expectedForce_N);

    // Moving average to prevent false positives
    g_psiBuffer[g_psiBufferIdx] = psi_raw;
    g_psiBufferIdx = (g_psiBufferIdx + 1) % PSI_BUFFER_SIZE;
    
    float psi_sum = 0.0f;
    for (int i = 0; i < PSI_BUFFER_SIZE; i++) {
        psi_sum += g_psiBuffer[i];
    }
    float psi_smoothed = psi_sum / (float)PSI_BUFFER_SIZE;
    
    // Output the internal physical values for telemetry
    if (debugState_st != nullptr) {
        debugState_st->admittancePsi_N = psi_smoothed;
        debugState_st->expectedForce_N = expectedForce_N;
        debugState_st->physicalPos_m = physicalPos_m;
        debugState_st->physicalVel_mps = g_filteredPhysicalVel_mps;
        debugState_st->physicalAcc_mps2 = g_filteredPhysicalAcc_mps2;
    }

    // Threshold detection (e.g., 30 Newtons)
    const float GRAVITY_N_KG = 9.81f;
    const float EPSILON_THRESHOLD_N = maxForceInKg_fl32 * 2;// GRAVITY_N_KG / 2; // empirically determined
    return (psi_smoothed > EPSILON_THRESHOLD_N);
}

/**
 * @brief Energy Tank & Passive Parameter Adaptation (Landi et al.)
 * Safely increases virtual mass using dissipated energy to suppress oscillations.
 */
static inline void AdaptMassViaEnergyTank(
    bool isOscillating, float oscillationIntensity_01, float vModelVel_mps, float dt_s, 
    float baseMass_kg, float& virtualMass_kg)
{
    // Tuning Parameters for the Energy Tank
    const float T_MAX_J = 5.0f;               // Maximum energy the tank can store (Joules)
    const float T_DELTA_J = 0.1f;             // Minimum energy reserve required (Joules)
    const float M_MAX_KG = 2.5f;              // Maximum allowed virtual mass during oscillation (kg)
    const float M_INCREASE_RATE_KG_S = 15.0f; // How fast mass increases when unstable
    const float M_DECREASE_RATE_KG_S = 2.0f;  // Forgetting factor (recovery speed)

    // 1. Fill the tank with dissipated energy from the PREVIOUS frame's damping (P_D = D * v^2)
    float P_D_W = g_lastActiveDamping_Ns_m * (vModelVel_mps * vModelVel_mps);
    if (g_tankEnergy_J < T_MAX_J) {
        g_tankEnergy_J += P_D_W * dt_s;
        if (g_tankEnergy_J > T_MAX_J) g_tankEnergy_J = T_MAX_J;
    }

    // 2. Parameter Adaptation Logic
    if (isOscillating || oscillationIntensity_01 > 0.15f) {
        // Scale the mass increase. If triggered by Landi detector, use full rate, otherwise scale.
        float intensity = isOscillating ? 1.0f : oscillationIntensity_01;
        float desired_M_dot = M_INCREASE_RATE_KG_S * intensity; 
        
        // Power required to increase inertia (P_M = 0.5 * v^2 * M_dot)
        float P_M_required_W = 0.5f * (vModelVel_mps * vModelVel_mps) * desired_M_dot;
        float energy_required_J = P_M_required_W * dt_s;

        // Check passivity condition: Do we have enough energy in the tank?
        if ((g_tankEnergy_J - energy_required_J) >= T_DELTA_J && (baseMass_kg + g_massAdaptationOffset_kg) < M_MAX_KG) {
            g_tankEnergy_J -= energy_required_J;                  // Consume energy
            g_massAdaptationOffset_kg += desired_M_dot * dt_s;    // Inject virtual mass
        }
    } else {
        // System is stable: Recover back to base parameters (Forgetting factor)
        if (g_massAdaptationOffset_kg > 0.0f) {
            g_massAdaptationOffset_kg -= M_DECREASE_RATE_KG_S * dt_s;
            if (g_massAdaptationOffset_kg < 0.0f) g_massAdaptationOffset_kg = 0.0f;
        }
    }

    // Prevent adapted mass from exceeding maximum limit
    if ((baseMass_kg + g_massAdaptationOffset_kg) > M_MAX_KG) {
        g_massAdaptationOffset_kg = M_MAX_KG - baseMass_kg;
    }

    virtualMass_kg = baseMass_kg + g_massAdaptationOffset_kg;
}

/**
 * @brief Calculates active damping including AOM Boost and Tracking Error Trajectory Shaping
 */
static inline float CalcActiveDamping(
    float dampingRatio_zeta, float virtualMass_kg, float currentStiffness_N_m,
    float oscillationIntensity_01, float vModelPos_01, float actualPosFraction_01,
    int32_t actualServoTrackingError_i32, float travelSteps_cnt, float effectForceOffset_fl32)
{
    // Calculate Base Damping based on mass and current stiffness: c_c = 2 * sqrt(m * k)
    float baseDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
    
    // AOM BOOST: If oscillation is detected, inject massive damping to freeze the system and bleed energy.
    float dampingMultiplier = 1.0f + (oscillationIntensity_01 * 8.0f);

    // =========================================================
    // NEW: Tracking-Error dependent damping (Trajectory Shaping) to reduce EMF spikes. 
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
    if (effectForceOffset_fl32 == 0.0f) // disable dampening in case of applied effects
    {
        if (trackingError_01 > 0.005f) {
            // Tuning: the multiplier (here 20.0f) determines how much the model decelerates.
            dampingMultiplier += (trackingError_01 * 20.0f); 
        }
    }

    return baseDamping_Ns_m * dampingMultiplier;
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

  // --- 2. OSCILLATION DETECTION (Active Oscillation Mitigation - AOM) ---
  // The oscillation detection level is computed in the main loop and passed as a parameter.
  // It is a value between 0.0 (no oscillation) and 1.0 (heavy oscillation detected) that indicates the current intensity of oscillations in the system.
  float g_oscillationIntensity_01 = constrain(oscillationDetectionLevel_fl32, 0.0f, 1.0f);

  // --- 3. PHYSICAL GEOMETRY ---
  // travelSteps_cnt: total steps from min to max soft endstop
  float travelSteps_cnt = (float)(calc_st->softEndstopMaxStepperPos_i32 - calc_st->softEndstopMinStepperPos_i32);
  
  // totalTravel_m: physical length of the pedal stroke in meters
  float totalTravel_m = travelSteps_cnt * motorRevolutionsPerSteps_fl32 * config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 * 0.001f;
  
  // actualPosFraction_01: The current command position of the ESP stepper [0.0, 1.0]
  float actualPosFraction_01 = stepper->getCurrentPositionFraction();

  // --- 4. SPRING REACTION & LOCAL STIFFNESS ---
  // Read the spring force from the spline based on current virtual position (ground truth)
  float springForceRaw_kg = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(g_vModelPos_01, 0.0f, 1.0f));
  float springForce_N = springForceRaw_kg * GRAVITY_N_KG;

  // Calculate local physical spring stiffness (N/m) for dynamic damping tuning
  float localStiffness_kg_step = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, constrain(g_vModelPos_01, 0.0f, 1.0f), false);
  float localStiffness_N_m = max(localStiffness_kg_step * (travelSteps_cnt / max(totalTravel_m, 0.0001f)) * GRAVITY_N_KG, 1.0f);

  // --- 5. EFFECT OFFSETS & TOTAL FORCE ---
  // convert position offset to force offset using the local stiffness at the current position on the force curve
  float effectPositionToForceConversion_kg = effectOffsets_st.forceOffset_Steps_fl32 * localStiffness_kg_step;
  float effectForceOffset_fl32 = effectOffsets_st.forceOffset_kg_fl32 + effectPositionToForceConversion_kg;
  float externalForce_N = (loadCellReadingKg_fl32 + effectForceOffset_fl32) * GRAVITY_N_KG;

  // --- 6. DYNAMIC TRAVEL LIMITS ---
  float lowerTravelLimit_01 = 0.0f;
  float upperTravelLimit_01 = 1.0f;
  CalcDynamicTravelLimits(travelSteps_cnt, localStiffness_kg_step, effectOffsets_st, lowerTravelLimit_01, upperTravelLimit_01);

  // --- 7. SOFT ENDSTOP CALCULATION ---
  float currentStiffness_N_m = localStiffness_N_m;
  float softEndstopForce_N = CalcSoftEndstopForce(g_vModelPos_01, totalTravel_m, endstopBehavior_st, currentStiffness_N_m, upperTravelLimit_01);

  // --- 8. ADMITTANCE OSCILLATION DETECTOR (Landi et al.) ---
  // We calculate base damping with un-adapted mass for the physics ideal-model reference
  float idealBaseDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
  
  // Use the exact restoring force (spline + endstop) instead of linear stiffness assumption
  float totalSpringReaction_N = springForce_N + softEndstopForce_N;
  bool isOscillating = DetectAdmittanceOscillation(externalForce_N, actualPosFraction_01, totalTravel_m, totalSpringReaction_N, idealBaseDamping_Ns_m, virtualMass_kg, dt_s, debugState_st, config_st->payloadPedalConfig_st.maxForce_fl32);

  // --- 9. ENERGY TANK & PASSIVE PARAMETER ADAPTATION ---
  AdaptMassViaEnergyTank(isOscillating, g_oscillationIntensity_01, g_vModelVel_mps, dt_s, virtualMass_kg, virtualMass_kg);

  // --- 10. DYNAMIC ADAPTIVE DAMPING ---
  // (Re-calculate active damping with the new adapted mass)
  float activeDamping_Ns_m = CalcActiveDamping(dampingRatio_zeta, virtualMass_kg, currentStiffness_N_m, g_oscillationIntensity_01, g_vModelPos_01, actualPosFraction_01, stepper->getServosPosError(), travelSteps_cnt, effectForceOffset_fl32);
  g_lastActiveDamping_Ns_m = activeDamping_Ns_m;

  // --- 11. INTEGRATION (MASS-SPRING-DAMPER-ENDSTOP) ---
  // F_net = F_human - F_spring - F_softEndstop - F_damping
  float dampingForce_N = activeDamping_Ns_m * g_vModelVel_mps;
  float netForce_N = externalForce_N - springForce_N - softEndstopForce_N - dampingForce_N;
  
  // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
  const float FRICTION_N = 3.0f;
  if (fabsf(g_vModelVel_mps) > 0.001f) {
      netForce_N -= (g_vModelVel_mps > 0 ? FRICTION_N : -FRICTION_N);
  }

  // Update virtual acceleration
  float acceleration_mps2 = netForce_N / virtualMass_kg;

  // Hard Clamping of acceleration to prevent limit cycles (Hunting)
  const float MAX_ACCEL_MPS2 = 50.0f; 
  acceleration_mps2 = constrain(acceleration_mps2, -MAX_ACCEL_MPS2, MAX_ACCEL_MPS2);

  // Predictive EMF Reduction (Regenerative Power Clamping)
  ApplyRegenPowerClamping(virtualMass_kg, g_vModelVel_mps, acceleration_mps2);

  // Velocity Integration
  g_vModelVel_mps += acceleration_mps2 * dt_s;

  // --- 12. VELOCITY CHOKING (STABILITY PROTECTION) ---
  // Limit the movement speed if the system becomes unstable.
  float velocityLimit_01 = 1.0f - (g_oscillationIntensity_01 * 0.7f); // Up to 70% speed reduction
  
  float maxPhysicalVel_mps = 0.8f; 
  if (calc_st->stepsPerMotorRevolution_u32 > 0) {
      maxPhysicalVel_mps = (float)MAXIMUM_STEPPER_SPEED_U32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 / (float)calc_st->stepsPerMotorRevolution_u32 * 0.001f;
  }
  float dynamicSpeedLimit = maxPhysicalVel_mps * velocityLimit_01;
  g_vModelVel_mps = constrain(g_vModelVel_mps, -dynamicSpeedLimit, dynamicSpeedLimit);

  // --- 13. POSITION INTEGRATION, BOUNDARY CONSTRAINTS & DRIFT CORRECTION ---
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
  g_vModelPos_01 += divergence_01 * 0.0005f; // 0.05% correction per cycle. 

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

  // --- 14. STEP CONVERSION & OUTPUT ---
  // Convert normalized virtual position back to absolute stepper steps
  float targetPosSteps_fl32 = (g_vModelPos_01 * travelSteps_cnt) + (float)calc_st->softEndstopMinStepperPos_i32;
  int32_t finalTargetPos_i32 = (int32_t)floor(targetPosSteps_fl32);
  
  // Final safety clamp to hard hardware limits
  return (int32_t)constrain(finalTargetPos_i32, stepper->getHardEndstopMinPosition(), stepper->getHardEndstopMaxPosition());
}