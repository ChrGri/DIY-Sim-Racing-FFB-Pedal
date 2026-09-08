#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"
#include "StepperMovementStrategy.h"

// Dedicated Virtual Admittance States for Flight Dynamics / Rudder Mode
static float g_vRudderModelPos_01 =
    0.50f; // Start at physical center for rudder
static float g_vRudderModelVel_mps = 0.0f;
static float g_smoothedRudderEffectPos_m = 0.0f;
static float g_prevSmoothedRudderEffectPos_m = 0.0f;
static float g_smoothedRudderEffectVel_mps = 0.0f;
static float g_prevSmoothedRudderEffectVel_mps = 0.0f;
static float g_smoothedRudderEffectAcc_mps2 = 0.0f;

// Rudder dynamic setpoint and filtering states
static float s_activeCenterPos_01 = 0.50f;
static float s_filteredSyncForce_N = 0.0f;
static float s_filteredPilotForce_N = 0.0f;
static float s_smoothedSyncTargetPos_01 = 0.50f;
static bool s_wasRudderActive = false;
static bool s_prevBrakeStatus = false;
static float s_couplingBlendFactor = 1.0f; // 1.0 = rigid yaw coupling, 0.0 = decoupled toe braking

/**
 * @brief Resets all virtual admittance states and filters for rudder flight
 * mode. Call whenever rudder mode is disabled or cleared.
 */
inline void ResetRudderStrategyState() {
  s_wasRudderActive = false;
  s_prevBrakeStatus = false;
  s_couplingBlendFactor = 1.0f;
  g_vRudderModelPos_01 = 0.50f;
  g_vRudderModelVel_mps = 0.0f;
  s_activeCenterPos_01 = 0.50f;
  s_filteredSyncForce_N = 0.0f;
  s_filteredPilotForce_N = 0.0f;
  s_smoothedSyncTargetPos_01 = 0.50f;
  g_smoothedRudderEffectPos_m = 0.0f;
  g_prevSmoothedRudderEffectPos_m = 0.0f;
  g_smoothedRudderEffectVel_mps = 0.0f;
  g_prevSmoothedRudderEffectVel_mps = 0.0f;
  g_smoothedRudderEffectAcc_mps2 = 0.0f;
}

/**
 * @brief Dedicated Flight Dynamics & Rudder Admittance Control Strategy.
 *
 * Exclusively implements aviation rudder (fixed-wing airplane) and anti-torque
 * (helicopter) physics, completely isolated from sim racing pedal algorithms.
 *
 * Supports:
 * - Mode 1 (Fixed-Wing Airplane): Bipolar aerodynamic centering spring, dynamic
 * Q-feel scaling, linear/progressive/S-curve feel, zero notch around neutral,
 * dynamic trim offset.
 * - Mode 2 (Helicopter Anti-Torque): Pure non-centering position hold, Coulomb
 * friction clamping, hydraulic viscous damping, hover bias offset.
 * - Dual-Pedal Push-Pull Coupling: Anti-symmetric ESP-NOW synchronization (x_R
 * = 1.0 - x_L) with high opposing stiffness stopping common-mode dual-forward
 * pressing.
 *
 * @param loadCellReadingKg_fl32 Raw force measured on loadcell in kg.
 * @param stepper Pointer to StepperWithLimits interface.
 * @param calc_st Pointer to static calculation variables.
 * @param config_st Pointer to pedal configuration structure.
 * @param effectOffsets_st High-frequency tactical vibrations (RPM rumble, stall
 * buffet, ground roll).
 * @param endstopBehavior_st Soft endstop feel configuration.
 * @param rudderOffsets_st Flight rudder specific offset parameters.
 * @param debugState_st Optional pointer to debug state structure.
 * @param admittanceStates_pst Optional pointer to state recording struct.
 * @return float Absolute target position in steps for the stepper motor.
 */
