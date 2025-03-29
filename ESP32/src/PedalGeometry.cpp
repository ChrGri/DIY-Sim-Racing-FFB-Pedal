#include "PedalGeometry.h"

#include "StepperWithLimits.h"
//#include "FastTrig.h"

KALMAN<3,1> K_pedal_geometry;
unsigned long _timeLastObservation;

static const float KF_MODEL_NOISE_FORCE_ACCELERATION = ( 10000000000. );


// evolution matrix. Size is <Nstate,Nstate>
  /*K_pedal_geometry.F = {1.0, 0.0, 0.0,
           0.0, 1.0, 0.0,
           0.0, 0.0, 1.0};*/
        


float sledPositionInMM(StepperWithLimits* stepper, DAP_config_st * config_st, float motorRevolutionsPerStep_fl32) {
  float currentPos = stepper->getCurrentPositionFromMin();
  return currentPos * motorRevolutionsPerStep_fl32 * config_st->payLoadPedalConfig_.spindlePitch_mmPerRev_u8;
}

float sledPositionInMM_withPositionAsArgument(float currentPos_fl32, DAP_config_st * config_st, float motorRevolutionsPerStep_fl32) {
  return currentPos_fl32 * motorRevolutionsPerStep_fl32 * config_st->payLoadPedalConfig_.spindlePitch_mmPerRev_u8;
}

float pedalInclineAngleDeg(float sledPositionMM, DAP_config_st * config_st) {
  // see https://de.wikipedia.org/wiki/Kosinussatz
  // A: is lower pedal pivot
  // C: is upper pedal pivot
  // B: is rear pedal pivot
  float a = config_st->payLoadPedalConfig_.lengthPedal_a;
  float b = config_st->payLoadPedalConfig_.lengthPedal_b;
  float c_ver = config_st->payLoadPedalConfig_.lengthPedal_c_vertical;
  float c_hor = config_st->payLoadPedalConfig_.lengthPedal_c_horizontal + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);

//#define DEBUG_PEDAL_INCLINE
#ifdef DEBUG_PEDAL_INCLINE
  Serial.print("a: ");    Serial.print(a);
  Serial.print(", b: ");  Serial.print(b);
  Serial.print(", c: ");  Serial.print(c);

  Serial.print(", sledPositionMM: ");  Serial.print(sledPositionMM);
#endif

  float nom = b*b + c*c - a*a;
  float den = 2.0f * b * c;
  
  float alpha = 0.0f;
  if (abs(den) > 0.01f) {
    alpha = acos( nom / den );
  }

#ifdef DEBUG_PEDAL_INCLINE
  Serial.print(", alpha1: ");  Serial.print(alpha * RAD_TO_DEG);
#endif

  // add incline due to AB incline --> result is incline realtive to horizontal 
  if (abs(c_hor)>0.01f) {
    alpha += atan2f(c_ver, c_hor); // y, x
    //alpha += atan2Fast(c_ver, c_hor); // y, x
  }





#ifdef DEBUG_PEDAL_INCLINE
  Serial.print(", alpha2: ");  Serial.print(alpha * RAD_TO_DEG);
  Serial.println(" ");
#endif

  
  return alpha * RAD_TO_DEG;
}



float convertToPedalForce(float F_l, float sledPositionMM, DAP_config_st * config_st) {
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

  float a = (float)config_st->payLoadPedalConfig_.lengthPedal_a;
  float b = (float)config_st->payLoadPedalConfig_.lengthPedal_b;
  float d = (float)config_st->payLoadPedalConfig_.lengthPedal_d;

  float c_ver = (float)config_st->payLoadPedalConfig_.lengthPedal_c_vertical;
  float c_hor = (float)config_st->payLoadPedalConfig_.lengthPedal_c_horizontal + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);


  //Serial.print("a: ");    Serial.print(a);
  //Serial.print(", b: ");  Serial.print(b);
  //Serial.print(", c: ");  Serial.print(c);
  //Serial.print(", d: ");  Serial.print(d);
  //Serial.print(", sled: ");  Serial.print(sledPositionMM);
  //Serial.print(", b_hor: ");  Serial.print(b_hor);
  //Serial.println();


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
     F_b *= b / (b_plus_d) * sqrt( one_minus_arg );
  }
  
  
  return F_b;
}




// Calculate gradient of phi with respect to sled position.
// This is done by taking the derivative of the force with respect to the sled position.
float convertToPedalForceGain(float sledPositionMM, DAP_config_st * config_st) {
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

  float a = config_st->payLoadPedalConfig_.lengthPedal_a;
  float b = config_st->payLoadPedalConfig_.lengthPedal_b;
  float d = config_st->payLoadPedalConfig_.lengthPedal_d;

  float c_ver = config_st->payLoadPedalConfig_.lengthPedal_c_vertical;
  float c_hor = config_st->payLoadPedalConfig_.lengthPedal_c_horizontal + sledPositionMM;
  float c = sqrtf(c_ver * c_ver + c_hor * c_hor);


  float alpha = acos( (b*b + c*c - a*a) / (2.0f*b*c) );
  float alphaPlus = atan2f(c_ver, c_hor); // y, x
  //float alphaPlus = atan2Fast(c_ver, c_hor); // y, x

  float sinAlpha = sin(alpha);
  float cosAlpha = cos(alpha);
  float sinAlphaPlus = sin(alphaPlus);
  float cosAlphaPlus = cos(alphaPlus);

  // d_alpha_d_x
  float d_alpha_d_x = - 1.0f / fabs( sinAlpha ) * ( 1.0f / b - cosAlpha / c) * cosAlphaPlus;

  // d_alphaPlus_d_x
  float d_alphaPlus_d_x = - c_ver / (c * c);

  float d_phi_d_x = d_alpha_d_x + d_alphaPlus_d_x;

  // return in deg/mm
  return d_phi_d_x * RAD_TO_DEG;
}





