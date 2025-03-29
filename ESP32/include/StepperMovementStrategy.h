#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"


// see https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_Basic/PID_Basic.ino
#include <QuickPID.h>

//Define Variables we'll be connecting to
float Setpoint, Input, Output;

//Specify the links and initial tuning parameters
float Kp=0.3f, Ki=50.0f, Kd=0.000f;
uint8_t control_strategy_u8 = 0;
QuickPID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd,  /* OPTIONS */
               myPID.pMode::pOnError,                   /* pOnError, pOnMeas, pOnErrorMeas */
               myPID.dMode::dOnMeas,                    /* dOnError, dOnMeas */
               myPID.iAwMode::iAwOff,             /* iAwCondition, iAwClamp, iAwOff */
               myPID.Action::direct);                   /* direct, reverse */
bool pidWasInitialized = false;

#define PID_OUTPUT_LIMIT_FL32 0.5f

/**********************************************************************************************/
/*                                                                                            */
/*                         Movement strategy: spring stiffness                                */
/*                                                                                            */
/**********************************************************************************************/

int32_t MoveByInterpolatedStrategy(float filteredLoadReadingKg, float stepperPosFraction, ForceCurve_Interpolated* forceCurve, const DAP_calculationVariables_st* calc_st, const DAP_config_st* config_st) {
  float spingStiffnessInv_lcl = calc_st->springStiffnesssInv;
  //float springStiffnessInterp = forceCurve->stiffnessAtPosition(stepperPosFraction);
  float springStiffnessInterp = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, stepperPosFraction, false);

  
  if (springStiffnessInterp > 0) {
    spingStiffnessInv_lcl = (1.0f / springStiffnessInterp);
  }

  // caclulate pedal position
  float pedalForceInterp = forceCurve->EvalForceCubicSpline(config_st, calc_st, stepperPosFraction);
  float stepperPosInterp = (calc_st->stepperPosMax - calc_st->stepperPosMin) * stepperPosFraction;
  return spingStiffnessInv_lcl * (filteredLoadReadingKg - pedalForceInterp) + stepperPosInterp;
}




/**********************************************************************************************/
/*                                                                                            */
/*                         Movement strategy: PID                                             */
/*                                                                                            */
/**********************************************************************************************/

void tunePidValues(DAP_config_st& config_st)
{
  Kp=config_st.payLoadPedalConfig_.PID_p_gain;
  Ki=config_st.payLoadPedalConfig_.PID_i_gain;
  Kd=config_st.payLoadPedalConfig_.PID_d_gain;

  control_strategy_u8 = config_st.payLoadPedalConfig_.control_strategy_b;

  myPID.SetTunings(config_st.payLoadPedalConfig_.PID_p_gain, config_st.payLoadPedalConfig_.PID_i_gain, config_st.payLoadPedalConfig_.PID_d_gain);
}

