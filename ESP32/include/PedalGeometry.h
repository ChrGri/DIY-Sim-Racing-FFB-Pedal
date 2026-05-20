#pragma once

#include "DiyActivePedal_types.h"
#include "StepperWithLimits.h"
#include "FastTrig.h"


static inline IRAM_ATTR_FLAG float sledPositionInMM(StepperWithLimits* stepper_pstwl, DapConfig_t * config_pst, float motorRevolutionsPerStep_fl32) {
  float currentPos_fl32 = stepper_pstwl->getCurrentPositionFromMin();
  return currentPos_fl32 * motorRevolutionsPerStep_fl32 * (float)config_pst->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
}

static inline IRAM_ATTR_FLAG float sledPositionInMM_withPositionAsArgument(float currentPos_fl32, DapConfig_t * config_pst, float motorRevolutionsPerStep_fl32) {
  return currentPos_fl32 * motorRevolutionsPerStep_fl32 * (float)config_pst->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
}



static inline IRAM_ATTR_FLAG float pedalInclineAngleDeg(float sledPositionMm_fl32, DapConfig_t * config_pst) {
  // see https://de.wikipedia.org/wiki/Kosinussatz
  // A: is lower pedal pivot
  // C: is upper pedal pivot
  // B: is rear pedal pivot
  float pedalLengthA_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalA_i16;
  float pedalLengthB_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalB_i16;
  float pedalLengthCVertical_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float pedalLengthCHorizontal_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMm_fl32;
  float pedalLengthC_fl32 = sqrtf(pedalLengthCVertical_fl32 * pedalLengthCVertical_fl32 + pedalLengthCHorizontal_fl32 * pedalLengthCHorizontal_fl32);
  

//#define DEBUG_PEDAL_INCLINE
#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print("a: ");    ActiveSerial->print(pedalLengthA_fl32);
  ActiveSerial->print(", b: ");  ActiveSerial->print(pedalLengthB_fl32);
  ActiveSerial->print(", c: ");  ActiveSerial->print(pedalLengthC_fl32);

  ActiveSerial->print(", sledPositionMM: ");  ActiveSerial->print(sledPositionMm_fl32);
#endif

  float cosineNom_fl32 = pedalLengthB_fl32 * pedalLengthB_fl32 + pedalLengthC_fl32 * pedalLengthC_fl32 - pedalLengthA_fl32 * pedalLengthA_fl32;
  float cosineDen_fl32 = 2.0f * pedalLengthB_fl32 * pedalLengthC_fl32;
  
  float alpha_fl32 = 0.0f;
  if (fabsf(cosineDen_fl32) > 0.01f) {
    // alpha = acos( nom / den );
    // alpha = fastAcos( nom / den );
    alpha_fl32 = iacos( cosineNom_fl32 / cosineDen_fl32 ) * DEG_TO_RAD_FL32;
  }

#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print(", alpha1: ");  ActiveSerial->print(alpha_fl32 * RAD_TO_DEG_FL32);
#endif

  // add incline due to AB incline --> result is incline realtive to horizontal 
  if (fabsf(pedalLengthCHorizontal_fl32)>0.01f) {
    // alpha += atan2f(c_ver, c_hor); // y, x
    alpha_fl32 += atan2Fast(pedalLengthCVertical_fl32, pedalLengthCHorizontal_fl32); // y, x
  }





#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print(", alpha2: ");  ActiveSerial->print(alpha_fl32 * RAD_TO_DEG_FL32);
  ActiveSerial->println(" ");
#endif

  
  return alpha_fl32 * RAD_TO_DEG_FL32;
}