float pedalInclineAngleAccel(float pedalInclineAngleDeg_global) {


  // estimate pedals angular velocity and acceleration
  // obtain time
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - _timeLastObservation;
  if (elapsedTime < 1) { elapsedTime=1; }
  _timeLastObservation = currentTime;

  // update state transition and system covariance matrices
  float delta_t = (float)elapsedTime / 1000000.0f; // convert to seconds
  float delta_t_pow2 = delta_t * delta_t;
  float delta_t_pow3 = delta_t_pow2 * delta_t;
  float delta_t_pow4 = delta_t_pow2 * delta_t_pow2;

  K_pedal_geometry.F = {1.0f,  delta_t, 0.5f * delta_t * delta_t,
          0.0,  1.0f, delta_t,
          0.0, 0.0, 1.0f};

  // measurement matrix. Size is <Nobs,Nstate>
  K_pedal_geometry.H = {1.0f, 0.0f, 0.0f};

  // model covariance matrix. Size is <Nstate,Nstate>
  /*K_pedal_geometry.Q = {1000, 0.0, 0.0,
          0.0, 1000, 0.0,
          0.0, 0.0, 1000};*/

  // measurement covariance matrix. Size is <Nobs,Nobs>
  K_pedal_geometry.R = { 0.0001f };

  /*
  float K_Q_11 = KF_MODEL_NOISE_FORCE_ACCELERATION * 0.5f * delta_t_pow3;
  float K_Q_12 = KF_MODEL_NOISE_FORCE_ACCELERATION * 0.5f * delta_t_pow2;
  K_pedal_geometry.Q = {  KF_MODEL_NOISE_FORCE_ACCELERATION * 0.25f * delta_t_pow4,   K_Q_11,                                                   K_Q_12,
                          K_Q_11,                                                     KF_MODEL_NOISE_FORCE_ACCELERATION * delta_t_pow2,         delta_t,
                          K_Q_12,                                                     delta_t,                                                  1.0};
*/  


  // 1 * x + deltaT * x_d + 0.5 * deltaT^2 * x_dd + 1/6 * deltaT^3 * x_ddd
  // 1 / 6 * delta_t * delta_t * delta_t
  // 1 / 2 * delta_t * delta_t
  // delta_t

  float Q11 = KF_MODEL_NOISE_FORCE_ACCELERATION * (1.0f / 6.0f * delta_t * delta_t * delta_t) * (1.0f / 6.0f * delta_t * delta_t * delta_t);
  float Q12 = KF_MODEL_NOISE_FORCE_ACCELERATION * (1.0f / 6.0f * delta_t * delta_t * delta_t) * (1.0f / 2.0f * delta_t * delta_t);
  float Q13 = KF_MODEL_NOISE_FORCE_ACCELERATION * (1.0f / 6.0f * delta_t * delta_t * delta_t) * (delta_t);

  float Q21 = Q12;
  float Q22 = KF_MODEL_NOISE_FORCE_ACCELERATION * (1.0f / 2.0f * delta_t * delta_t) * (1.0f / 2.0f * delta_t * delta_t);
  float Q23 = KF_MODEL_NOISE_FORCE_ACCELERATION * (1.0f / 2.0f * delta_t * delta_t) * (delta_t);

  float Q31 = Q13;
  float Q32 = Q23;
  float Q33 = KF_MODEL_NOISE_FORCE_ACCELERATION * (delta_t) * (delta_t);





  K_pedal_geometry.Q = {  Q11, Q12, Q12,
                          Q21, Q21, Q21,
                          Q31, Q32, Q33};

        

  // APPLY KALMAN FILTER
  K_pedal_geometry.update({pedalInclineAngleDeg_global});
  float pedalPos = K_pedal_geometry.x(0,0);
  float pedalVel = K_pedal_geometry.x(0,1);
  float pedalAccel = K_pedal_geometry.x(0,2);


  //Serial.print("alpha2: ");  
  //Serial.print(pedalInclineAngleDeg_global * RAD_TO_DEG);
  //Serial.print(", pedalPos: ");  
  //Serial.print(pedalPos * RAD_TO_DEG);
  //Serial.print(", pedalVel: ");  
  //Serial.print(pedalVel);
  //Serial.print(", pedalAccel: ");  

  //Serial.print(",   ");
  //Serial.print(pedalAccel);
  //Serial.println(" ");

  return pedalAccel * RAD_TO_DEG;

 } 
