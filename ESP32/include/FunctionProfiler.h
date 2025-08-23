#pragma once
#include <Arduino.h>
#include <limits>
#include <string>
#include "esp_cpu.h"
#include "esp_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


class FunctionProfiler {
public:
    static const int MAX_TIMERS = 16;
    std::string taskName;
    TaskHandle_t taskHandle;
    bool activeFlag_b = false;

    uint32_t nmbCalls_u32 = 1000;   // report after N calls (default)
    uint32_t globalCount = 0;       // counts across all sections   

    FunctionProfiler(TaskHandle_t task = nullptr, const char* name = "default") {
        taskHandle = task ? task : xTaskGetCurrentTaskHandle();
        taskName = name;
        cpu_freq_mhz = esp_clk_cpu_freq() / 1000000;
        reset();
    }

    void setName(const char* name) {
        taskName = name;
    }

    void setNumberOfCalls(uint32_t nmbCallsArg_u32)
    {
        nmbCalls_u32 = constrain( nmbCallsArg_u32, 1, 10000);
    }

    void activate(bool activeFlagArg_b)
    {
        activeFlag_b = activeFlagArg_b;
    }

    // Start section
    void start(uint8_t id) {
        if (id >= MAX_TIMERS) return;
        active[id] = true;
        startCycles[id] = esp_cpu_get_ccount();
    }

    // Stop section
    void end(uint8_t id) {
        if (id >= MAX_TIMERS || !active[id]) return;
        uint64_t dur = esp_cpu_get_ccount() - startCycles[id];
        accumulate(id, dur);
        counts[id]++;
        active[id] = false;
    }

    // Called automatically when task is switched out
    void pauseAll(uint64_t now) {
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (active[i]) {
                uint64_t dur = now - startCycles[i];
                accumulate(i, dur);
                taskSwitch[i]++;
            }
        }
    }

    // Called automatically when task resumes
    void resumeAll(uint64_t now) {
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (active[i]) {
                startCycles[i] = now; // reset base point
            }
        }
    }

    void report() {
        if (activeFlag_b)
        {
            if (counts[0] >= nmbCalls_u32)
                {
                Serial.printf("\n--- Profiler report for task %s ---\n", taskName.c_str());
                for (int i = 0; i < MAX_TIMERS; i++) {
                    if (counts[i] >= nmbCalls_u32) {
                        uint64_t avg = totalCycles[i] / counts[i];
                        Serial.printf("ID %2d: calls=%-6u | avg=%-5lu us | min=%-5lu us | max=%-5lu us | last=%-5lu us | task_switches=%d\n",
                            i, counts[i],
                            (unsigned long)(avg / cpu_freq_mhz),
                            (unsigned long)(min_cycles[i] / cpu_freq_mhz),
                            (unsigned long)(max_cycles[i] / cpu_freq_mhz),
                            (unsigned long)(last_cycles[i] / cpu_freq_mhz),
                            taskSwitch[i]);
                    }
                }
                Serial.println(F("-----------------------------------"));

                // reset the values
                reset();
            }
        }
        
    }

    void reset() {
        for (int i = 0; i < MAX_TIMERS; i++) {
            totalCycles[i] = 0;
            counts[i] = 0;
            taskSwitch[i] = 0;
            min_cycles[i] = std::numeric_limits<uint64_t>::max();
            max_cycles[i] = 0;
            last_cycles[i] = 0;
            active[i] = false;
            startCycles[i] = 0;
        }
        globalCount = 0;
    }

private:
    uint32_t cpu_freq_mhz;
    uint64_t startCycles[MAX_TIMERS];
    uint64_t totalCycles[MAX_TIMERS];
    uint64_t last_cycles[MAX_TIMERS];
    uint64_t min_cycles[MAX_TIMERS];
    uint64_t max_cycles[MAX_TIMERS];
    uint32_t counts[MAX_TIMERS];
    uint32_t taskSwitch[MAX_TIMERS];
    
    bool active[MAX_TIMERS];

    void accumulate(uint8_t id, uint64_t dur) {
    totalCycles[id] += dur;
    // counts[id]++;
    last_cycles[id] = dur;
    if (dur < min_cycles[id]) min_cycles[id] = dur;
    if (dur > max_cycles[id]) max_cycles[id] = dur;

    // globalCount++;
    // if (globalCount >= nmbCalls_u32) {
    //     report();
    //     reset();
    //     globalCount = 0;
    // }
}
};
