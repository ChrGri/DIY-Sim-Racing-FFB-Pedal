#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"


// see https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_Basic/PID_Basic.ino
#include <QuickPID.h>



//Define Variables we'll be connecting to
float g_setpoint_fl32, g_input_fl32, g_output_fl32;

//Specify the links and initial tuning parameters
float g_kp_fl32 = 0.02f;
float g_ki_fl32 = 0.5f;
float g_kd_fl32 = 0.000f;
uint8_t g_controlStrategy_u8 = 0;
QuickPID g_pid(&g_input_fl32, &g_output_fl32, &g_setpoint_fl32, g_kp_fl32, g_ki_fl32, g_kd_fl32,  /* OPTIONS */
               QuickPID::pMode::pOnError,                   /* pOnError, pOnMeas, pOnErrorMeas */
               QuickPID::dMode::dOnMeas,                    /* dOnError, dOnMeas */
               QuickPID::iAwMode::iAwClamp,             /* iAwCondition, iAwClamp, iAwOff */
               QuickPID::Action::direct);                   /* direct, reverse */
bool g_pidWasInitialized_b = false;

#define PID_OUTPUT_LIMIT_FL32 0.5f


int32_t IRAM_ATTR_FLAG MoveByPidStrategy(float loadCellReadingKg_fl32, StepperWithLimits* stepper, ForceCurveInterpolated* forceCurve, const DapCalculationVariables_t* calc_st, DapConfig_t* config_st, float absForceOffset_fl32, float absPosOffset_fl32)
{

  if (g_pidWasInitialized_b == false)
  {
    //turn the PID on
    // myPID.SetTunings(Kp, Ki, Kd);
    g_pid.SetMode(QuickPID::Control::automatic);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwCondition);
    g_pid.SetAntiWindupMode(QuickPID::iAwMode::iAwClamp);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwOff);

    g_pidWasInitialized_b = true;
    g_pid.SetSampleTimeUs(REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64);
    g_pid.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, PID_OUTPUT_LIMIT_FL32); // allow the PID to only change the position a certain amount per cycle
  }

  // get current position
  int32_t currentPosFromMinInSteps_i32 = stepper->getCurrentPositionFromMin();
  
  // apply offset
  int32_t currentPosWithOffset_i32 = currentPosFromMinInSteps_i32 + absPosOffset_fl32;
  
  float stepperPosFraction_fl32 = stepper->getCurrentPositionFractionFromExternalPos(currentPosFromMinInSteps_i32);
  float stepperPosFractionWithForceOffset_fl32 = stepper->getCurrentPositionFractionFromExternalPos(currentPosWithOffset_i32);

  // clamp the stepper position to prevent problems with the spline 
  float stepperPosFractionConstrained_fl32 = constrain(stepperPosFraction_fl32, 0.0f, 1.0f);
  float stepperPosFractionWithForceOffset_constrained = constrain(stepperPosFractionWithForceOffset_fl32, 0.0f, 1.0f);

  // constrain the output to the correct positioning interval to prevent PID windup 
  float negOutputLimit_fl32 = 1.0f - stepperPosFractionConstrained_fl32;
  float posOutputLimit_fl32 = stepperPosFractionConstrained_fl32;
  if (posOutputLimit_fl32 < PID_OUTPUT_LIMIT_FL32)
  {
    g_pid.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, posOutputLimit_fl32);
  }
  else if (negOutputLimit_fl32 < PID_OUTPUT_LIMIT_FL32)
  {
    g_pid.SetOutputLimits(-negOutputLimit_fl32, PID_OUTPUT_LIMIT_FL32);
  }
  else
  {
    g_pid.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, PID_OUTPUT_LIMIT_FL32);
  }

  // read target force at spline position
  float loadCellTargetKg_fl32 = forceCurve->EvalForceCubicSpline(config_st, calc_st, stepperPosFractionWithForceOffset_constrained);

  // apply effect force offset
  loadCellTargetKg_fl32 -= absForceOffset_fl32;

  // clip to min & max force to prevent Ki to overflow
  float loadCellReadingKgClip_fl32 = constrain(loadCellReadingKg_fl32, calc_st->forceMin_fl32, calc_st->forceMax_fl32);
  float loadCellTargetKgClip_fl32 = constrain(loadCellTargetKg_fl32, calc_st->forceMin_fl32, calc_st->forceMax_fl32);


  
  // ToDO
  // - Min and Max force need to be identified from forceCurve->forceAtPosition() as they migh differ from calc_st.forceMin_fl32 & calc_st.forceMax_fl32
  // - model predictive control, see e.g. https://www.researchgate.net/profile/Mohamed-Mourad-Lafifi/post/Model-Predictive-Control-examples/attachment/60202ac761fb570001029f61/AS%3A988637009301508%401612720839656/download/An+Introduction+to+Model-based+Predictive+Control+%28MPC%29.pdf
  //	https://www.youtube.com/watch?v=XaD8Lngfkzk
  //	https://github.com/pronenewbits/Arduino_Constrained_MPC_Library

  if (calc_st->forceRange_fl32 > 0.001f)
  {
    g_input_fl32 = (loadCellReadingKgClip_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32;
    g_setpoint_fl32 = (loadCellTargetKgClip_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32;
  }
  else
  {
    g_input_fl32 = 0.0f;
    g_setpoint_fl32 = 0.0f;
  }

  // compute PID output
  g_pid.Compute();

  // integrate the position update
  // The setpoint comes from the force curve. The input comes from the loadcell. When the loadcell reading is below the force curve, the difference becomes positive. 
  // Thus, the stepper has to move towards the foot to increase the loadcell reading.
  // Since the QuickPID has some filtering applied on the input, both variables are changed for performance reasons.
  float posStepperNew_fl32 = stepperPosFraction_fl32 - g_output_fl32;
  posStepperNew_fl32 *= (float)(calc_st->stepperPosMax_i32 - calc_st->stepperPosMin_i32);
  posStepperNew_fl32 += calc_st->stepperPosMin_i32;

  // convert position to integer
  int32_t posStepperNew_i32 = floor(posStepperNew_fl32);
  
  // clamp target position to range
  posStepperNew_i32 = (int32_t)constrain(posStepperNew_i32, calc_st->stepperPosMin_i32, calc_st->stepperPosMax_i32);
  //posStepperNew_i32 = (int32_t)constrain(posStepperNew_i32, calc_st->stepperPosMinEndstop_i32, calc_st->stepperPosMaxEndstop_i32);

  return posStepperNew_i32;
}


// Admittance Control Variables
float g_admittancePos_fl32 = 0.0f;
float g_admittanceVel_fl32 = 0.0f;


int32_t IRAM_ATTR_FLAG MoveByAdmittanceStrategy(float loadCellReadingKg_fl32, StepperWithLimits* stepper, ForceCurveInterpolated* forceCurve, const DapCalculationVariables_t* calc_st, DapConfig_t* config_st, float absForceOffset_fl32, float absPosOffset_fl32)
{

  // ground truth position from servo can be obtained via stepper->getServosInternalPositionCorrected()
  // This position gives the true servo position, but the signal might lag 10ms behind, due to the communication delay. 
  //The stepper->getCurrentPositionFromMin() gives the position based on the step commands, which is more responsive but might deviate from the true position if there is step loss or other issues.


  // read current ESP position and convert to fraction of total travel
  int32_t currentPosFromMinInSteps_i32 = stepper->getCurrentPositionFromMin();

  // obtain servos output position from min
  int32_t servosPosFromMin_i32 = ( stepper->getServosInternalPositionCorrected() + stepper->getServosPosError() ) - stepper->getMinPosition(); 

  float actualPosFraction_fl32 = stepper->getCurrentPositionFraction(); 
  float actualServoPosFraction_fl32 = (float)servosPosFromMin_i32 / (float)(stepper->getTravelSteps());
  
  actualPosFraction_fl32 = actualServoPosFraction_fl32;

  
  // 1. Convert loadcell reading (human force) into normalized percentage [0, 1]
  float loadCellReadingKgClip_fl32 = constrain(loadCellReadingKg_fl32, calc_st->forceMin_fl32, calc_st->forceMax_fl32);
  float humanForceNorm_fl32 = 0.0f;
  if (calc_st->forceRange_fl32 > 0.001f) {
    humanForceNorm_fl32 = (loadCellReadingKgClip_fl32 - calc_st->forceMin_fl32) / calc_st->forceRange_fl32;
  }
  
  // Add external force effects (ABS, etc.)
  float appliedForce_fl32 = humanForceNorm_fl32 - (absForceOffset_fl32 / max(calc_st->forceRange_fl32, 0.001f));

  // 2. Read the spring force (target force) from the spline based on current virtual position
  
  float springForceNorm_fl32 = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(actualPosFraction_fl32, 0.0f, 1.0f));
  //float springForceNorm_fl32 = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(g_admittancePos_fl32, 0.0f, 1.0f));
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
  float virtualMass_kg = 0.2f; // e.g., 2.0 kg of virtual pedal mass

  // Calculate local physical spring stiffness (N/m) based on a small delta around current position
  float springForcePlus_N = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(actualPosFraction_fl32 + 0.01f, 0.0f, 1.0f));
  if (calc_st->forceRange_fl32 > 0.001f) {
      springForcePlus_N = (springForcePlus_N - calc_st->forceMin_fl32) / calc_st->forceRange_fl32 * calc_st->forceRange_fl32 * 9.81f;
  } else {
      springForcePlus_N = 0.0f;
  }
  float localStiffness_N_m = 0.0f;
  if (totalTravel_m > 0.0001f) {
      localStiffness_N_m = max((springForcePlus_N - springForce_N) / (0.01f * totalTravel_m), 1.0f);
  }

  // Alternative: directly compute stiffness from the gradient of the force curve
  //localStiffness_N_m = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, constrain(actualPosFraction_fl32, 0.0f, 1.0f), false);

  // Calculate Critical Damping: c_c = 2 * sqrt(mass * stiffness)
  // We apply a parameterizable damping ratio (zeta).
  // zeta = 1.0 is critically damped (no oscillation, fastest settling without overshoot).
  float dampingRatio_zeta = 1.5f;  
  float virtualDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);

  float dt = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;

  // F_net = F_human - F_spring - F_damping = M * a
  // Velocity is tracked in m/s
  float netForce_N = humanForce_N - springForce_N - (virtualDamping_Ns_m * g_admittanceVel_fl32);
  float acceleration_m_s2 = netForce_N / virtualMass_kg;

    // 4. Integrate acceleration to get physical velocity
  g_admittanceVel_fl32 += acceleration_m_s2 * dt;

  // Protect against mathematical blow-up and clamp velocity to hardware maximum limits
  // Calculate max physical velocity (m/s) achievable by stepper
  float maxPhysicalVel_m_s = 1.0f; 
  if (totalTravel_m > 0.0001f) {
      maxPhysicalVel_m_s = ((float)MAXIMUM_STEPPER_SPEED_U32) / travelSteps * totalTravel_m;
  }
  g_admittanceVel_fl32 = constrain(g_admittanceVel_fl32, -maxPhysicalVel_m_s, maxPhysicalVel_m_s);

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


  float physicalPos_fl32 = stepper->getCurrentPositionFraction();
  float positionError = g_admittancePos_fl32 - physicalPos_fl32;

  // If the virtual model is way ahead of the motor, pull the virtual model back 
  // to prevent a "jump" once the motor regains control.
  if (abs(positionError) > 0.05f) { 
      g_admittancePos_fl32 = physicalPos_fl32 + (positionError * 0.1f); // Simple low-pass sync
  }

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



