#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"


// see https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_Basic/PID_Basic.ino
#include <QuickPID.h>

//Define Variables we'll be connecting to
float Setpoint, Input, Output;

//Specify the links and initial tuning parameters
float Kp=0.02f, Ki=0.5f, Kd=0.000f;
uint8_t control_strategy_u8 = 0;
QuickPID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd,  /* OPTIONS */
               QuickPID::pMode::pOnError,                   /* pOnError, pOnMeas, pOnErrorMeas */
               QuickPID::dMode::dOnMeas,                    /* dOnError, dOnMeas */
               QuickPID::iAwMode::iAwClamp,             /* iAwCondition, iAwClamp, iAwOff */
               QuickPID::Action::direct);                   /* direct, reverse */
bool pidWasInitialized = false;

#define PID_OUTPUT_LIMIT_FL32 0.5f


int32_t IRAM_ATTR_FLAG MoveByPidStrategy(float loadCellReadingKg, StepperWithLimits* stepper, ForceCurve_Interpolated* forceCurve, const DAP_calculationVariables_st* calc_st, DAP_config_st* config_st, float absForceOffset_fl32, float absPosOffset_fl32) {

  if (pidWasInitialized == false)
  {
    //turn the PID on
    // myPID.SetTunings(Kp, Ki, Kd);
    myPID.SetMode(QuickPID::Control::automatic);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwCondition);
    myPID.SetAntiWindupMode(QuickPID::iAwMode::iAwClamp);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwOff);

    pidWasInitialized = true;
    myPID.SetSampleTimeUs(REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US);
    myPID.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, PID_OUTPUT_LIMIT_FL32); // allow the PID to only change the position a certain amount per cycle
  }

  // get current position
  int32_t currentPosFromMinInSteps_i32 = stepper->getCurrentPositionFromMin();
  
  // apply offset
  int32_t currentPosWithOffset_i32 = currentPosFromMinInSteps_i32 + absPosOffset_fl32;
  
  float stepperPosFraction = stepper->getCurrentPositionFractionFromExternalPos(currentPosFromMinInSteps_i32);
  float stepperPosFractionWithForceOffset_fl32 = stepper->getCurrentPositionFractionFromExternalPos(currentPosWithOffset_i32);

  // clamp the stepper position to prevent problems with the spline 
  float stepperPosFraction_constrained = constrain(stepperPosFraction, 0.0f, 1.0f);
  float stepperPosFractionWithForceOffset_constrained = constrain(stepperPosFractionWithForceOffset_fl32, 0.0f, 1.0f);

  // constrain the output to the correct positioning interval to prevent PID windup 
  float neg_output_limit_fl32 = 1.0f - stepperPosFraction_constrained;
  float pos_output_limit_fl32 = stepperPosFraction_constrained;
  if (pos_output_limit_fl32 < PID_OUTPUT_LIMIT_FL32)
  {
    myPID.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, pos_output_limit_fl32);
  }
  else if (neg_output_limit_fl32 < PID_OUTPUT_LIMIT_FL32)
  {
    myPID.SetOutputLimits(-neg_output_limit_fl32, PID_OUTPUT_LIMIT_FL32);
  }
  else
  {
    myPID.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, PID_OUTPUT_LIMIT_FL32);
  }

  // read target force at spline position
  float loadCellTargetKg = forceCurve->EvalForceCubicSpline(config_st, calc_st, stepperPosFractionWithForceOffset_constrained);

  // apply effect force offset
  loadCellTargetKg -= absForceOffset_fl32;

  // clip to min & max force to prevent Ki to overflow
  float loadCellReadingKg_clip = constrain(loadCellReadingKg, calc_st->Force_Min, calc_st->Force_Max);
  float loadCellTargetKg_clip = constrain(loadCellTargetKg, calc_st->Force_Min, calc_st->Force_Max);


  
  // ToDO
  // - Min and Max force need to be identified from forceCurve->forceAtPosition() as they migh differ from calc_st.Force_Min & calc_st.Force_Max
  // - model predictive control, see e.g. https://www.researchgate.net/profile/Mohamed-Mourad-Lafifi/post/Model-Predictive-Control-examples/attachment/60202ac761fb570001029f61/AS%3A988637009301508%401612720839656/download/An+Introduction+to+Model-based+Predictive+Control+%28MPC%29.pdf
  //	https://www.youtube.com/watch?v=XaD8Lngfkzk
  //	https://github.com/pronenewbits/Arduino_Constrained_MPC_Library

  if (calc_st->Force_Range > 0.001f)
  {
      Input = ( loadCellReadingKg_clip - calc_st->Force_Min) / calc_st->Force_Range;
      Setpoint = ( loadCellTargetKg_clip - calc_st->Force_Min) / calc_st->Force_Range; 
  }
  else
  {
    Input = 0.0f;
    Input = 0;
    Setpoint= 0;
  }
  // compute PID output
  myPID.Compute();

  // integrate the position update
  // The setpoint comes from the force curve. The input comes from the loadcell. When the loadcell reading is below the force curve, the difference becomes positive. 
  // Thus, the stepper has to move towards the foot to increase the loadcell reading.
  // Since the QuickPID has some filtering applied on the input, both variables are changed for performance reasons.
  float posStepperNew_fl32 = stepperPosFraction - Output;
  posStepperNew_fl32 *= (float)(calc_st->stepperPosMax - calc_st->stepperPosMin);
  posStepperNew_fl32 += calc_st->stepperPosMin;

  // convert position to integer
  int32_t posStepperNew = floor(posStepperNew_fl32);
  
  // clamp target position to range
  posStepperNew=constrain(posStepperNew,calc_st->stepperPosMin,calc_st->stepperPosMax );
  //posStepperNew=constrain(posStepperNew,calc_st->stepperPosMinEndstop,calc_st->stepperPosMaxEndstop );

  return posStepperNew;
}