int32_t MoveByPidStrategy(float loadCellReadingKg, float stepperPosFraction, StepperWithLimits* stepper, ForceCurve_Interpolated* forceCurve, const DAP_calculationVariables_st* calc_st, DAP_config_st* config_st, float absForceOffset_fl32, float changeVelocity) {

  if (pidWasInitialized == false)
  {
    //turn the PID on
    myPID.SetTunings(Kp, Ki, Kd);
    myPID.SetMode(myPID.Control::automatic);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwCondition);
    myPID.SetAntiWindupMode(myPID.iAwMode::iAwClamp);
    //myPID.SetAntiWindupMode(myPID.iAwMode::iAwOff);


    pidWasInitialized = true;
    myPID.SetSampleTimeUs(PUT_TARGET_CYCLE_TIME_IN_US);
    //myPID.SetOutputLimits(-1.0,0.0);
    myPID.SetOutputLimits(-PID_OUTPUT_LIMIT_FL32, PID_OUTPUT_LIMIT_FL32); // allow the PID to only change the position a certain amount per cycle

    myPID.SetTunings(config_st->payLoadPedalConfig_.PID_p_gain, config_st->payLoadPedalConfig_.PID_i_gain, config_st->payLoadPedalConfig_.PID_d_gain);
  }


  // clamp the stepper position to prevent problems with the spline 
  float stepperPosFraction_constrained = constrain(stepperPosFraction, 0.0f, 1.0f);

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
  float loadCellTargetKg = forceCurve->EvalForceCubicSpline(config_st, calc_st, stepperPosFraction_constrained);

  // apply effect force offset
  loadCellTargetKg -=absForceOffset_fl32;

  // clip to min & max force to prevent Ki to overflow
  float loadCellReadingKg_clip = constrain(loadCellReadingKg, calc_st->Force_Min, calc_st->Force_Max);
  float loadCellTargetKg_clip = constrain(loadCellTargetKg, calc_st->Force_Min, calc_st->Force_Max);


  // dynamically scale the PID values depending on the force curve gradient
  if (control_strategy_u8 == 1)
  {
    float gradient_orig_fl32 = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, stepperPosFraction_constrained, true); // determine gradient to modify the PID gain. The steeper the gradient, the less gain should be used

    // normalize gradient
    float gradient_fl32 = gradient_orig_fl32;
    float gradient_abs_fl32 = fabs(gradient_fl32);

    float gain_modifier_fl32 = 10.0f;
    if (gradient_abs_fl32 > 1e-5f)
    {
       gain_modifier_fl32 = 1.0f / pow( gradient_abs_fl32 , 1.0f);
    }
    gain_modifier_fl32 = constrain( gain_modifier_fl32, 0.1f, 10.0f);

    myPID.SetTunings(gain_modifier_fl32 * Kp, gain_modifier_fl32 * Ki, gain_modifier_fl32 * Kd);
  }
  else
  {
    myPID.SetTunings(Kp, Ki, Kd);
  }



 


  
  
  
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

  // add velocity feedforward
  posStepperNew_fl32 += changeVelocity * config_st->payLoadPedalConfig_.PID_velocity_feedforward_gain;

  // convert position to integer
  int32_t posStepperNew = floor(posStepperNew_fl32);
  
  // clamp target position to range
  posStepperNew=constrain(posStepperNew,calc_st->stepperPosMin,calc_st->stepperPosMax );

  //#define PLOT_PID_VALUES
  #ifdef PLOT_PID_VALUES
    static RTDebugOutput<float, 8> rtDebugFilter({ "stepperPosFraction", "loadCellTargetKg", "loadCellReadingKg", "loadCellReadingKg_clip", "Setpoint", "Input", "Output", "posStepperNew"});
    rtDebugFilter.offerData({ stepperPosFraction, loadCellTargetKg, loadCellReadingKg, loadCellReadingKg_clip, Setpoint, Input, Output, posStepperNew});       
  #endif
  

  return posStepperNew;
}

unsigned long lastTime = 0;
float output = 0.0f;       // PID-Ausgang
float filteredOutput = 0.0f;
bool expFilterHasBeenInitialized = false;
// Filter-Variablen
float RC = 1.0f / (2.0f * 3.14159f * 100.0f); // RC für 20 Hz Eckfrequenz

int printStep = 0;

