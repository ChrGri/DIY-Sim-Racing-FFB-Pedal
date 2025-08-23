#pragma once
#include "esp_cpu.h"
#include "FunctionProfiler.h"
#include <map>

static TaskHandle_t currentTask = nullptr;
static uint64_t taskStartCycle = 0;
static std::map<TaskHandle_t, FunctionProfiler*> profilerRegistry;

inline void registerProfiler(FunctionProfiler* p) {
    profilerRegistry[p->taskHandle] = p;
}



#if defined(traceTASK_SWITCHED_OUT)
#undef traceTASK_SWITCHED_OUT
#warning "traceTASK_SWITCHED_OUT is defined"
#else
#warning "traceTASK_SWITCHED_OUT is NOT defined"
#endif

#if defined(traceTASK_SWITCHED_IN)
#undef traceTASK_SWITCHED_IN
#warning "traceTASK_SWITCHED_IN is defined"
#else
#warning "traceTASK_SWITCHED_IN is NOT defined"
#endif

// Called by FreeRTOS when switching in
#define traceTASK_SWITCHED_IN()  \
    do { \
        currentTask = xTaskGetCurrentTaskHandle(); \
        taskStartCycle = esp_cpu_get_ccount(); \
        auto it = profilerRegistry.find(currentTask); \
        if (it != profilerRegistry.end()) { \
            it->second->resumeAll(taskStartCycle); \
        } \
    } while (0)

// Called by FreeRTOS when switching out
#define traceTASK_SWITCHED_OUT() \
    do { \
        uint64_t now = esp_cpu_get_ccount(); \
        if (currentTask) { \
            auto it = profilerRegistry.find(currentTask); \
            if (it != profilerRegistry.end()) { \
                it->second->pauseAll(now); \
            } \
        } \
    } while (0)