// see https://pidtuner.com

#ifdef USES_ADS1220
  void measureStepResponse(StepperWithLimits* stepper, const DAP_calculationVariables_st* calc_st, const DAP_config_st* config_st, const LoadCell_ADS1220* loadcell)
#else
  void measureStepResponse(StepperWithLimits* stepper, const DAP_calculationVariables_st* calc_st, const DAP_config_st* config_st, const LoadCell_ADS1256* loadcell)
#endif
{

  int32_t currentPos = stepper->getCurrentPositionFromMin();
  int32_t minPos = currentPos - dap_calculationVariables_st.stepperPosRange * 0.05f;
  int32_t maxPos = currentPos + dap_calculationVariables_st.stepperPosRange * 0.05f;

  stepper->moveTo(minPos, true);

  ActiveSerial->println("======================================");
  ActiveSerial->println("Start system identification data:");

  unsigned long initialTime = micros();
  unsigned long t = micros();
  bool targetPosHasBeenSet_b = false;
  float loadcellReading;

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

    targetPos = (int32_t)constrain(targetPos, dap_calculationVariables_st.stepperPosMin, dap_calculationVariables_st.stepperPosMax);

    // execute move to target position and meaure system response
    float currentPos;
    for (uint32_t sampleIdx_u32 = 0; sampleIdx_u32 < 2000; sampleIdx_u32++)
    {
      // get loadcell reading
      loadcellReading = loadcell->getReadingKg();

      // update time
      t = micros() - initialTime;

      // after some time, set target position
      if (sampleIdx_u32 == 50)
      {
        stepper->moveTo(targetPos, false);
      }

      // get current position
      currentPos = stepper->getCurrentPositionFraction();
      loadcellReading = (loadcellReading - calc_st->Force_Min) / calc_st->Force_Range; 
  
    }
  }

  ActiveSerial->println("======================================");
  ActiveSerial->println("End system identification data");
}



