#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"


// see https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_Basic/PID_Basic.ino
#include <QuickPID.h>

// Admittance Control Variables
float g_admittancePos_fl32 = 0.0f;
float g_admittanceVel_fl32 = 0.0f;

int32_t IRAM_ATTR_FLAG MoveByPidStrategy(float loadCellReadingKg_fl32, StepperWithLimits* stepper, ForceCurveInterpolated* forceCurve, const DapCalculationVariables_t* calc_st, DapConfig_t* config_st, float absForceOffset_fl32, float absPosOffset_fl32)
{
  // 1. Convert loadcell reading (human force) into normalized percentage [0, 1]
  float loadCellReadingKgClip_fl32 = constrain(loadCellReadingKg_fl32, calc_st->forceMin_fl32, calc_st->forceMax_fl32);
  float humanForceNorm_fl32 = 0.0f;
  if (calc_st->forceRange_fl32 > 0.001f) {
    humanForceNorm_fl32 = (loadCellReadingKgClip_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32;
  }
  
  // Add external force effects (ABS, etc.)
  float appliedForce_fl32 = humanForceNorm_fl32 - (absForceOffset_fl32 / max(calc_st->forceRange_fl32, 0.001f));

  // 2. Read the spring force (target force) from the spline based on current virtual position
  float springForceNorm_fl32 = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(g_admittancePos_fl32, 0.0f, 1.0f));
  if (calc_st->forceRange_fl32 > 0.001f) {
      springForceNorm_fl32 = (springForceNorm_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32;
  } else {
      springForceNorm_fl32 = 0.0f;
  }

    // 3. Admittance physics model (Mass-Spring-Damper)
  // Converting to SI Units (Meters, Newtons) to allow true physical parameterization.
  // 1 kg of force = ~9.81 N
  float humanForce_N = appliedForce_fl32 * calc_st->forceRange_fl32 * 9.81f;
  float springForce_N = springForceNorm_fl32 * calc_st->forceRange_fl32 * 9.81f;

  // Convert current virtual position [0, 1] to actual physical travel in meters
  // stepperPosMax_i32 - stepperPosMin_i32 is the total travel in steps.
  // motorRevolutionsPerSteps_fl32 = 1.0 / 3200
  // spindlePitch_mmPerRev_u8 is mm/rev
  float travelSteps = (float)(calc_st->stepperPosMax_i32 - calc_st->stepperPosMin_i32);
  float totalTravel_m = travelSteps * motorRevolutionsPerSteps_fl32 * config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 * 0.001f;
  
  float currentPos_m = g_admittancePos_fl32 * totalTravel_m;

  // Parameters can be exposed to config later.
  float virtualMass_kg = 0.05f; // e.g., 2.0 kg of virtual pedal mass

  // Calculate local physical spring stiffness (N/m) based on a small delta around current position
  float springForcePlus_N = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(g_admittancePos_fl32 + 0.01f, 0.0f, 1.0f));
  if (calc_st->forceRange_fl32 > 0.001f) {
      springForcePlus_N = (springForcePlus_N - calc_st->forceMin_fl32) / calc_st->forceRange_fl32 * calc_st->forceRange_fl32 * 9.81f;
  } else {
      springForcePlus_N = 0.0f;
  }
  float localStiffness_N_m = 0.0f;
  if (totalTravel_m > 0.0001f) {
      localStiffness_N_m = max((springForcePlus_N - springForce_N) / (0.01f * totalTravel_m), 1.0f);
  }

  // Calculate Critical Damping: c_c = 2 * sqrt(mass * stiffness)
  // We apply a parameterizable damping ratio (zeta).
  // zeta = 1.0 is critically damped (no oscillation, fastest settling without overshoot).
  float dampingRatio_zeta = 0.9f; 
  float virtualDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);

  float dt = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;

  // F_net = F_human - F_spring - F_damping = M * a
  // Velocity is tracked in m/s
  float netForce_N = humanForce_N - springForce_N - (virtualDamping_Ns_m * g_admittanceVel_fl32);
  float acceleration_m_s2 = netForce_N / virtualMass_kg;

  // 4. Integrate acceleration to get physical velocity
  g_admittanceVel_fl32 += acceleration_m_s2 * dt;

  // 5. Integrate physical velocity to get physical position
  currentPos_m += g_admittanceVel_fl32 * dt;

  // Map physical position back to normalized percentage
  if (totalTravel_m > 0.0001f) {
      g_admittancePos_fl32 = currentPos_m / totalTravel_m;
  } else {
      g_admittancePos_fl32 = 0.0f;
  }

  // 6. Hard constraint at mechanical boundaries (inelastic collision)
  if (g_admittancePos_fl32 <= 0.0f) {
      g_admittancePos_fl32 = 0.0f;
      if (g_admittanceVel_fl32 < 0.0f) g_admittanceVel_fl32 = 0.0f;
  } else if (g_admittancePos_fl32 >= 1.0f) {
      g_admittancePos_fl32 = 1.0f;
      if (g_admittanceVel_fl32 > 0.0f) g_admittanceVel_fl32 = 0.0f;
  }

  // 7. Convert normalized position to stepper integer steps
  float posStepperNew_fl32 = g_admittancePos_fl32 * (float)(calc_st->stepperPosMax_i32 - calc_st->stepperPosMin_i32);
  posStepperNew_fl32 += calc_st->stepperPosMin_i32;

  // apply position offsets (like ABS vibrations, bite points, etc)
  posStepperNew_fl32 -= (absPosOffset_fl32 * (float)(calc_st->stepperPosMax_i32 - calc_st->stepperPosMin_i32));

  int32_t posStepperNew_i32 = floor(posStepperNew_fl32);
  
  // clamp target position to range
  posStepperNew_i32 = (int32_t)constrain(posStepperNew_i32, calc_st->stepperPosMin_i32, calc_st->stepperPosMax_i32);

  return posStepperNew_i32;
}



