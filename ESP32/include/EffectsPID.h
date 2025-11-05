
#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"
#include <QuickPID.h>
class EffectOffsetPID {
private:
    // *Input, *Output, *Setpoint, *Kp, *Ki, *Kd
    float targetOffsetNorm = 0.0f;  
    float currentInputNorm = 0.0f;  
    float outputOffsetNorm = 0.0f;  
    float outputOffsetNormLast = 0.0f;
    // PID 
    float Kp_eff = 5.0f;  
    float Ki_eff = 0.0f;  
    float Kd_eff = 0.5f;  
    const float OUTPUT_MIN = -0.05f;
    const float OUTPUT_MAX = 0.05f;
    QuickPID effectsPID;


public:
    EffectOffsetPID():
    effectsPID(
            &currentInputNorm, 
            &outputOffsetNorm, 
            &targetOffsetNorm, 
            Kp_eff, 
            Ki_eff, 
            Kd_eff,
            QuickPID::pMode::pOnError, 
            QuickPID::dMode::dOnMeas,
            QuickPID::iAwMode::iAwClamp,
            QuickPID::Action::direct) 
    {   
        effectsPID.SetMode(QuickPID::Control::automatic);
        effectsPID.SetSampleTimeUs(REPETITION_INTERVAL_PEDAL_UPDATE_TASK_IN_US); 
        effectsPID.SetOutputLimits(OUTPUT_MIN, OUTPUT_MAX);
        //effectsPID.SetIntegralLimit(QuickPID::iLimit::iClamp);
    }
    
    int32_t computeEffectOffset(int32_t effectPositionOffset, const DAP_calculationVariables_st* calc_st)
    {
        
        targetOffsetNorm= (float)effectPositionOffset/calc_st->stepperPosRange;
        targetOffsetNorm = constrain(targetOffsetNorm, OUTPUT_MIN, OUTPUT_MAX);
        currentInputNorm = outputOffsetNormLast; 
        effectsPID.Compute();
        outputOffsetNorm= constrain(outputOffsetNorm, OUTPUT_MIN, OUTPUT_MAX);
        outputOffsetNormLast=outputOffsetNorm;
        int32_t targetEffectPosition= floor(outputOffsetNorm * calc_st->stepperPosRange);
        return targetEffectPosition;
    }
};

EffectOffsetPID EffectOffsetPID_;