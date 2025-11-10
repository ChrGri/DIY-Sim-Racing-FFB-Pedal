#pragma once
#include "DiyActivePedal_types.h"
#include "Main.h"

class MovementDirection
{
    private:
        long lastPosition=0;
        int64_t lastCheckTime=0;
    public:
    float velocityCheck(long currentPosition)
    {
        int64_t currentTime = esp_timer_get_time();
        int64_t deltaTime = currentTime-lastCheckTime;
        long deltaPosition= currentPosition-lastPosition;
        float velocity = (float)deltaPosition/(float)deltaTime;
        return velocity;
    }
};
MovementDirection movementDireciton;