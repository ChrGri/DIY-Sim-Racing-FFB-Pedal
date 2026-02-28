#pragma once

#include "DiyActivePedal_types.h"
#include "StepperWithLimits.h"
#include "FastTrig.h"


static inline float sledPositionInMM(StepperWithLimits* stepper, DapConfig_t * config_st, float motorRevolutionsPerStep_fl32) {
  float currentPos = stepper->getCurrentPositionFromMin();
  return currentPos * motorRevolutionsPerStep_fl32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
}

static inline float sledPositionInMM_withPositionAsArgument(float currentPos_fl32, DapConfig_t * config_st, float motorRevolutionsPerStep_fl32) {
  return currentPos_fl32 * motorRevolutionsPerStep_fl32 * (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
}

static inline float pedalInclineAngleDeg(float sledPositionMM, DapConfig_t * config_st) {
  // see https://de.wikipedia.org/wiki/Kosinussatz
  // A: is lower pedal pivot
  // C: is upper pedal pivot
  // B: is rear pedal pivot
  float a = (float)config_st->payloadPedalConfig_st.lengthPedalA_i16;
  float b = (float)config_st->payloadPedalConfig_st.lengthPedalB_i16;
  float c_ver = (float)config_st->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float c_hor = (float)config_st->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);
  

//#define DEBUG_PEDAL_INCLINE
#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print("a: ");    ActiveSerial->print(a);
  ActiveSerial->print(", b: ");  ActiveSerial->print(b);
  ActiveSerial->print(", c: ");  ActiveSerial->print(c);

  ActiveSerial->print(", sledPositionMM: ");  ActiveSerial->print(sledPositionMM);
#endif

  float nom = b*b + c*c - a*a;
  float den = 2.0f * b * c;
  
  float alpha = 0.0f;
  if (abs(den) > 0.01f) {
    // alpha = acos( nom / den );
    // alpha = fastAcos( nom / den );
    alpha = iacos( nom / den ) * DEG_TO_RAD_FL32;
  }

#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print(", alpha1: ");  ActiveSerial->print(alpha * RAD_TO_DEG_FL32);
#endif

  // add incline due to AB incline --> result is incline realtive to horizontal 
  if (abs(c_hor)>0.01f) {
    // alpha += atan2f(c_ver, c_hor); // y, x
    alpha += atan2Fast(c_ver, c_hor); // y, x
  }





#ifdef DEBUG_PEDAL_INCLINE
  ActiveSerial->print(", alpha2: ");  ActiveSerial->print(alpha * RAD_TO_DEG_FL32);
  ActiveSerial->println(" ");
#endif

  
  return alpha * RAD_TO_DEG_FL32;
}


static inline float convertToPedalForce(float F_l, float sledPositionMM, DapConfig_t * config_st) {
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

  float a = (float)config_st->payloadPedalConfig_st.lengthPedalA_i16;
  float b = (float)config_st->payloadPedalConfig_st.lengthPedalB_i16;
  float d = (float)config_st->payloadPedalConfig_st.lengthPedalD_i16;

  float c_ver = (float)config_st->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float c_hor = (float)config_st->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);
  


  //ActiveSerial->print("a: ");    ActiveSerial->print(a);
  //ActiveSerial->print(", b: ");  ActiveSerial->print(b);
  //ActiveSerial->print(", c: ");  ActiveSerial->print(c);
  //ActiveSerial->print(", d: ");  ActiveSerial->print(d);
  //ActiveSerial->print(", sled: ");  ActiveSerial->print(sledPositionMM);
  //ActiveSerial->print(", b_hor: ");  ActiveSerial->print(b_hor);
  //ActiveSerial->println();


  // lower plus upper pedal plate length
  float b_plus_d = fabsf(b + d);

  // compute gamma angle, see https://de.wikipedia.org/wiki/Kosinussatz
  float nom = a*a + b*b - c*c;
  float den = 2 * a * b;
  
  float arg = 0.0f;
  if (abs(den) > 0.01f) {
    arg = nom / den;
    arg *= arg;
  }

  // apply conversion factor to loadcell reading 
  float one_minus_arg = 1.0f - arg;
  float F_b  = F_l;
  if ( (b_plus_d > 0.0f) && (one_minus_arg > 0.0f) )
  {
     F_b *= b / (b_plus_d) * sqrtf( one_minus_arg );
  }
  
  
  return F_b;
}


// Calculate gradient of phi with respect to sled position.
// This is done by taking the derivative of the force with respect to the sled position.
static inline float convertToPedalForceGain(float sledPositionMM, DapConfig_t * config_st) {
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

  float a = (float)config_st->payloadPedalConfig_st.lengthPedalA_i16;
  float b = (float)config_st->payloadPedalConfig_st.lengthPedalB_i16;
  float d = (float)config_st->payloadPedalConfig_st.lengthPedalD_i16;

  float c_ver = (float)config_st->payloadPedalConfig_st.lengthPedalCVertical_i16;
  float c_hor = (float)config_st->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);
  

  // float alpha = acos( (b*b + c*c - a*a) / (2.0f*b*c) );
  // float alpha = fastAcos( (b*b + c*c - a*a) / (2.0f*b*c) );
  float alpha = iacos( (b*b + c*c - a*a) / (2.0f*b*c) ) * DEG_TO_RAD_FL32;


  // float alphaPlus = atan2f(c_ver, c_hor); // y, x
  float alphaPlus = atan2Fast(c_ver, c_hor); // y, x

  

  // float sinAlpha = sin(alpha);
  // float cosAlpha = cos(alpha);
  // float sinAlphaPlus = sin(alphaPlus);
  // float cosAlphaPlus = cos(alphaPlus);

  float alphaInDeg = alpha * RAD_TO_DEG_FL32;
  float alphaPlusInDeg = alphaPlus * RAD_TO_DEG_FL32;
  float sinAlpha = isin(alphaInDeg);
  float cosAlpha = icos(alphaInDeg);
  float sinAlphaPlus = isin(alphaPlusInDeg);
  float cosAlphaPlus = icos(alphaPlusInDeg);

  // d_alpha_d_x
  float d_alpha_d_x = - 1.0f / fabs( sinAlpha ) * ( 1.0f / b - cosAlpha / c) * cosAlphaPlus;

  // d_alphaPlus_d_x
  float d_alphaPlus_d_x = - c_ver / (c * c);

  float d_phi_d_x = d_alpha_d_x + d_alphaPlus_d_x;

  // return in deg/mm
  return d_phi_d_x * RAD_TO_DEG_FL32;
}