static inline IRAM_ATTR_FLAG float pedalArcPercentage(StepperWithLimits* stepper_pstwl, DapConfig_t * config_pst, float motorRevolutionsPerStep_fl32, DapCalculationVariables_t* dapCalc_pst) {

  // travelSteps_cnt: total steps from min to max soft endstop
  float travelSteps_cnt = (float)(dapCalc_pst->softEndstopMaxStepperPos_i32 - dapCalc_pst->softEndstopMinStepperPos_i32);

  // steps to mm
  float stepsToMm_fl32 = motorRevolutionsPerStep_fl32 * (float)config_pst->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;

  float minSledPos_mm = 0.0f;
  float maxSledPos_mm = travelSteps_cnt * stepsToMm_fl32;

  // actualSledPos_mm: The current physical position of the ESP stepper in mm
  float actualSledPosFraction_01 = stepper_pstwl->getCurrentPositionFraction();
  float actualSledPos_mm = actualSledPosFraction_01 * maxSledPos_mm;

  // 2. Forward Kinematics: Angles at the boundaries and current physical state
  float angleAtMinSled_deg = pedalInclineAngleDeg(minSledPos_mm, config_pst);
  float angleAtMaxSled_deg = pedalInclineAngleDeg(maxSledPos_mm, config_pst);
  float currentAngle_deg = pedalInclineAngleDeg(actualSledPos_mm, config_pst);

  float actualPosFraction_01 = fabsf( (currentAngle_deg - angleAtMinSled_deg) / (angleAtMaxSled_deg - angleAtMinSled_deg) );
  return actualPosFraction_01 = constrain(actualPosFraction_01, 0.0f, 1.0f);
}

static inline IRAM_ATTR_FLAG float convertToPedalForce(float loadcellForce_fl32, float sledPositionMm_fl32, DapConfig_t * config_pst) {
  // see https://de.wikipedia.org/wiki/Kosinussatz
  // A: is lower pedal pivot
  // B: is rear pedal pivot
  // C: is upper pedal pivot
  // D: is foot rest
  //
  // a: is loadcell rod (connection CB)
  // b: is lower pedal plate (connection AC)
  // c: is sled line (connection AC)
  // d: is upper pedal plate  (connection AC)

  float pedalLengthA_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalA_i16;
  float pedalLengthB_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalB_i16;
  float pedalLengthD_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalD_i16;

  float pedalLengthCVertical_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float pedalLengthCHorizontal_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMm_fl32;
  float pedalLengthC_fl32 = sqrtf(pedalLengthCVertical_fl32 * pedalLengthCVertical_fl32 + pedalLengthCHorizontal_fl32 * pedalLengthCHorizontal_fl32);
  


  //ActiveSerial->print("a: ");    ActiveSerial->print(a);
  //ActiveSerial->print(", b: ");  ActiveSerial->print(b);
  //ActiveSerial->print(", c: ");  ActiveSerial->print(c);
  //ActiveSerial->print(", d: ");  ActiveSerial->print(d);
  //ActiveSerial->print(", sled: ");  ActiveSerial->print(sledPositionMM);
  //ActiveSerial->print(", b_hor: ");  ActiveSerial->print(b_hor);
  //ActiveSerial->println();


  // lower plus upper pedal plate length
  float pedalLengthBPlusD_fl32 = fabsf(pedalLengthB_fl32 + pedalLengthD_fl32);

  // compute gamma angle, see https://de.wikipedia.org/wiki/Kosinussatz
  float cosineNom_fl32 = pedalLengthA_fl32 * pedalLengthA_fl32 + pedalLengthB_fl32 * pedalLengthB_fl32 - pedalLengthC_fl32 * pedalLengthC_fl32;
  float cosineDen_fl32 = 2 * pedalLengthA_fl32 * pedalLengthB_fl32;
  
  float cosineArg_fl32 = 0.0f;
  if (fabsf(cosineDen_fl32) > 0.01f) {
    cosineArg_fl32 = cosineNom_fl32 / cosineDen_fl32;
    cosineArg_fl32 *= cosineArg_fl32;
  }

  // apply conversion factor to loadcell reading 
  float oneMinusCosineArg_fl32 = 1.0f - cosineArg_fl32;
  float pedalForce_fl32  = loadcellForce_fl32;
  if ( (pedalLengthBPlusD_fl32 > 0.0f) && (oneMinusCosineArg_fl32 > 0.0f) )
  {
     pedalForce_fl32 *= pedalLengthB_fl32 / (pedalLengthBPlusD_fl32) * sqrtf( oneMinusCosineArg_fl32 );
  }
  
  
  return pedalForce_fl32;
}


