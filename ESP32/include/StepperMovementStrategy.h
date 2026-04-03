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


// --- Global Admittance & Stability Variables ---
// These variables maintain state across control cycles
float g_vModelPos_01 = 0.0f;    // Normalized virtual position [0.0 to 1.0]
float g_vModelVel_mps = 0.0f;   // Physical virtual velocity [meters/second]

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
  float oscillationDetectionLevel_fl32)
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


  // --- 5. DYNAMIC TRAVEL LIMITS (Including Effect Offsets) ---
  // High-frequency effects (like ABS) can push the pedal slightly beyond the soft limits.
  // We calculate the required limit expansion based on local stiffness.
  float additionalTravelSteps_StepOffset = effectOffsets_st.forceOffset_Steps_fl32;
  float additionalTravelSteps_ForceOffset = 0.0f;
  if (localStiffness_kg_step > 0.0001f) {
    additionalTravelSteps_ForceOffset = (effectOffsets_st.forceOffset_kg_fl32 / localStiffness_kg_step);
  }

  float lowerTravelLimit_01 = 0.0f;
  float upperTravelLimit_01 = 1.0f;
  
  if (travelSteps_cnt > 0.0f) {
    float ext_A = additionalTravelSteps_StepOffset / travelSteps_cnt;
    float ext_B = additionalTravelSteps_ForceOffset / travelSteps_cnt;
    
    lowerTravelLimit_01 = min(0.0f, min(ext_A, ext_B));
    upperTravelLimit_01 = 1.0f + max(0.0f, max(ext_A, ext_B));
  }


  // --- 6. SOFT ENDSTOP CALCULATION ---
  // Simulates a heavy spring pushing back once the travel range is exceeded
  float softEndstopForce_N = 0.0f;
  float currentStiffness_N_m = localStiffness_N_m;

  if (endstopBehavior_st.travelRange_mm_fl32 > 0.01f) {
    if (g_vModelPos_01 > 1.0f) {
      float softEndstopStiffness_N_m = endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 * 1000.0f;
      float deflection_m = (g_vModelPos_01 - 1.0f) * totalTravel_m;
      softEndstopForce_N = softEndstopStiffness_N_m * deflection_m;
      
      // Update local stiffness for damping calculation to prevent endstop bouncing
      currentStiffness_N_m = softEndstopStiffness_N_m;
    }
    // Expand upper limit to allow the virtual model to penetrate the soft endstop
    upperTravelLimit_01 += (endstopBehavior_st.travelRange_mm_fl32 / 1000.0f) / totalTravel_m;
  }


  // --- 7. DYNAMIC ADAPTIVE DAMPING ---
  // Calculate Base Damping based on mass and current stiffness: c_c = 2 * sqrt(m * k)
  float baseDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * currentStiffness_N_m);
  
  // AOM BOOST: If oscillation is detected, inject massive damping to freeze the system and bleed energy.
  float dampingMultiplier = 1.0f + (g_oscillationIntensity_01 * 8.0f);
  float activeDamping_Ns_m = baseDamping_Ns_m * dampingMultiplier;

  // convert position offset to force offset using the local stiffness at the current position on the force curve
  float effectPositionToForceConversion_kg = effectOffsets_st.forceOffset_Steps_fl32 * localStiffness_kg_step;
  float effectForceOffset_fl32 = effectOffsets_st.forceOffset_kg_fl32 + effectPositionToForceConversion_kg;

  // --- 8. INTEGRATION (MASS-SPRING-DAMPER-ENDSTOP) ---
  // F_net = F_human - F_spring - F_softEndstop - F_damping
  float externalForce_N = (loadCellReadingKg_fl32 + effectForceOffset_fl32) * GRAVITY_N_KG;
  float dampingForce_N = activeDamping_Ns_m * g_vModelVel_mps;
  float netForce_N = externalForce_N - springForce_N - softEndstopForce_N - dampingForce_N;
  
  // Minimal Coulomb Friction to prevent micro-hunting (jitter) around the rest position
  const float FRICTION_N = 3.0f;
  if (abs(g_vModelVel_mps) > 0.001f) {
      netForce_N -= (g_vModelVel_mps > 0 ? FRICTION_N : -FRICTION_N);
  }

  // Update virtual acceleration and velocity
  float acceleration_mps2 = netForce_N / virtualMass_kg;
  g_vModelVel_mps += acceleration_mps2 * dt_s;


  // --- 9. VELOCITY CHOKING (STABILITY PROTECTION) ---
  // Limit the movement speed if the system becomes unstable.
  float velocityLimit_01 = 1.0f - (g_oscillationIntensity_01 * 0.7f); // Up to 70% speed reduction
  
  float maxPhysicalVel_mps = 0.8f; 
  if (calc_st->stepsPerMotorRevolution_u32 > 0) {
      maxPhysicalVel_mps = (float)MAXIMUM_STEPPER_SPEED_U32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 / (float)calc_st->stepsPerMotorRevolution_u32 * 0.001f;
  }
  float dynamicSpeedLimit = maxPhysicalVel_mps * velocityLimit_01;
  g_vModelVel_mps = constrain(g_vModelVel_mps, -dynamicSpeedLimit, dynamicSpeedLimit);


  // --- 10. POSITION INTEGRATION, BOUNDARY CONSTRAINTS & DRIFT CORRECTION ---
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


  // --- 11. STEP CONVERSION & OUTPUT ---
  // Convert normalized virtual position back to absolute stepper steps
  float targetPosSteps_fl32 = (g_vModelPos_01 * travelSteps_cnt) + (float)calc_st->softEndstopMinStepperPos_i32;
  int32_t finalTargetPos_i32 = (int32_t)floor(targetPosSteps_fl32);
  
  // Final safety clamp to hard hardware limits
  return (int32_t)constrain(finalTargetPos_i32, stepper->getHardEndstopMinPosition(), stepper->getHardEndstopMaxPosition());
}
