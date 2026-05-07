
#pragma once

#include "DiyActivePedal_types.h"
#include "Main.h"
#include <cmath>
const float DT_SECONDS = 0.0003f; 

class ChatterReduction {
private:
    
    int64_t last_check_time = 0; 
    float time_gap=0.0f;
    const float ACCEL_THRESHOLD_MIN_EFFECT = 129999.0f; 
    const float ACCEL_THRESHOLD_START = 79999.0f;
    const float VEL_THRESHOLD = 10000.0f;
    bool isChatter = false;
    const float D_FACTOR_MAX = 10.0f;
    const float DYNAMIC_GAIN_MIN=0.03f;  
    long printLast=0;

public:
    float lastActualAcceleration=0.0f;
    long lastActualPosition = 0; 
    float lastActualVelocity = 0.0f;
    ChatterReduction() 
    {
    }
    bool checkForChatter(long current_actual_position, int64_t current_time, bool printLog_b)
    {
        float time_differ= (float)(current_time-last_check_time) / 1000000.0f;
        float current_actual_velocity = (float)(current_actual_position - lastActualPosition) / time_differ;
        float current_actual_acceleration = (current_actual_velocity - lastActualVelocity) / time_differ ;
        bool is_chatter = false;
        if (std::abs(current_actual_acceleration) > ACCEL_THRESHOLD_START) is_chatter = true;
        lastActualPosition = current_actual_position;
        lastActualVelocity = current_actual_velocity;
        lastActualAcceleration=current_actual_acceleration;
        isChatter = is_chatter;
        time_gap= time_differ;
        last_check_time=current_time;
        if(millis()-printLast>100 && printLog_b)
        {
            ActiveSerial->print("time differ:");
            ActiveSerial->print(time_differ);
            ActiveSerial->print(", Vel:");
            ActiveSerial->print(lastActualVelocity);
            ActiveSerial->print(", ACC:");
            ActiveSerial->println(lastActualAcceleration);
            printLast=millis();
        }

        return is_chatter;
    }
    float DynamicEffectGain()
    {
        float intensity_S = 0.0f;
        float dynamic_gain = 1.0f;
        if(abs(lastActualVelocity)< VEL_THRESHOLD) return dynamic_gain;

        if(isChatter)
        {
            intensity_S = (abs(lastActualAcceleration) - ACCEL_THRESHOLD_START) / (ACCEL_THRESHOLD_MIN_EFFECT - ACCEL_THRESHOLD_START);
            intensity_S = constrain(intensity_S, 0.0f, 1.0f);
            dynamic_gain = 1.0f - (intensity_S * (1.0f - DYNAMIC_GAIN_MIN));
        }
        return dynamic_gain;
    }
};
ChatterReduction chatterReduction;