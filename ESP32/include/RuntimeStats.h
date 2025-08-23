#pragma once
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// To use this profiler, you must enable the following in your FreeRTOS configuration
// (e.g., via build_flags in platformio.ini for ESP32):
// -D CONFIG_FREERTOS_USE_TRACE_FACILITY=1
// -D CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=1

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configGENERATE_RUN_TIME_STATS == 1 ) )

class TaskRuntimeProfiler {
public:
    void report() {
        UBaseType_t taskCount;
        uint32_t totalRunTime;

        // First call to get number of tasks
        taskCount = uxTaskGetSystemState(NULL, 0, &totalRunTime);

        if (taskCount == 0) {
            Serial.println("No tasks to report on.");
            return;
        }

        TaskStatus_t *taskArray = (TaskStatus_t *)malloc(taskCount * sizeof(TaskStatus_t));
        if (taskArray == NULL) {
            Serial.println("Malloc failed in profiler!");
            return;
        }

        // Second call fills the array
        taskCount = uxTaskGetSystemState(taskArray, taskCount, &totalRunTime);

        Serial.printf("\n--- Task Runtime Report (total runtime: %lu ticks) ---\n", totalRunTime);
        for (UBaseType_t i = 0; i < taskCount; i++) {
            float pct = 0.0f;
            if (totalRunTime > 0) {
                pct = (taskArray[i].ulRunTimeCounter * 100.0f) / totalRunTime;
            }

            Serial.printf("Task %-16s | Runtime: %-10lu | CPU: %5.2f%%\n",
                taskArray[i].pcTaskName,
                taskArray[i].ulRunTimeCounter,
                pct);
        }
        Serial.println(F("------------------------------------------------------"));

        free(taskArray);
    }
};

#else
#warning "Task profiling disabled. Set configUSE_TRACE_FACILITY and configGENERATE_RUN_TIME_STATS to 1."
class TaskRuntimeProfiler {
public:
    void report() {
        Serial.println("Task profiling is disabled. Please enable configUSE_TRACE_FACILITY and configGENERATE_RUN_TIME_STATS in your project configuration.");
    }
};
#endif // configUSE_TRACE_FACILITY && configGENERATE_RUN_TIME_STATS
