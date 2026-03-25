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

/**
 * @brief Executes the Admittance Control strategy for the active pedal.
 * * Admittance control is a branch of robotics and haptics where the system measures 
 * an external force (user's foot) and outputs a position or velocity based on a 
 * simulated virtual environment. This function simulates a 1-Degree-of-Freedom (1-DOF) 
 * Mass-Spring-Damper system.
 * * The control loop works by calculating the net force on a "Virtual Mass":
 * F_net = F_human - F_spring - F_damping
 * * It then integrates acceleration (F_net / Mass) to find virtual velocity, and 
 * integrates velocity to find the new virtual position. The physical stepper motor 
 * is then commanded to track this virtual position.
 * * @note CRUCIAL ARCHITECTURAL WARNING regarding `g_vModelPos_01`:
 * DO NOT overwrite the virtual model position (`g_vModelPos_01`) with the physical 
 * motor's current position (`stepper->getCurrentPositionFraction()`) at the start 
 * of the loop. 
 * * In admittance control, the virtual model is the "Ground Truth" of the physics 
 * engine. The physical motor is merely a slave trying to follow this model. 
 * If you overwrite the virtual position with the physical position:
 * 1. You destroy the second-order physics (Mass and Damping). The system degrades 
 * into a jittery first-order system because the velocity integrator loses its 
 * historical state.
 * 2. You introduce a "Unity-Gain Loopback". The physics engine will calculate a 
 * reaction based on a delayed physical state, command a new physical state, 
 * and then snap back to the delayed state on the next frame. This destroys 
 * phase margin and causes violent, high-frequency oscillations (hunting).
 * * Instead, we use a "Soft Leash" (divergence correction) at the end of the loop 
 * to gently pull the virtual model toward reality, correcting floating-point drift 
 * without corrupting the inertia and damping integrators.
 * * @note INERTIAL COUPLING & PARASITIC MASS:
 * Because the load cell is physically sandwiched between the motor and the pedal 
 * faceplate, it measures the force required to accelerate the physical faceplate 
 * in addition to the user's input (F_measured = F_human + m_physical * a).
 * When solving the admittance equation with this measured force, the physical mass 
 * of the faceplate mathematically adds itself to the virtual mass parameter: 
 * m_effective = m_virtual + m_physical.
 * Therefore, the absolute lowest effective mass the user can feel is the physical 
 * weight of the faceplate itself (e.g., 500g). Setting `virtualMass_kg` lower than 
 * this simply means the perceived mass approaches the physical mass.
 * * @see "Impedance and Admittance Control" - Siciliano & Khatib, Springer Handbook of Robotics.
 * @see "Virtual Model Control" - Pratt et al. (For concepts on simulating virtual components on physical actuators).
 * @see "Control System Design: An Introduction to State-Space Methods" - Friedland (For state observers and divergence correction).
 * * @param loadCellReadingKg_fl32 Raw force applied by the user in Kg.
 * @param stepper Pointer to the stepper motor control interface.
 * @param forceCurve Pointer to the interpolated force/travel curve.
 * @param calc_st Pointer to the pedal's static calculation variables.
 * @param config_st Pointer to the pedal's configuration structure.
 * @param absForceOffset_fl32 High-frequency force offset (e.g., ABS vibration force).
 * @param absPosOffset_fl32 High-frequency position offset (e.g., ABS vibration displacement).
 * @return int32_t The next absolute target position in steps for the stepper motor driver.
 */

 
// --- Global Admittance State Variables ---
// renamed from g_admittancePos_fl32 and g_admittanceVel_fl32
float g_vModelPos_01 = 0.0f;    // Normalized virtual position [0.0 to 1.0]
float g_vModelVel_mps = 0.0f;   // Physical virtual velocity [meters/second]

