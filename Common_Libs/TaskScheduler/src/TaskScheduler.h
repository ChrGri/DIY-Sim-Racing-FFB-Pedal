#pragma once

#include <Arduino.h>
#include "esp_timer.h"
//#define BASE_TICK_US_U32 300

class TaskScheduler
{
public:

    TaskScheduler();
    void begin(uint8_t timerId_u8 = 0U);
    void addScheduledTask(TaskFunction_t fn_tf, const char *name_pch, uint16_t intervalUs_u16, UBaseType_t priority_ubt, BaseType_t core_bt, uint32_t stackSize_u32 = 2048U);
    

private:
    static const uint32_t BASE_TICK_US_U32 = 300U; // Base tick in microseconds
    static const uint8_t MAX_TASKS_U8 = 15U;      // Maximum tasks in scheduler
        // Task entry struct
        // Task entry struct
    typedef struct SchedTask
        {
            TaskHandle_t handle_th;
            const char *name_pch;
            TaskFunction_t fn_tf;
            uint16_t intervalTicks_u16;
            uint16_t counter_u16;
            uint32_t lastKick_u32; // last time task ran (micros)
            UBaseType_t priority_ubt;
            BaseType_t core_bt;
        } SchedTask_t;

        // Task table
        SchedTask_t tasks_ast[MAX_TASKS_U8];
        uint8_t taskCount_u8 = 0;

        // Timer handle
        esp_timer_handle_t periodicTimerHandle_eth = nullptr;

        void onTimer();
        static void IRAM_ATTR timerCallback(void *arg_pv);
};

