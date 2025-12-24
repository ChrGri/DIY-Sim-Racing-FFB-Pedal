#pragma once

#include <Arduino.h>
#include "esp_timer.h"
//#define BASE_TICK_US 300

class TaskScheduler
{
public:

    TaskScheduler();
    void begin(uint8_t timer_id = 0);
    void addScheduledTask(TaskFunction_t fn, const char *name, uint16_t intervalUs, UBaseType_t priority, BaseType_t core, uint32_t stackSize=2048u);
    

private:
        static const uint32_t BASE_TICK_US = 300; // Base tick in microseconds
        static const uint8_t MAX_TASKS = 15;      // Maximum tasks in scheduler
        // Task entry struct
        // Task entry struct
        typedef struct
        {
            TaskHandle_t handle;
            const char *name;
            TaskFunction_t fn;
            uint16_t intervalTicks;
            uint16_t counter;
            uint32_t lastKick; // last time task ran (micros)
            UBaseType_t priority;
            BaseType_t core;
        } SchedTask;

        // Task table
        SchedTask tasks[MAX_TASKS];
        uint8_t taskCount = 0;

        // Timer handle
        esp_timer_handle_t periodic_timer_handle = nullptr;

        void onTimer();
        static void IRAM_ATTR timerCallback(void *arg);
};