int32_t IRAM_ATTR_FLAG MoveByAdmittanceStrategy(float loadCellReadingKg_fl32, StepperWithLimits* stepper, ForceCurveInterpolated* forceCurve, const DapCalculationVariables_t* calc_st, DapConfig_t* config_st, float effectForceOffsetInKg_fl32, float effectPosOffsetInSteps_fl32)
{
  // --- 1. PHYSICAL PARAMETERS & CONFIGURATION ---
  // Parameters can be exposed to config later.
  float virtualMass_kg = 0.3f;     // Virtual pedal inertia
  float dampingRatio_zeta = 1.3f;  // Overdamped (zeta > 1) to ensure stability and "heavy" feel
  const float GRAVITY_N_KG = 9.81f; // Conversion constant for Kg to Newtons

  // Time step for integration (seconds), but use actual loop time for better performance and stability.
  // But make sure to protect against wrapsound of the timer by using uint64_t and checking for large dt values.
  uint64_t currentTimeUs = esp_timer_get_time();
  static uint64_t lastTimeUs = 0;
  if (lastTimeUs == 0) {
    lastTimeUs = currentTimeUs;
  }
  float dt_s = ((float)currentTimeUs - (float)lastTimeUs) * 1e-6f;
  if (dt_s > 1.0f || dt_s <= 0.0f) {
    // If the time step is too large (e.g., due to timer wraparound or long loop delay), reset it to a default value to prevent instability.
    dt_s = ((float)REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US_I64) * 1e-6f;
  }
  lastTimeUs = currentTimeUs;


  // Calculate total physical travel in meters
  // travelSteps_cnt: total steps from min to max
  float travelSteps_cnt = (float)(calc_st->stepperPosMax_i32 - calc_st->stepperPosMin_i32);
  
  // totalTravel_m: physical length of the pedal stroke in meters
  float totalTravel_m = travelSteps_cnt * motorRevolutionsPerSteps_fl32 * config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8 * 0.001f;
  
  // actualPosFraction_01: The current command position of the ESP stepper [0.0, 1.0]
  float actualPosFraction_01 = stepper->getCurrentPositionFraction(); 

  // --- 2. SPRING REACTION (The Force Curve) ---
  // 2. Read the spring force (target force) from the spline based on current physical position
  // We use actualPosFraction_01 here to couple the "feel" to the pedal's real location
  float springForceRaw_kg = forceCurve->EvalForceCubicSpline(config_st, calc_st, constrain(actualPosFraction_01, 0.0f, 1.0f));
  float springForce_N = springForceRaw_kg * GRAVITY_N_KG;

  // --- 3. DYNAMIC STIFFNESS & DAMPING ---
  // Calculate local physical spring stiffness (N/m) 
  // units are kg/step, convert to N/m: (kg/step) * (steps/m) * (N/kg)
  float localStiffness_kg_step = forceCurve->EvalForceGradientCubicSpline(config_st, calc_st, constrain(actualPosFraction_01, 0.0f, 1.0f), false);
  float localStiffness_N_m = max(localStiffness_kg_step * (travelSteps_cnt / max(totalTravel_m, 0.0001f)) * GRAVITY_N_KG, 1.0f);


  // --- 4. FORCE NORMALIZATION (Converting Human Input to SI Newtons) ---
  // 1. Convert loadcell reading (human force) into normalized percentage [0, 1]
  float loadCellReadingKgClip_fl32 = constrain(loadCellReadingKg_fl32, calc_st->forceMin_fl32, calc_st->forceMax_fl32);
  // 2. Apply absolute force offsets (e.g., from ABS vibrations) to the load cell reading

  // --- 5. EFFECT COUPLING (Converting High-Frequency Position Offsets to Force Offsets) ---
  // convert position offset to force offset using the local stiffness at the current position on the force curve
  float effectPositionToForceConversion_kg = effectPosOffsetInSteps_fl32 * localStiffness_kg_step;
  float appliedForce_kg = loadCellReadingKgClip_fl32 + effectForceOffsetInKg_fl32 + effectPositionToForceConversion_kg;
  float externalForce_N = appliedForce_kg * GRAVITY_N_KG;

  // Calculate Critical Damping: c_c = 2 * sqrt(mass * stiffness)
  // Applied damping coefficient in [Newton-seconds / meter]
  float virtualDamping_Ns_m = dampingRatio_zeta * 2.0f * sqrtf(virtualMass_kg * localStiffness_N_m);

  // --- 6. ADMITTANCE PHYSICS MODEL (Mass-Spring-Damper) ---
  // F_net = F_human - F_spring - F_damping = M * a
  float netForce_N = externalForce_N - springForce_N - (virtualDamping_Ns_m * g_vModelVel_mps);
  float acceleration_mps2 = netForce_N / virtualMass_kg;

  // Integrate acceleration to get physical velocity [m/s]
  g_vModelVel_mps += acceleration_mps2 * dt_s;

  // Protect against mathematical blow-up and clamp velocity to hardware maximum limits
  // Example: 250000steps/s * 10mm/rev * 1rev/3780steps = ~0.66 m/s
  float maxPhysicalVel_mps = 1.0f; 
  if (calc_st->stepsPerMotorRevolution_u32 > 0) {
    float maxPhysicalVel_mps = MAXIMUM_STEPPER_SPEED_U32;
    maxPhysicalVel_mps *= (float)config_st->payloadPedalConfig_st.spindlePitch_mmPerRev_u8;
    maxPhysicalVel_mps /= (float)calc_st->stepsPerMotorRevolution_u32;
    maxPhysicalVel_mps *= 0.001f; // convert mm/s to m/s
  }
  g_vModelVel_mps = constrain(g_vModelVel_mps, -maxPhysicalVel_mps, maxPhysicalVel_mps);

  // Integrate physical velocity to get physical position [meters]
  float currentPos_m = (g_vModelPos_01 * totalTravel_m) + (g_vModelVel_mps * dt_s);

  // Map physical position back to normalized percentage [0, 1]
  if (totalTravel_m > 0.0001f) {
      g_vModelPos_01 = currentPos_m / totalTravel_m;
  } else {
      g_vModelPos_01 = 0.0f;
  }

  // --- 7. BOUNDARY CONSTRAINTS ---
  // Hard constraint at mechanical boundaries (inelastic collision)
  if (g_vModelPos_01 <= 0.0f) {
      g_vModelPos_01 = 0.0f;
      if (g_vModelVel_mps < 0.0f) g_vModelVel_mps = 0.0f;
  } else if (g_vModelPos_01 >= 1.0f) {
      g_vModelPos_01 = 1.0f;
      if (g_vModelVel_mps > 0.0f) g_vModelVel_mps = 0.0f;
  }

  // --- 8. DRIFT CORRECTION (State Synchronization) ---
  // Synchronize the virtual model with the actual stepper command position
  // This prevents the model from diverging due to integration errors or floating point drift.
  float divergence_01 = actualPosFraction_01 - g_vModelPos_01;
  g_vModelPos_01 += divergence_01 * 0.05f; // 5% correction per cycle

  // --- 9. STEP CONVERSION & OUTPUT ---
  // Convert normalized position to stepper integer steps
  float targetPosSteps_fl32 = g_vModelPos_01 * travelSteps_cnt;
  targetPosSteps_fl32 += (float)calc_st->stepperPosMin_i32;

  // Apply high-frequency position offsets (ABS vibrations, etc) 
  // Offset is applied after the physics loop to keep the integrator "clean"
  //targetPosSteps_fl32 -= (effectPosOffsetInSteps_fl32 * travelSteps_cnt);

  int32_t finalTargetPos_i32 = (int32_t)floor(targetPosSteps_fl32);
  
  // Final safety clamp to hardware limits
  return (int32_t)constrain(finalTargetPos_i32, calc_st->stepperPosMin_i32, calc_st->stepperPosMax_i32);
}