// Calculate gradient of phi with respect to sled position.
// This is done by taking the derivative of the force with respect to the sled position.
static inline IRAM_ATTR_FLAG float convertToPedalForceGain(float sledPositionMm_fl32, DapConfig_t * config_pst) {
  // see https://de.wikipedia.org/wiki/Kosinussatz
  // A: is lower pedal pivot
  // B: is rear pedal pivot
  // C: is upper pedal pivot
  // D: is foot rest
  //
  // a: is loadcell rod (connection CB)

  // b: is lower pedal plate (connection AC)
  // c: is sled line (connection AC)
  // d: is upper pedal plate  (connection AC)

  float pedalLengthA_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalA_i16;
  float pedalLengthB_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalB_i16;
  float pedalLengthD_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalD_i16;

  float pedalLengthCVertical_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float pedalLengthCHorizontal_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMm_fl32;
  float pedalLengthC_fl32 = sqrtf(pedalLengthCVertical_fl32 * pedalLengthCVertical_fl32 + pedalLengthCHorizontal_fl32 * pedalLengthCHorizontal_fl32);
  

  // float alpha = acos( (b*b + c*c - a*a) / (2.0f*b*c) );
  // float alpha = fastAcos( (b*b + c*c - a*a) / (2.0f*b*c) );
  float alpha_fl32 = iacos( (pedalLengthB_fl32 * pedalLengthB_fl32 + pedalLengthC_fl32 * pedalLengthC_fl32 - pedalLengthA_fl32 * pedalLengthA_fl32) / (2.0f * pedalLengthB_fl32 * pedalLengthC_fl32) ) * DEG_TO_RAD_FL32;


  // float alphaPlus = atan2f(c_ver, c_hor); // y, x
  float alphaPlus_fl32 = atan2Fast(pedalLengthCVertical_fl32, pedalLengthCHorizontal_fl32); // y, x

  

  // float sinAlpha = sin(alpha);
  // float cosAlpha = cos(alpha);
  // float sinAlphaPlus = sin(alphaPlus);
  // float cosAlphaPlus = cos(alphaPlus);

  float alphaInDeg_fl32 = alpha_fl32 * RAD_TO_DEG_FL32;
  float alphaPlusInDeg_fl32 = alphaPlus_fl32 * RAD_TO_DEG_FL32;
  float sinAlpha_fl32 = isin(alphaInDeg_fl32);
  float cosAlpha_fl32 = icos(alphaInDeg_fl32);
  // float sinAlphaPlus_fl32 = isin(alphaPlusInDeg_fl32);
  float cosAlphaPlus_fl32 = icos(alphaPlusInDeg_fl32);

  // d_alpha_d_x
  float dAlphaDx_fl32 = - 1.0f / fabsf( sinAlpha_fl32 ) * ( 1.0f / pedalLengthB_fl32 - cosAlpha_fl32 / pedalLengthC_fl32) * cosAlphaPlus_fl32;

  // d_alphaPlus_d_x
  float dAlphaPlusDx_fl32 = - pedalLengthCVertical_fl32 / (pedalLengthC_fl32 * pedalLengthC_fl32);

  float dPhiDx_fl32 = dAlphaDx_fl32 + dAlphaPlusDx_fl32;

  // return in deg/mm
  (void)pedalLengthD_fl32;
  return dPhiDx_fl32 * RAD_TO_DEG_FL32;
}
