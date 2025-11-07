
#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"
#include <cmath>
const float DT_SECONDS = 0.0003f; 

class ChatterReduction {
private:
    long last_actual_position = 0; 
    int64_t last_check_time = 0; 
    float time_gap=0.0f;
    float last_actual_velocity = 0.0f;
    float vcc_predict = 0.0f;
    const float ACCEL_THRESHOLD_STEPS_PER_S2 = 79999.0f; 
    const float ACCEL_THRESHOLD_START = 40000.0f;
    const float D_BASE_GAIN = 0.08f;
    const float D_INCREMENT = 0.5f;       
    const float D_RECOVERY  = 0.005f;
    bool isChatter = false;
    float current_D_factor = 1.0f; 
    const float D_FACTOR_MAX = 10.0f;
    const float DYNAMIC_GAIN_MIN=0.03f;
    //const float MAGNITUDE_THRESHOLD_NO_EFFECT = 0.1f;   

public:
    float currentAcc=0.0f;
    float currentVel=0.0f;
    ChatterReduction() 
    {
    }
    bool checkForChatter(long current_actual_position, int64_t current_time)
    {
        float time_differ= (float)(current_time-last_check_time) / 1000000.0f;
        float current_actual_velocity = (float)(current_actual_position - last_actual_position) / time_differ;
        float current_actual_acceleration = (current_actual_velocity - last_actual_velocity) / time_differ ;
        bool is_chatter = false;
        if (std::abs(current_actual_acceleration) > ACCEL_THRESHOLD_STEPS_PER_S2) is_chatter = true;
        last_actual_position = current_actual_position;
        last_actual_velocity = current_actual_velocity;
        currentAcc=current_actual_acceleration;
        currentVel= current_actual_velocity;
        isChatter = is_chatter;
        time_gap= time_differ;
        last_check_time=current_time;
        vcc_predict = currentVel + currentAcc* time_gap;
        return is_chatter;
    }
    float DynamicEffectGain()
    {
        float intensity_S = 0.0f;
        float dynamic_gain = 1.0f;
        if(isChatter)
        {
            intensity_S = (abs(currentAcc) - ACCEL_THRESHOLD_START) / (ACCEL_THRESHOLD_STEPS_PER_S2 - ACCEL_THRESHOLD_START);
            intensity_S = constrain(intensity_S, 0.0f, 1.0f);
            dynamic_gain = 1.0f - (intensity_S * (1.0f - DYNAMIC_GAIN_MIN));
        }
        return dynamic_gain;
    }
};
ChatterReduction chatterReduction;