// see https://pidtuner.com

#ifdef USES_ADS1220
  void measureStepResponse(StepperWithLimits* stepper, const DapCalculationVariables_t* calc_st, const DapConfig_t* config_st, const LoadCellAds1220* loadcell)
#else
  void measureStepResponse(StepperWithLimits* stepper, const DapCalculationVariables_t* calc_st, const DapConfig_t* config_st, const LoadCellAds1256* loadcell)
#endif
{

  int32_t currentPos = stepper->getCurrentPositionFromMin();
  int32_t minPos = currentPos - dap_calculationVariables_st.stepperPosRange_fl32 * 0.05f;
  int32_t maxPos = currentPos + dap_calculationVariables_st.stepperPosRange_fl32 * 0.05f;

  stepper->moveTo(minPos, true);

  ActiveSerial->println("======================================");
  ActiveSerial->println("Start system identification data:");

  unsigned long initialTime = micros();
  unsigned long t = micros();
  bool targetPosHasBeenSet_b = false;
  float loadcellReading_fl32;

  int32_t targetPos;

  for (uint32_t cycleIdx = 0; cycleIdx < 5; cycleIdx++)
  {
    // toogle target position
    if (cycleIdx % 2 == 0)
    {
      targetPos = maxPos;
    }
    else
    {
      targetPos = minPos;
    }

    targetPos = (int32_t)constrain(targetPos, dap_calculationVariables_st.stepperPosMin_i32, dap_calculationVariables_st.stepperPosMax_i32);

    // execute move to target position and meaure system response
    float currentPos_fl32;
    for (uint32_t sampleIdx_u32 = 0; sampleIdx_u32 < 2000; sampleIdx_u32++)
    {
      // get loadcell reading
      loadcellReading_fl32 = loadcell->readLoadcellWeightInKg();

      // update time
      t = micros() - initialTime;

      // after some time, set target position
      if (sampleIdx_u32 == 50)
      {
        stepper->moveTo(targetPos, false);
      }

      // get current position
      currentPos_fl32 = stepper->getCurrentPositionFraction();
      loadcellReading_fl32 = (loadcellReading_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32; 
  
    }
  }

  ActiveSerial->println("======================================");
  ActiveSerial->println("End system identification data");
}