float posArray[10] = {0.0f};
uint8_t arrayIndex = 0;
int32_t MoveByForceTargetingStrategy(float loadCellReadingKg, StepperWithLimits* stepper, ForceCurve_Interpolated* forceCurve, const DAP_calculationVariables_st* calc_st, DAP_config_st* config_st, float absForceOffset_fl32, float changeVelocity, float d_phi_d_x, float d_x_hor_d_phi) {
  // see https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/wiki/Movement-control-strategies#mpc


  /*
  This closed-loop control strategy models the foot as a spring with a certain stiffness k1.
  The force resulting from that model is F1 = k1 * x. 
  To find the servo target position:
  1) A line with the slope -k1 at the point of the loadcell reading & current position is drawn.
  2) The intersection with the force-travel curve gives the target position
  
  Since the force-travel curve might be non-linear, Newtons method is used to numerically find the intersection point.
  f(x_n) = -k1 * x + b - forceCurve(x) = 0
  x_n+1 = x_n - f(x_n) / f'(x_n)
  whereas x_n is the servo position at iteration n
  f(x_n) = -k1 * x + b - forceCurve(x)
  f'(x_n) = -k1 - d(forceCurve(x)) / dx
  */

  
  
  // get current stepper position
  float stepperPos = stepper->getCurrentPositionFromMin();
  // float lagedPos;

  // posArray[arrayIndex] = stepperPos;
  // arrayIndex++;
  // arrayIndex %= 10;

  // lagedPos = 



  // for (uint8_t aryIdx = 1; aryIdx <= 9; aryIdx++)
  // {
  //   posArray[aryIdx-1] = posArray[aryIdx];
  // }
  // posArray[9] = stepperPos;

  // uint8_t lag = 9-2;
  // stepperPos = posArray[lag];

  
  // stepperPos = posArray[lag];
  

  // // get iSVs position
  // float stepperPos2 = stepper->getServosInternalPositionCorrected() - stepper->getMinPosition();
  // stepperPos = stepperPos2;

  // Serial.printf("ESP pos: %f,    iSV pos: %f\n", stepperPos, stepperPos2);
  // delay(20);

  if (false == expFilterHasBeenInitialized)
  {
    filteredOutput = stepperPos;
    expFilterHasBeenInitialized = true;
  }

  // get current stepper velocity
  int32_t currentSpeedInMilliHz = stepper->getCurrentSpeedInMilliHz();
  uint32_t maxSpeedInMilliHz = stepper->getMaxSpeedInMilliHz();
  float speedNormalized_fl32 = ( (float)currentSpeedInMilliHz ) / ((float)maxSpeedInMilliHz)  ; // 250000000 --> 250
  float speedAbsNormalized_fl32 = constrain( fabsf(speedNormalized_fl32), 0.1f, 1.0f);
  float oneMinusSpeedNormalized_fl32 = 1.0f - speedAbsNormalized_fl32;
  
  // motion corrected loadcell reading
  float loadCellReadingKg_corrected = loadCellReadingKg;

  
  // set initial guess
  float stepperPos_initial = stepperPos;

  
  // foot spring stiffness
  float d_f_d_x_hor = -config_st->payLoadPedalConfig_.MPC_0th_order_gain;

  // velocity dependent foot spring stiffness 
  float d_f_t_d_x_hor = -config_st->payLoadPedalConfig_.MPC_1st_order_gain;

  // acceleration dependent foot spring stiffness 
  float d_f_tt_d_x_hor = -config_st->payLoadPedalConfig_.MPC_2nd_order_gain;


  // Serial.printf("MPC 0: %f,    1:%f,    2:%f\n", config_st->payLoadPedalConfig_.MPC_0th_order_gain, config_st->payLoadPedalConfig_.MPC_1st_order_gain, config_st->payLoadPedalConfig_.MPC_2nd_order_gain);
  // delay(20);

  // how many mm movement to order if 1kg of error force is detected
  // this can be tuned for responsiveness vs oscillation
  float mm_per_motor_rev = config_st->payLoadPedalConfig_.spindlePitch_mmPerRev_u8;//TRAVEL_PER_ROTATION_IN_MM;
  float steps_per_motor_rev = (float)calc_st->stepsPerMotorRevolution; //(float)STEPS_PER_MOTOR_REVOLUTION;

  float mmPerStep = 0.0f;
  if (steps_per_motor_rev > 0.0f)
  {
    mmPerStep = mm_per_motor_rev / steps_per_motor_rev ;
  }

  // compute d(x_hor) / d(step) from chain rule
  // d(x_hor) / d(step) = ( d(x_hor) / d(phi) ) * [ d(phi)/d(x) ] * { d(x)/d(step) }
  float d_x_hor_d_step = (-d_x_hor_d_phi) * (-d_phi_d_x) * mmPerStep;

  // Serial.printf("PosFraction: %f,    pos:%f,    travelRange:%f,    posMin:%d,    posMax:%d\n", stepper->getCurrentPositionFractionFromExternalPos( stepperPos ), stepperPos, stepper->getCurrentTravelRange(),  calc_st->stepperPosMin, calc_st->stepperPosMax );
  // delay(20);

  // Serial.printf("speed: %f,    maxSpeed:%f\n", (float)currentSpeedInMilliHz, (float)maxSpeedInMilliHz);
  // delay(20);

  // velocity dependent force in kg = (kg*s/step) * (step/s)
  float forceInKgAndSecondPerStep_fl32 = d_f_t_d_x_hor * d_x_hor_d_step;
  float velocityDependingForceInKg_fl32 = forceInKgAndSecondPerStep_fl32 * (currentSpeedInMilliHz / 1000.0f);

  // acceleration dependent force in kg = (kg*s^2/step) * (step/(s^2))
  float forceInKgAndSecondSquarePerStep_fl32 = d_f_tt_d_x_hor * d_x_hor_d_step;
  // Todo: compute acceleration dependet force
  

  // correct loadcell reading with velocity and acceleration readings
  float expectedCycleTime = 0.001f;
  // loadCellReadingKg_corrected -= velocityDependingForceInKg_fl32;

  // [mmPerStep] = mm/step, e.g. 0.001563 = 10mm/rev / 6400steps/rev
  // [d_phi_d_x] = deg/mm e.g. -0.305367
  // [d_x_hor_d_phi] = mm/deg, e.g. -3.832119
  // [d_x_hor_d_step] = mm/step, e.g. 0.001458
  // if (printStep > 10)
  // {
  //   Serial.printf("Vel:%f,    mmPerStep:%f,    d_phi_d_x:%f,    d_x_hor_d_phi:%f,    d_x_hor_d_step:%f,    force:%f\n", speedNormalized_fl32, mmPerStep, d_phi_d_x, d_x_hor_d_phi, d_x_hor_d_step, velocityDependingForceInKg_fl32);
  //   printStep = 0;
  // }
  // else{
  //   printStep++;
  // }
  

 
  // Find the intersections of the force curve and the foot model via Newtons-method
  #define MAX_NUMBER_OF_NEWTON_STEPS 5
  // int64_t stepUpdates[MAX_NUMBER_OF_NEWTON_STEPS];
  for (uint8_t iterationIdx = 0; iterationIdx < MAX_NUMBER_OF_NEWTON_STEPS; iterationIdx++)
  {
    //float stepperPosFraction = stepper->getCurrentPositionFraction();
    float stepperPosFraction = stepper->getCurrentPositionFractionFromExternalPos( stepperPos );
  
    // clamp the stepper position to prevent problems with the spline
    float x_0 = constrain(stepperPosFraction, 0.0f, 1.0f);
    
    // get force and force gradient of force vs travel curve
    float loadCellTargetKg = forceCurve->EvalForceCubicSpline(config_st, calc_st, x_0);
    float gradient_force_curve_fl32 = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, x_0, false);

    // Convert loadcell reading to pedal force
    // float sledPosition = sledPositionInMM_withPositionAsArgument(x_0 * stepper->getTravelSteps(), config_st, motorRevolutionsPerSteps_fl32);
    // float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, config_st);
    // // float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
    // float d_phi_d_x_2 = convertToPedalForceGain(sledPosition, config_st);

    // // compute gain for horizontal foot model
    // float b = config_st->payLoadPedalConfig_.lengthPedal_b;
    // float d = config_st->payLoadPedalConfig_.lengthPedal_d;
    // float d_x_hor_d_phi_2 = -(b+d) * sinf(pedalInclineAngleInDeg_fl32 * DEG_TO_RAD_FL32);

    // apply effect force offset
    // loadCellTargetKg -= absForceOffset_fl32;

    // make stiffness dependent on force curve gradient
    // less steps per kg --> steeper line
    float gradient_normalized_force_curve_fl32 = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, x_0, true);
    gradient_normalized_force_curve_fl32 = constrain(gradient_normalized_force_curve_fl32, 0.05f, 1.0f);

    // compute force error
    float forceError_fl32 = loadCellReadingKg_corrected - loadCellTargetKg;

    // angular foot model
    // m1 = d_f_d_x dForce / dx
    //float m1 = d_f_d_phi * (-d_phi_d_x);
    
    // Translational foot model
    // given in kg/step
    float m1 = d_f_d_x_hor * d_x_hor_d_step;

    // m1 *= oneMinusSpeedNormalized_fl32;
    
    // gradient of the force curve
    // given in kg/step
    float m2 = gradient_force_curve_fl32; 
    
    // Serial.printf("m1:%f,    m2:%f,    speed:%f\n", m1, m2, (float)currentSpeedInHz);
    // delay(20);

    // Newton update
    // float denom = m1 - m2 + d_f_t_d_x_hor * fabsf(m1) / speedAbsNormalized_fl32;
    // float denom = (m1 - m2) * (1.0f - config_st->payLoadPedalConfig_.MPC_1st_order_gain * fabsf(m1) / speedAbsNormalized_fl32 );
    float denom = m1 - m2;// - velocityDependingForceInKg_fl32;//config_st->payLoadPedalConfig_.MPC_1st_order_gain * oneMinusSpeedNormalized_fl32;
    
    if ( fabsf(denom) > 0.0f )
    {
      // https://en.wikipedia.org/wiki/Newton%27s_method
      // Newton algorithm
      // x(n+1) = x(n) + stepUpdate
      // x(n+1) = x(n) - f(x_n) / f'(x_n)
      float stepUpdate = - forceError_fl32 / ( denom );
      // a positive stepUpdate means sled moves away from the pedal.

      // smoothen update with force curve gradient since it had better results w/ clutch pedal characteristic
      // stepUpdate *= gradient_normalized_force_curve_fl32;
      // stepUpdate *= speedAbsNormalized_fl32;

      // update expected force reading
      // Todo: update expected force after step execution
      loadCellReadingKg_corrected += m1 * stepUpdate;

      // update position
      stepperPos += stepUpdate;

      // stop iteration
      if (fabsf(stepUpdate) < 2.0f)
      {
        break;
      }
      // stepUpdates[iterationIdx] = stepUpdate;
    }
  }

  // unsigned long now = micros();
  // float deltaTime = ( (float)(now - lastTime) ) / 1000000.0f; // in Sekunden
  // lastTime = now;

  

  // // Filter-Koeffizient berechnen
  // float alpha = deltaTime / (deltaTime + RC);

  // output = stepperPos;// - stepperPos_initial;

  // // Tiefpassfilter anwenden
  // filteredOutput = alpha * output + (1.0 - alpha) * filteredOutput;

  // stepperPos = filteredOutput;

  // Serial.printf("0:%i, 1:%i, 2:%i, 3:%i, 4:%i\n", stepUpdates[0], stepUpdates[1], stepUpdates[2], stepUpdates[3], stepUpdates[4]);
  // delay(20);
  
  // read the min position
  stepperPos += stepper->getMinPosition();

  // clamp target position to range
  int32_t posStepperNew = constrain(stepperPos, calc_st->stepperPosMin, calc_st->stepperPosMax );

  return posStepperNew;
}





