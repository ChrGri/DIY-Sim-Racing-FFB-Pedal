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



