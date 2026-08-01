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



/*
  Geometry

        __
       (_/
        d
       /
      X_______
     /        ----a-----*
    b                   cv
   /                    |
  *--------ch(+ sled)---*


  alpha = /_ b ch

  X is a special point, since it has the only unknown coordinates, and encodes the current angle

  coordinates of X are at the point of intersection of a circle with radius a around (ch+sled,cv) and a circle with radius b at the origin.
  to compute, let:

  cv2 = cv * cv; ch2 = ch * ch; c2 = ch2 + cv2;
  det = sqrt(-cv2 * (c2 - (a - b) * (a - b)) * (c2 - (a + b) * (a + b)));
  scl = 0.5 / (cv * c2);
  Xx = scl * cv * (ch * (c2 + b*b - a*a) - det)
  Xy = scl * (cv2 * (c2 + b*b - a*a) + ch * det)

  This costs one sqrt and one divide, but with the position of X we can compute things very easily.

  For instance, sin(alpha) is the cross product of the vector X and ch as just |(Xy / b)|  
  [from: - Xy ch / (b * ch) = ||X|| ||ch|| sin(theta)]

  Likewise, sin(gamma) comes from cross product of vectors (ch - Xx, cv - Xy) and (-Xx, -Xy) as |(cv * Xx - Xy * ch) / (a * b)|
  [from: (ch - Xx) * -Xy + (cv - Xy) * Xx| / (a * b) =>  (ch * -Xy + cv * Xx)/ab]

 -------------------------------------------------------------------------------
  Mathematical Derivation of the Optimized Algorithm:

  The goal of this calculation is to determine the pedal angle (defined by the 
  coordinates of point X) depending on the sled position (ch). This is modeled 
  as the intersection of two circles:

  1. Circle 1: Describes the path of the lower pedal pivot around the origin. 
     Center: (0, 0), Radius: b.
     Equation: x^2 + y^2 = b^2

  2. Circle 2: Describes the length of the pushrod to the upper pivot point. 
     Center: (ch, cv), Radius: a.
     Equation: (x - ch)^2 + (y - cv)^2 = a^2

  Solving the system of equations:
  Expanding Circle 2 and subtracting Circle 1 gives:
     x^2 - 2*ch*x + ch^2 + y^2 - 2*cv*y + cv^2 = a^2
  Substituting x^2 + y^2 = b^2 and setting c^2 = ch^2 + cv^2:
     b^2 - 2*ch*x - 2*cv*y + c^2 = a^2
  Solving for y (using an auxiliary variable tt = c^2 + b^2 - a^2):
     y = (tt - 2*ch*x) / (2*cv)

  Substituting this y back into Circle 1 yields a quadratic equation for x. 
  Solving this using the quadratic formula leads to a discriminant (referred 
  to as 'det' or determinant below). 
  Rearranged and optimized, the term under the square root becomes:
     det = sqrt( -cv^2 * (c^2 - (a - b)^2) * (c^2 - (a + b)^2) )

  This yields the coordinates for x and y:
     x = (ch * tt - det) / (2 * c^2)
     y = (cv^2 * tt + ch * det) / (2 * cv * c^2)

  Since we are only looking for the angle alpha via atan2(y, x), the exact 
  scaling factor (1 / (2 * cv * c^2)) is irrelevant for the ratio. We can 
  multiply both sides by (2 * cv * c^2) to get the unscaled, but proportionally 
  correct vector coordinates Xx and Xy as they appear in the code:
     Xx = cv * (ch * tt - det)
     Xy = cv^2 * tt + ch * det
  
  The resulting atan2(Xy, Xx) provides the desired angle. The standard atan2 
  is replaced by a fast polynomial approximation.
*/
static inline IRAM_ATTR_FLAG float pedalInclineAngleDeg(float sledPositionMm_fl32, DapConfig_t * config_pst) {

  // Basic geometric lengths from configuration
  const float pedalLengthA_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalA_i16;
  const float pedalLengthB_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalB_i16;
  const float lengthCVertical_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCVertical_i16;
  const float lengthCHorizontal_fl32 = (float)config_pst->payloadPedalConfig_st.lengthPedalCHorizontal_i16 + sledPositionMm_fl32;

  // Squared lengths for the circle equations
  const float cVertSq_fl32 = lengthCVertical_fl32 * lengthCVertical_fl32;
  const float cHorizSq_fl32 = lengthCHorizontal_fl32 * lengthCHorizontal_fl32;
  const float cSq_fl32 = cHorizSq_fl32 + cVertSq_fl32;
  
  // Auxiliary terms to solve the system of equations (represented as 'tt' in derivation)
  const float termT_fl32 = cSq_fl32 + pedalLengthB_fl32 * pedalLengthB_fl32 - pedalLengthA_fl32 * pedalLengthA_fl32;
  
  // Calculate discriminant (det)
  const float detArg1_fl32 = cSq_fl32 - (pedalLengthA_fl32 - pedalLengthB_fl32) * (pedalLengthA_fl32 - pedalLengthB_fl32);
  const float detArg2_fl32 = cSq_fl32 - (pedalLengthA_fl32 + pedalLengthB_fl32) * (pedalLengthA_fl32 + pedalLengthB_fl32);
  const float determinant_fl32 = sqrtf(-cVertSq_fl32 * detArg1_fl32 * detArg2_fl32);
  
  // Unscaled coordinates (Xx, Xy) of the intersection point
  const float pivotXx_fl32 = lengthCVertical_fl32 * (lengthCHorizontal_fl32 * termT_fl32 - determinant_fl32);
  const float pivotXy_fl32 = cVertSq_fl32 * termT_fl32 + lengthCHorizontal_fl32 * determinant_fl32;

  // Check for division by zero / singularity
  if ((pivotXx_fl32 == 0.0f) && (pivotXy_fl32 == 0.0f)) return NAN;

  // Inline approximation of atan2(pivotXy_fl32, pivotXx_fl32)
  const bool isYGreater_b = fabsf(pivotXy_fl32) >= fabsf(pivotXx_fl32);
  float angleBase_fl32 = isYGreater_b ? 90.0f : (pivotXx_fl32 >= 0.0f) ? 0.0f : 180.0f;
  if (pivotXy_fl32 < 0.0f) angleBase_fl32 = -angleBase_fl32;
  
  const float ratioZ_fl32 = isYGreater_b ? (pivotXx_fl32 / pivotXy_fl32) : (pivotXy_fl32 / pivotXx_fl32);
  const float angleSign_fl32 = isYGreater_b ? -RAD_TO_DEG_FL32 : RAD_TO_DEG_FL32;
  const float ratioZSq_fl32 = ratioZ_fl32 * ratioZ_fl32;
  
  // Polynomial approximation for the angle calculation in degrees
  //return (((0.079331f * ratioZSq_fl32) - 0.288679f) * ratioZSq_fl32 + 0.995354f) * ratioZ_fl32 * angleSign_fl32 + angleBase_fl32;
  return ((((-0.0389929f * ratioZSq_fl32) + 0.1462766f) * ratioZSq_fl32 - 0.3211819f) * ratioZSq_fl32 + 0.9992150f) * ratioZ_fl32 * angleSign_fl32 + angleBase_fl32;
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
  float pedalLengthCSquared_fl32 = pedalLengthCVertical_fl32 * pedalLengthCVertical_fl32 + pedalLengthCHorizontal_fl32 * pedalLengthCHorizontal_fl32;
  
  // lower plus upper pedal plate length
  float pedalLengthBPlusD_fl32 = fabsf(pedalLengthB_fl32 + pedalLengthD_fl32);

  // compute gamma angle, see https://de.wikipedia.org/wiki/Kosinussatz
  float cosineNom_fl32 = pedalLengthA_fl32 * pedalLengthA_fl32 + pedalLengthB_fl32 * pedalLengthB_fl32 - pedalLengthCSquared_fl32;
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