// int32_t mpcBasedMove(float loadCellReadingKg, float stepperPosFraction, StepperWithLimits* stepper, ForceCurve_Interpolated* forceCurve, const DAP_calculationVariables_st* calc_st, DAP_config_st* config_st, float absForceOffset_fl32) 
// {



//   static const float MOVE_MM_FOR_1KG = 3.0;
//   static const float MOVE_STEPS_FOR_1KG = (MOVE_MM_FOR_1KG / TRAVEL_PER_ROTATION_IN_MM) * STEPS_PER_MOTOR_REVOLUTION;



//   // get target force at current location
//   float loadCellTargetKg = forceCurve->EvalForceCubicSpline(config_st, calc_st, stepperPosFraction);
//   loadCellTargetKg -=absForceOffset_fl32;

  

//   // get loadcell reading
//   float loadCellReadingKg_clip = constrain(loadCellReadingKg, calc_st->Force_Min, calc_st->Force_Max);

  
  
//   // if target force at location is lower than loadcell reading --> move towards the foot k_f * n_steps

//   // Take into account system constraints like stepper rpm & acceleration

  

//   // if target force at location is lower than loadcell reading --> move away from the foot -k_f * n_steps

  

//   // predict target force at new location and compare to predicted force --> compute cost matrix

  
  

  

  