float IRAM_ATTR_FLAG MoveByRudderStrategy(
    float loadCellReadingKg_fl32, StepperWithLimits *stepper,
    DapCalculationVariables_t *calc_st, DapConfig_t *config_st,
    EffectOffsets_t effectOffsets_st, EndstopBehavior_t endstopBehavior_st,
    RudderOffsets_t rudderOffsets_st,
    AdmittanceDebugState_t *debugState_st = nullptr,
    AdmittanceStates_t *admittanceStates_pst = nullptr) {
  // 1. Integration timestep (constant interval for maximum numerical stability)
  float dt_s = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
  const float GRAVITY_N_KG = 9.81f;

  // 2. Physical Parameters & Flight Feel Tuning
  float virtualMass_kg =
      ((float)config_st->payloadPedalConfig_st.virtualPedalMassInPercent_u8) /
      100.0f;
  if (virtualMass_kg < 0.2f) {
    virtualMass_kg = (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER)
                         ? 1.5f
                         : 1.0f;
  }
  float dampingRatio_zeta =
      (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) ? 2.5f : 1.4f;

  static uint8_t s_prevRudderMode = 255;
  if (!s_wasRudderActive || s_prevRudderMode != rudderOffsets_st.rudderMode_u8) {
    s_prevRudderMode = rudderOffsets_st.rudderMode_u8;
    float initialCenter_01 = constrain(rudderOffsets_st.centerPosition_01 +
                                           rudderOffsets_st.trimOffset_01,
                                       0.05f, 0.95f);
    float currentPhysPos_01 = 0.50f;
    if (stepper != nullptr) {
      currentPhysPos_01 = constrain(stepper->getCurrentPositionFraction(), 0.0f, 1.0f);
    }
    g_vRudderModelPos_01 = currentPhysPos_01;
    g_vRudderModelVel_mps = 0.0f;
    s_activeCenterPos_01 = initialCenter_01;
    s_filteredSyncForce_N = 0.0f;
    s_filteredPilotForce_N = 0.0f;
    s_smoothedSyncTargetPos_01 = initialCenter_01;
    s_wasRudderActive = true;
  }

  float targetCenter_01 = constrain(rudderOffsets_st.centerPosition_01 +
                                        rudderOffsets_st.trimOffset_01,
                                    0.05f, 0.95f);

  // Smooth slew towards trim setpoint (0.4/sec transition)
  const float CENTER_SLEW_RATE = 2.0f;
  float maxCenterStep = CENTER_SLEW_RATE * dt_s;
  float centerDelta = targetCenter_01 - s_activeCenterPos_01;
  if (fabsf(centerDelta) > 0.0001f) {
    float centerStep = constrain(centerDelta, -maxCenterStep, maxCenterStep);
    s_activeCenterPos_01 += centerStep;
    if (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) {
      // When hover bias slider is adjusted in Helicopter mode, smoothly shift
      // pedal position
      g_vRudderModelPos_01 =
          constrain(g_vRudderModelPos_01 + centerStep, 0.05f, 0.95f);
    }
  }

  // Filtered remote sync force from opposite pedal (15ms tau eliminates loadcell micro-vibrations)
  float rawSyncForce_N = calc_st->syncPedalForce_N_fl32;
  float cleanSyncForce_N = 0.0f;
  if (fabsf(rawSyncForce_N) > 1.0f) {
    cleanSyncForce_N = (rawSyncForce_N > 0.0f) ? (rawSyncForce_N - 1.0f)
                                               : (rawSyncForce_N + 1.0f);
  }
  const float SYNC_FORCE_TAU = 0.015f; // 15ms filter: fast response without high-frequency buzzing
  float sync_alpha = 1.0f - expf(-dt_s / SYNC_FORCE_TAU);
  s_filteredSyncForce_N = (sync_alpha * cleanSyncForce_N) +
                          ((1.0f - sync_alpha) * s_filteredSyncForce_N);

  // Pilot Applied Force & Filtering (computed early for bilateral coupling decisions)
  float rawPilotForce_N = (loadCellReadingKg_fl32 * GRAVITY_N_KG);
  float cleanPilotForce_N =
      (rawPilotForce_N > 1.5f) ? (rawPilotForce_N - 1.5f) : 0.0f;

  const float PILOT_FORCE_TAU = 0.025f;
  float pilot_alpha = 1.0f - expf(-dt_s / PILOT_FORCE_TAU);
  s_filteredPilotForce_N = (pilot_alpha * cleanPilotForce_N) +
                           ((1.0f - pilot_alpha) * s_filteredPilotForce_N);
  calc_st->currentPedalForce_N_fl32 = s_filteredPilotForce_N;

  // Detect Toe Brake (Differential Braking) mode
  bool isToeBrakeMode = (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_TOE_BRAKE) ||
                        (calc_st != nullptr && calc_st->rudderBrakeStatus_b);

  if (isToeBrakeMode != s_prevBrakeStatus) {
    s_prevBrakeStatus = isToeBrakeMode;
    s_filteredSyncForce_N = 0.0f;
    s_smoothedSyncTargetPos_01 = g_vRudderModelPos_01;
  }

  // Continuous Blend Factor (250ms exponential crossfade eliminating mechanical jolt/kick)
  float targetBlend = isToeBrakeMode ? 0.0f : 1.0f;
  if (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_TOE_BRAKE) {
    s_couplingBlendFactor = 0.0f;
  } else {
    const float BLEND_TAU = 0.25f;
    float blend_alpha = 1.0f - expf(-dt_s / BLEND_TAU);
    s_couplingBlendFactor += blend_alpha * (targetBlend - s_couplingBlendFactor);
    s_couplingBlendFactor = constrain(s_couplingBlendFactor, 0.0f, 1.0f);
  }

  // 1. Direct opposing push-pull reaction force & real-time kinematic coupling
  float rudderPedalOpposingForce_N = 0.0f;
  float syncTrackingForce_N = 0.0f;
  float commonModeForce_N = 0.0f;

  if (s_couplingBlendFactor > 0.001f &&
      calc_st->syncPedalPositionRatio_fl32 >= 0.0f &&
      calc_st->syncPedalPositionRatio_fl32 <= 1.0f) {
    float rawSyncTargetPos_01 = 1.0f - calc_st->syncPedalPositionRatio_fl32;

    // Smooth EMA trajectory filter (14ms smoothing converts packet arrivals into an analog motion)
    const float SYNC_POS_TAU = 0.014f;
    float pos_alpha = 1.0f - expf(-dt_s / SYNC_POS_TAU);
    float desiredDelta_01 =
        pos_alpha * (rawSyncTargetPos_01 - s_smoothedSyncTargetPos_01);

    // Fast sync slew rate: maintains rigid coupling without phase lag
    float maxSyncSlew_01 =
        (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER)
            ? (8.0f * dt_s)
            : (12.0f * dt_s);
    desiredDelta_01 =
        constrain(desiredDelta_01, -maxSyncSlew_01, maxSyncSlew_01);
    s_smoothedSyncTargetPos_01 += desiredDelta_01;

    float posError = s_smoothedSyncTargetPos_01 - g_vRudderModelPos_01;

    // 1. Opposing push-pull reaction force (scaled smoothly by blend factor)
    rudderPedalOpposingForce_N = s_couplingBlendFactor * (-1.0f * s_filteredSyncForce_N);

    // 2. Rigid Bilateral Push-Pull Sync tracking gain (scaled smoothly by blend factor)
    float yawTracking_N = 250.0f * posError;
    syncTrackingForce_N = s_couplingBlendFactor * yawTracking_N;
    // Don't drive pedals deeper into physical limits to eliminate bilateral chatter at endstops:
    if (g_vRudderModelPos_01 <= 0.005f) {
      if (syncTrackingForce_N < 0.0f) syncTrackingForce_N = 0.0f;
      if (rudderPedalOpposingForce_N < 0.0f) rudderPedalOpposingForce_N = 0.0f;
    }
    if (g_vRudderModelPos_01 >= 0.995f) {
      if (syncTrackingForce_N > 0.0f) syncTrackingForce_N = 0.0f;
      if (rudderPedalOpposingForce_N > 0.0f) rudderPedalOpposingForce_N = 0.0f;
    }

    // 3. Smooth Common-Mode Lock on smoothed trajectory: Blocks pushing both pedals forward simultaneously in yaw mode
    float remoteSmoothedPos_01 = 1.0f - s_smoothedSyncTargetPos_01;
    float commonModeCompression_01 = (g_vRudderModelPos_01 + remoteSmoothedPos_01) - 1.0f;
    if (commonModeCompression_01 > 0.003f) {
      const float K_COMMON_LOCK_N = 1200.0f; // Immense rigid linkage barrier stiffness
      commonModeForce_N = s_couplingBlendFactor * (-K_COMMON_LOCK_N * (commonModeCompression_01 - 0.003f));
    }
  } else {
    // Mode 3 (Toe Brake) / Decoupled State: Zero out all bilateral connection forces
    rudderPedalOpposingForce_N = 0.0f;
    syncTrackingForce_N = 0.0f;
    commonModeForce_N = 0.0f;
    s_filteredSyncForce_N = 0.0f;
    s_smoothedSyncTargetPos_01 = g_vRudderModelPos_01;
  }

  // 4. Physical Geometry & Task-Space Conversion (Arc Length in Meters)
  float travelSteps_cnt = (float)(calc_st->softEndstopMaxStepperPos_i32 -
                                  calc_st->softEndstopMinStepperPos_i32);
  float motorRevolutionsPerSteps_lcl_fl32 =
      1.0f / (float)calc_st->stepsPerMotorRevolution_u32;
  float pitch_mm =
      (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;

  float minSledPos_mm = 0.0f;
  float maxSledPos_mm =
      travelSteps_cnt * motorRevolutionsPerSteps_lcl_fl32 * pitch_mm;

  float actualSledPosFraction_01 = stepper->getCurrentPositionFraction();
  float actualSledPos_mm = actualSledPosFraction_01 * maxSledPos_mm;

  float angleAtMinSled_deg = pedalInclineAngleDeg(minSledPos_mm, config_st);
  float angleAtMaxSled_deg = pedalInclineAngleDeg(maxSledPos_mm, config_st);
  float currentAngle_deg = pedalInclineAngleDeg(actualSledPos_mm, config_st);

  float leverArm_m =
      ((float)config_st->payloadPedalConfig_st.lengthPedalB_i16 +
       (float)config_st->payloadPedalConfig_st.lengthPedalD_i16) *
      0.001f;
  float totalTravel_m = fabsf(angleAtMaxSled_deg - angleAtMinSled_deg) *
                        DEG_TO_RAD_FL32 * leverArm_m;
  if (totalTravel_m < 0.001f)
    totalTravel_m = 0.05f;

  float actualPosFraction_01 = 0.5f;
  if (fabsf(angleAtMaxSled_deg - angleAtMinSled_deg) > 0.001f) {
    actualPosFraction_01 = (currentAngle_deg - angleAtMinSled_deg) /
                           (angleAtMaxSled_deg - angleAtMinSled_deg);
  }
  actualPosFraction_01 = constrain(actualPosFraction_01, 0.0f, 1.0f);

  // 5. Centering Spring Reaction & Damping Formulation
  float displacement_01 = constrain(g_vRudderModelPos_01, 0.0f, 1.0f);
  float springForce_N = 0.0f;
  float localStiffness_N_m = 10.0f;
  float localStiffness_kg_step = 0.01f;

  if (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) {
    // Mode 2: Helicopter Anti-Torque (Pure Friction / Position-Hold / Zero
    // Centering Spring)
    springForce_N = 0.0f;
    localStiffness_N_m = 15.0f;
    localStiffness_kg_step = 0.001f;
  } else {
    // Mode 0: Fixed-Wing Airplane (Symmetric Centering Spring) & Mode 2/3: Toe Brake
    float effectiveCenter_01 = s_activeCenterPos_01;
    float deltaPos = g_vRudderModelPos_01 - effectiveCenter_01;
    float deadzone = constrain(rudderOffsets_st.deadzone_01, 0.0f, 0.1f);

    float effectiveDelta = 0.0f;
    if (fabsf(deltaPos) > deadzone) {
      effectiveDelta =
          (deltaPos > 0.0f) ? (deltaPos - deadzone) : (deltaPos + deadzone);
    }

    float rudderMaxForceKg = config_st->payloadPedalConfig_st.maxForce_fl32;
    if (rudderMaxForceKg <= 0.5f)
      rudderMaxForceKg = 10.0f; // Default 10 kg aerodynamic resistance

    float centerForceKg = rudderOffsets_st.centerForce_kg;
    if (centerForceKg > rudderMaxForceKg) centerForceKg = rudderMaxForceKg;

    float halfTravel = max(0.5f - deadzone, 0.05f);
    float u = constrain(effectiveDelta / halfTravel, -1.0f, 1.0f);

    float absU = fabsf(u);
    float planeForceKg = (absU > 0.0001f) ? (centerForceKg + absU * (rudderMaxForceKg - centerForceKg)) : 0.0f;
    float brakeForceKg = absU * rudderMaxForceKg;
    float forceKg = (s_couplingBlendFactor * planeForceKg) +
                    ((1.0f - s_couplingBlendFactor) * brakeForceKg);

    springForce_N = (u > 0.0f ? 1.0f : (u < 0.0f ? -1.0f : 0.0f)) * forceKg * GRAVITY_N_KG;

    float gradStiffness_N_m =
        (rudderMaxForceKg * GRAVITY_N_KG) / max(0.5f * totalTravel_m, 0.001f);
    localStiffness_N_m = max(gradStiffness_N_m, 10.0f);
    localStiffness_kg_step =
        (rudderMaxForceKg / max(0.5f * travelSteps_cnt, 1.0f));
  }

  // 6. Tactical Environmental Effects Ingestion (RPM, Stall Buffet, Ground
  // Roll)
  float metersPerStep =
      (travelSteps_cnt > 0.0001f) ? (totalTravel_m / travelSteps_cnt) : 0.0f;
  float rawEffectPos_m =
      effectOffsets_st.forceOffset_Steps_fl32 * metersPerStep;

  const float EFFECT_TAU = 0.005f;
  float alpha_eff = 1.0f - expf(-dt_s / EFFECT_TAU);
  g_smoothedRudderEffectPos_m =
      (alpha_eff * rawEffectPos_m) +
      ((1.0f - alpha_eff) * g_smoothedRudderEffectPos_m);

  float rawEffectVel_mps =
      (g_smoothedRudderEffectPos_m - g_prevSmoothedRudderEffectPos_m) / dt_s;
  g_prevSmoothedRudderEffectPos_m = g_smoothedRudderEffectPos_m;
  g_smoothedRudderEffectVel_mps =
      (alpha_eff * rawEffectVel_mps) +
      ((1.0f - alpha_eff) * g_smoothedRudderEffectVel_mps);

  float rawEffectAcc_mps2 =
      (g_smoothedRudderEffectVel_mps - g_prevSmoothedRudderEffectVel_mps) /
      dt_s;
  g_prevSmoothedRudderEffectVel_mps = g_smoothedRudderEffectVel_mps;
  g_smoothedRudderEffectAcc_mps2 =
      (alpha_eff * rawEffectAcc_mps2) +
      ((1.0f - alpha_eff) * g_smoothedRudderEffectAcc_mps2);

  float idealBaseDamping_Ns_m =
      dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);
  float effectInjectedForce_N =
      (g_smoothedRudderEffectPos_m * localStiffness_N_m) +
      (g_smoothedRudderEffectVel_mps * idealBaseDamping_Ns_m) +
      (g_smoothedRudderEffectAcc_mps2 * virtualMass_kg);

  // 7. Pilot Applied Force & Filtering (already computed above for bilateral coupling decisions)


  // 8. Friction and Damping Forces
  float viscousDamping_Ns_m = idealBaseDamping_Ns_m;
  if (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) {
    // Mode 2: Helicopter hydraulic viscous damper feel (connected to UI Viscous
    // Damping slider)
    float dampingSlider =
        (float)config_st->payloadPedalConfig_st.virtualPedalDampingInPercent_u8;
    if (dampingSlider < 5.0f)
      dampingSlider = 45.0f; // Default 45% if unconfigured
    viscousDamping_Ns_m = 30.0f + (dampingSlider * 0.8f); // 34 to 110 N*s/m
  }

  // 9. Soft Endstops & Critical Barrier Damping (Eliminates violent chatter at limits)
  float lowerTravelLimit_01 = 0.0f;
  if (isToeBrakeMode && g_vRudderModelPos_01 >= (s_activeCenterPos_01 - 0.02f)) {
    lowerTravelLimit_01 = s_activeCenterPos_01;
  }
  float upperTravelLimit_01 = 1.0f;
  float softEndstopForce_N = 0.0f;
  float endstopDamping_Ns_m = 0.0f;

  float endstopStiffness_N_m =
      endstopBehavior_st.stiffnessAtMaxTravel_Npermm_fl32 * 1000.0f;
  if (endstopStiffness_N_m < 5000.0f) endstopStiffness_N_m = 10000.0f;
  float endstopCritDamping_Ns_m =
      2.0f * sqrtf(virtualMass_kg * endstopStiffness_N_m);

  float softEndstopTravel_m = (endstopBehavior_st.travelRange_mm_fl32 > 0.01f)
                                  ? (endstopBehavior_st.travelRange_mm_fl32 * 0.001f)
                                  : 0.0f;
  float softEndstopTravel_01 = (totalTravel_m > 0.001f) ? (softEndstopTravel_m / totalTravel_m) : 0.0f;

  if (softEndstopTravel_01 > 0.001f) {
    // Progressive cushion zone before hard limit
    float upperSoftThreshold_01 = upperTravelLimit_01 - softEndstopTravel_01;
    if (g_vRudderModelPos_01 > upperSoftThreshold_01) {
      float deflection_m = (g_vRudderModelPos_01 - upperSoftThreshold_01) * totalTravel_m;
      float penetration_01 = constrain(deflection_m / softEndstopTravel_m, 0.0f, 1.0f);
      softEndstopForce_N = (endstopStiffness_N_m * deflection_m) * (0.5f + 0.5f * penetration_01);
      endstopDamping_Ns_m = 2.0f * endstopCritDamping_Ns_m * penetration_01;
    }

    float lowerSoftThreshold_01 = lowerTravelLimit_01 + softEndstopTravel_01;
    if (g_vRudderModelPos_01 < lowerSoftThreshold_01) {
      float deflection_m = (lowerSoftThreshold_01 - g_vRudderModelPos_01) * totalTravel_m;
      float penetration_01 = constrain(deflection_m / softEndstopTravel_m, 0.0f, 1.0f);
      softEndstopForce_N = -1.0f * (endstopStiffness_N_m * deflection_m) * (0.5f + 0.5f * penetration_01);
      endstopDamping_Ns_m = 2.0f * endstopCritDamping_Ns_m * penetration_01;
    }
  }

  // Clamping at physical limits with inelastic boundary restitution (v=0)
  if (g_vRudderModelPos_01 >= upperTravelLimit_01) {
    g_vRudderModelPos_01 = upperTravelLimit_01;
    if (g_vRudderModelVel_mps > 0.0f) {
      g_vRudderModelVel_mps = 0.0f;
    }
  } else if (g_vRudderModelPos_01 <= lowerTravelLimit_01) {
    g_vRudderModelPos_01 = lowerTravelLimit_01;
    if (g_vRudderModelVel_mps < 0.0f) {
      g_vRudderModelVel_mps = 0.0f;
    }
  }

  float dampingForce_N =
      (viscousDamping_Ns_m + endstopDamping_Ns_m) * g_vRudderModelVel_mps;

  float coulombFriction_N =
      ((float)config_st->payloadPedalConfig_st.coulombFrictionIn0p1N_u8) * 0.1f;
  if (coulombFriction_N < 0.5f)
    coulombFriction_N = 1.5f;

  const float VELOCITY_EPSILON_MPS =
      (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) ? 0.015f
                                                                 : 0.005f;
  float frictionForce_N =
      coulombFriction_N * tanhf(g_vRudderModelVel_mps / VELOCITY_EPSILON_MPS);

  // 10. Net Acceleration & Semi-Implicit Euler Integration
  float totalExternalForce_N =
      (loadCellReadingKg_fl32 * GRAVITY_N_KG) +
      (effectOffsets_st.forceOffset_kg_fl32 * GRAVITY_N_KG) +
      effectInjectedForce_N + rudderPedalOpposingForce_N + syncTrackingForce_N +
      commonModeForce_N;

  float netForce_N = totalExternalForce_N - springForce_N - dampingForce_N -
                     frictionForce_N - softEndstopForce_N;

  // Inelastic boundary condition: normal reaction force prevents bouncing at hard endstops
  if (g_vRudderModelPos_01 >= upperTravelLimit_01 && netForce_N > 0.0f) {
    netForce_N = 0.0f;
    g_vRudderModelVel_mps = 0.0f;
  }
  if (g_vRudderModelPos_01 <= lowerTravelLimit_01 && netForce_N < 0.0f) {
    netForce_N = 0.0f;
    g_vRudderModelVel_mps = 0.0f;
  }

  float accel_mps2 = netForce_N / virtualMass_kg;

  g_vRudderModelVel_mps += accel_mps2 * dt_s;
  float maxPedalVel_mps =
      (rudderOffsets_st.rudderMode_u8 == RUDDER_MODE_HELICOPTER) ? 0.12f
                                                                 : 0.80f;
  g_vRudderModelVel_mps =
      constrain(g_vRudderModelVel_mps, -maxPedalVel_mps, maxPedalVel_mps);

  g_vRudderModelPos_01 += (g_vRudderModelVel_mps * dt_s) / totalTravel_m;

  // Strict physical clamp to active travel range
  if (g_vRudderModelPos_01 >= upperTravelLimit_01) {
    g_vRudderModelPos_01 = upperTravelLimit_01;
    if (g_vRudderModelVel_mps > 0.0f) g_vRudderModelVel_mps = 0.0f;
  } else if (g_vRudderModelPos_01 <= lowerTravelLimit_01) {
    g_vRudderModelPos_01 = lowerTravelLimit_01;
    if (g_vRudderModelVel_mps < 0.0f) g_vRudderModelVel_mps = 0.0f;
  }

  // 11. Target Stepper Position Output: Strictly clamped to soft endstops
  float targetStepPos_fl32 = (float)calc_st->softEndstopMinStepperPos_i32 +
                             (g_vRudderModelPos_01 * travelSteps_cnt);
  targetStepPos_fl32 = constrain(targetStepPos_fl32,
                                 (float)calc_st->softEndstopMinStepperPos_i32,
                                 (float)calc_st->softEndstopMaxStepperPos_i32);

  if (admittanceStates_pst != nullptr) {
    admittanceStates_pst->physicalPos_m = g_vRudderModelPos_01 * totalTravel_m;
    admittanceStates_pst->virtualVel_mps = g_vRudderModelVel_mps;
    admittanceStates_pst->virtualAcc_mps2 = accel_mps2;
  }

  return targetStepPos_fl32;
}
