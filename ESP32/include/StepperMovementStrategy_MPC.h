#pragma once
#include "DiyActivePedal_types.h"
#include "Main.h"
#include "StepperWithLimits.h"
#include "ForceCurve.h"

unsigned long g_mpcLastTime_u32 = 0U;
float g_mpcOutput_fl32 = 0.0f;       // PID-Ausgang
float g_mpcFilteredOutput_fl32 = 0.0f;
bool g_mpcExpFilterHasBeenInitialized_b = false;
// Filter-Variablen
float g_rc_fl32 = 1.0f / (2.0f * 3.14159f * 100.0f); // RC fÃ¼r 20 Hz Eckfrequenz
float g_mpc0Gain_fl32 = 8.2f;
int g_printStep_i32 = 0;

float g_posArray_afl32[10] = {0.0f};
uint8_t g_arrayIndex_u8 = 0U;
int32_t MoveByForceTargetingStrategy(float loadCellReadingKg_fl32, StepperWithLimits* stepper, ForceCurveInterpolated* forceCurve, const DapCalculationVariables_t* calc_st, DapConfig_t* config_st, float absForceOffset_fl32, float changeVelocity_fl32, float dPhiDX_fl32, float dXHorDPhi_fl32)
{
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
  float stepperPos_fl32 = stepper->getCurrentPositionFromMin();

  float estimatedServoPosErrorInSteps_fl32 = stepper->getEstimatedPosError();
  if (0)
  {
    stepperPos_fl32 += estimatedServoPosErrorInSteps_fl32;
  }


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
  // float stepperPos2_fl32 = stepper->getServosInternalPositionCorrected() - stepper->getMinPosition();
  // stepperPos_fl32 = stepperPos2_fl32;

  // ActiveSerial->printf("ESP pos: %f,    iSV pos: %f\n", stepperPos_fl32, stepperPos2_fl32);
  // delay(20);

  if (false == g_mpcExpFilterHasBeenInitialized_b)
  {
    g_mpcFilteredOutput_fl32 = stepperPos_fl32;
    g_mpcExpFilterHasBeenInitialized_b = true;
  }

  // get current stepper velocity
  int32_t currentSpeedInMilliHz = stepper->getCurrentSpeedInMilliHz();
  uint32_t maxSpeedInMilliHz = stepper->getMaxSpeedInMilliHz();
  float speedNormalized_fl32 = ( (float)currentSpeedInMilliHz ) / ((float)maxSpeedInMilliHz)  ; // 250000000 --> 250
  float speedAbsNormalized_fl32 = constrain( fabsf(speedNormalized_fl32), 0.1f, 1.0f);
  float oneMinusSpeedNormalized_fl32 = 1.0f - speedAbsNormalized_fl32;
  
  // motion corrected loadcell reading
  float loadCellReadingKgCorrected_fl32 = loadCellReadingKg_fl32;

  // set initial guess
  float stepperPosInitial_fl32 = stepperPos_fl32;

  // foot spring stiffness
  float dFDXHor_fl32 = -1.0f * g_mpc0Gain_fl32;

  // velocity dependent foot spring stiffness 
  float dFTDXHor_fl32 = 0.0f;

  // acceleration dependent foot spring stiffness 
  float dFTtDXHor_fl32 = 0.0f;


  // ActiveSerial->printf("MPC 0: %f,    1:%f,    2:%f\n", config_st->payloadPedalConfig_st.MPC_0th_order_gain, config_st->payloadPedalConfig_st.MPC_1st_order_gain, config_st->payloadPedalConfig_st.MPC_2nd_order_gain);
  // delay(20);

  // how many mm movement to order if 1kg of error force is detected
  // this can be tuned for responsiveness vs oscillation
  float mmPerMotorRev_fl32 = config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;//TRAVEL_PER_ROTATION_IN_MM;
  float stepsPerMotorRev_fl32 = (float)calc_st->stepsPerMotorRevolution_u32; //(float)STEPS_PER_MOTOR_REVOLUTION;

  float mmPerStep_fl32 = 0.0f;
  if (stepsPerMotorRev_fl32 > 0.0f)
  {
    mmPerStep_fl32 = mmPerMotorRev_fl32 / stepsPerMotorRev_fl32 ;
  }

  // compute d(x_hor) / d(step) from chain rule
  // d(x_hor) / d(step) = ( d(x_hor) / d(phi) ) * [ d(phi)/d(x) ] * { d(x)/d(step) }
  float dXHorDStep_fl32 = (-dXHorDPhi_fl32) * (-dPhiDX_fl32) * mmPerStep_fl32;

  // ActiveSerial->printf("PosFraction: %f,    pos:%f,    travelRange:%f,    posMin:%d,    posMax:%d\n", stepper->getCurrentPositionFractionFromExternalPos( stepperPos ), stepperPos, stepper->getCurrentTravelRange(),  calc_st->stepperPosMin_i32, calc_st->stepperPosMax_i32 );
  // delay(20);

  // ActiveSerial->printf("speed: %f,    maxSpeed:%f\n", (float)currentSpeedInMilliHz, (float)maxSpeedInMilliHz);
  // delay(20);

  // velocity dependent force in kg = (kg*s/step) * (step/s)
  float forceInKgAndSecondPerStep_fl32 = dFTDXHor_fl32 * dXHorDStep_fl32;
  float velocityDependingForceInKg_fl32 = forceInKgAndSecondPerStep_fl32 * (currentSpeedInMilliHz / 1000.0f);

  // acceleration dependent force in kg = (kg*s^2/step) * (step/(s^2))
  float forceInKgAndSecondSquarePerStep_fl32 = dFTtDXHor_fl32 * dXHorDStep_fl32;
  // Todo: compute acceleration dependet force
  

  // correct loadcell reading with velocity and acceleration readings
  float expectedCycleTime_fl32 = 0.001f;
  // loadCellReadingKg_corrected -= velocityDependingForceInKg_fl32;

  // [mmPerStep_fl32] = mm/step, e.g. 0.001563 = 10mm/rev / 6400steps/rev
  // [dPhiDX_fl32] = deg/mm e.g. -0.305367
  // [dXHorDPhi_fl32] = mm/deg, e.g. -3.832119
  // [dXHorDStep_fl32] = mm/step, e.g. 0.001458
  // if (printStep > 10)
  // {
  //   ActiveSerial->printf("Vel:%f,    mmPerStep:%f,    d_phi_d_x:%f,    d_x_hor_d_phi:%f,    d_x_hor_d_step:%f,    force:%f\n", speedNormalized_fl32, mmPerStep_fl32, dPhiDX_fl32, dXHorDPhi_fl32, dXHorDStep_fl32, velocityDependingForceInKg_fl32);
  //   printStep = 0;
  // }
  // else{
  //   printStep++;
  // }
  

 
  // Find the intersections of the force curve and the foot model via Newtons-method
  #define MAX_NUMBER_OF_NEWTON_STEPS_U8 5U
  // int64_t stepUpdates[MAX_NUMBER_OF_NEWTON_STEPS];
  for (uint8_t iterationIdx_u8 = 0; iterationIdx_u8 < MAX_NUMBER_OF_NEWTON_STEPS_U8; iterationIdx_u8++)
  {
    //float stepperPosFraction = stepper->getCurrentPositionFraction();
    float stepperPosFraction_fl32 = stepper->getCurrentPositionFractionFromExternalPos( stepperPos_fl32 );
  
    // clamp the stepper position to prevent problems with the spline
    float x0_fl32 = constrain(stepperPosFraction_fl32, 0.0f, 1.0f);
    
    // get force and force gradient of force vs travel curve
    float loadCellTargetKg_fl32 = forceCurve->EvalForceCubicSpline(config_st, calc_st, x0_fl32);
    float gradientForceCurve_fl32 = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, x0_fl32, false);

    // Convert loadcell reading to pedal force
    // float sledPosition = sledPositionInMM_withPositionAsArgument(x_0 * stepper->getTravelSteps(), config_st, motorRevolutionsPerSteps_fl32);
    // float pedalInclineAngleInDeg_fl32 = pedalInclineAngleDeg(sledPosition, config_st);
    // // float pedalForce_fl32 = convertToPedalForce(loadcellReading, sledPosition, &dap_config_pedalUpdateTask_st);
    // float d_phi_d_x_2 = convertToPedalForceGain(sledPosition, config_st);

    // // compute gain for horizontal foot model
    // float b = config_st->payloadPedalConfig_st.lengthPedalB_i16;
    // float d = config_st->payloadPedalConfig_st.lengthPedalD_i16;
    // float d_x_hor_d_phi_2 = -(b+d) * sinf(pedalInclineAngleInDeg_fl32 * DEG_TO_RAD_FL32);

    // apply effect force offset
    // loadCellTargetKg -= absForceOffset_fl32;

    // make stiffness dependent on force curve gradient
    // less steps per kg --> steeper line
    float gradientNormalizedForceCurve_fl32 = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, x0_fl32, true);
    gradientNormalizedForceCurve_fl32 = constrain(gradientNormalizedForceCurve_fl32, 0.05f, 1.0f);

    // compute force error
    float forceError_fl32 = loadCellReadingKgCorrected_fl32 - loadCellTargetKg_fl32;

    // angular foot model
    // m1 = d_f_d_x dForce / dx
    //float m1 = d_f_d_phi * (-d_phi_d_x);
    
    // Translational foot model
    // given in kg/step
    float m1_fl32 = dFDXHor_fl32 * dXHorDStep_fl32;

    // m1 *= oneMinusSpeedNormalized_fl32;
    
    // gradient of the force curve
    // given in kg/step
    float m2_fl32 = gradientForceCurve_fl32; 
    
    // ActiveSerial->printf("m1:%f,    m2:%f,    speed:%f\n", m1, m2, (float)currentSpeedInHz);
    // delay(20);

    // Newton update
    // float denom = m1 - m2 + d_f_t_d_x_hor * fabsf(m1) / speedAbsNormalized_fl32;
    // float denom = (m1 - m2) * (1.0f - config_st->payloadPedalConfig_st.MPC_1st_order_gain * fabsf(m1) / speedAbsNormalized_fl32 );
    float denom_fl32 = m1_fl32 - m2_fl32;// - velocityDependingForceInKg_fl32;//config_st->payloadPedalConfig_st.MPC_1st_order_gain * oneMinusSpeedNormalized_fl32;
    
    if ( fabsf(denom_fl32) > 0.0f )
    {
      // https://en.wikipedia.org/wiki/Newton%27s_method
      // Newton algorithm
      // x(n+1) = x(n) + stepUpdate
      // x(n+1) = x(n) - f(x_n) / f'(x_n)
      float stepUpdate_fl32 = - forceError_fl32 / (denom_fl32);
      // a positive stepUpdate means sled moves away from the pedal.

      // smoothen update with force curve gradient since it had better results w/ clutch pedal characteristic
      // stepUpdate *= gradient_normalized_force_curve_fl32;
      // stepUpdate *= speedAbsNormalized_fl32;

      // update expected force reading
      // Todo: update expected force after step execution
      loadCellReadingKgCorrected_fl32 += m1_fl32 * stepUpdate_fl32;

      // update position
      stepperPos_fl32 += stepUpdate_fl32;

      // stop iteration
      if (fabsf(stepUpdate_fl32) < 2.0f)
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

  // output = stepperPos_fl32;// - stepperPosInitial_fl32;

  // // Tiefpassfilter anwenden
  // filteredOutput = alpha * output + (1.0 - alpha) * filteredOutput;

  // stepperPos = filteredOutput;

  // ActiveSerial->printf("0:%i, 1:%i, 2:%i, 3:%i, 4:%i\n", stepUpdates[0], stepUpdates[1], stepUpdates[2], stepUpdates[3], stepUpdates[4]);
  // delay(20);
  
  // read the min position
  stepperPos_fl32 += stepper->getMinPosition();

  // clamp target position to range
  int32_t posStepperNew = constrain(stepperPos_fl32, calc_st->stepperPosMin_i32, calc_st->stepperPosMax_i32 );

  return posStepperNew;
}