//   // e_k = r^2 = (F_lc - k * (delta_x_0) - F_t(x_0 + delta_x_0))^2

//   // r: force residue

//   // e: cost

//   // F_lc: current loadcell measurement

//   // k: sping stiffness of the foot

//   // x_0: current stepper position

//   // x_1: next stepper pos

//   // delta_x_0 = x_1 - x_0: step update at time step 0

//   // F_t(x): target force at location

  

  

//   // minimize e with x_1

//   // d[e(delta_x_0)] / d[delta_x_0] == 0

  

//   // d[e] / d[delta_x_0] = d[e] / d[r] * d[r] / d[delta_x_0]

//   // d[e] / d[r] = 2 * r

  

//   // d[r] / d[delta_x_0] = d[F_lc - k * (delta_x_0) - F_t(x_0 + delta_x_0)] = -k  - d[F_t]/d[delta_x_0]

  

  

//   // MPC: sum up over planing horizon and optimize costs

//   // take only the first control value & repeat in the next cycle

//   // constraint |delta_x_0| < max step rate

  

//   // l = sum_k( e_k(delta_x_k, x_0) )

//   // where k = [0, 1, ..., N]

//   return 0;
// }



// see https://pidtuner.com
void measureStepResponse(StepperWithLimits* stepper, const DAP_calculationVariables_st* calc_st, const DAP_config_st* config_st, const LoadCell_ADS1256* loadcell)
{

  int32_t currentPos = stepper->getCurrentPositionFromMin();
  int32_t minPos = currentPos - dap_calculationVariables_st.stepperPosRange * 0.05f;
  int32_t maxPos = currentPos + dap_calculationVariables_st.stepperPosRange * 0.05f;

  stepper->moveTo(minPos, true);

  Serial.println("======================================");
  Serial.println("Start system identification data:");

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

      static RTDebugOutput<float, 3, 9> rtDebugFilter;
      rtDebugFilter.offerData({ ((float)t) *1e-6f , currentPos,  loadcellReading});   
    }
  }

  Serial.println("======================================");
  Serial.println("End system identification data");
}
