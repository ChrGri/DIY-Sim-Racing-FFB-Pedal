// FunctionProfiler.h
#pragma once
#include <Arduino.h>
#include <Main.h>
#include <limits>
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Example usage
// 1) create a profiler instance
// #include "FunctionProfiler.h"
// FunctionProfiler profiler_pedalUpdateTask;
// 2) activate the profiler
// profiler_pedalUpdateTask.activate( true );
// 3) start the timer for ID 0
// profiler_pedalUpdateTask.start(0);
// 4) end the timer for ID 0
// profiler_pedalUpdateTask.end(0);
// 5) print the report
// profiler_pedalUpdateTask.report();
// 6) In setup(), start the printing task
// profiler_pedalUpdateTask.startPrintTask();

class FunctionProfiler {
public:
    static const int MAX_TIMERS = 16;
    String taskName = "default";
    uint32_t nmbCalls_u32 = 3000;

    FunctionProfiler() {
        for (int i = 0; i < MAX_TIMERS; ++i) {
            mins[i] = std::numeric_limits<unsigned long>::max();
        }
        reportMutex = xSemaphoreCreateMutex();
        xTaskCreatePinnedToCore(
            printTaskStatic,
            "ProfilerPrint",
            4096,
            this,
            1, // Low priority
            NULL,
            0); // Core 1
    }

    void setName(String name) {
        taskName = name;
    }

    void setNumberOfCalls(uint32_t nmbCallsArg_u32) {
        nmbCalls_u32 = constrain(nmbCallsArg_u32, 1, 10000);
    }

    void activate(bool activeFlagArg_b) {
        activeFlag_b = activeFlagArg_b;
    }

    void start(uint8_t id) {
        if (activeFlag_b) {
            if (id >= MAX_TIMERS) return;
            startTimes[id] = micros();
            active[id] = true;
        }
    }

    void end(uint8_t id) {
        if (activeFlag_b) {
            if (id >= MAX_TIMERS || !active[id]) return;
            unsigned long dur = micros() - startTimes[id];
            durations[id] += dur;
            counts[id]++;
            last[id] = dur;
            if (dur < mins[id]) mins[id] = dur;
            if (dur > maxs[id]) {
                maxs[id] = dur;
                maxs_cycle[id] = counts[id];
            }
            active[id] = false;
        }
    }

    void reset() {
        for (int i = 0; i < MAX_TIMERS; ++i) {
            durations[i] = 0;
            counts[i] = 0;
            mins[i] = std::numeric_limits<unsigned long>::max();
            maxs[i] = 0;
            last[i] = 0;
            active[i] = false;
            maxs_cycle[i] = 0;
        }
    }

    void report() {
        if (activeFlag_b && counts[0] >= nmbCalls_u32) {
            // Lock the mutex to safely swap buffers
            if (xSemaphoreTake(reportMutex, portMAX_DELAY) == pdTRUE) {
                // Copy current data to the report buffers
                memcpy(reportDurations, durations, sizeof(durations));
                memcpy(reportCounts, counts, sizeof(counts));
                memcpy(reportMins, mins, sizeof(mins));
                memcpy(reportMaxs, maxs, sizeof(maxs));
                memcpy(reportLast, last, sizeof(last));
                memcpy(reportMaxsCycle, maxs_cycle, sizeof(maxs_cycle));
                
                // Reset the main buffers for the next cycle
                reset();

                // Give back the mutex
                xSemaphoreGive(reportMutex);

                // Print the report from the dedicated task
                printReportInternal();
            }
        }
    }

private:
    unsigned long startTimes[MAX_TIMERS] = {0};
    unsigned long durations[MAX_TIMERS] = {0};
    unsigned long last[MAX_TIMERS] = {0};
    unsigned long mins[MAX_TIMERS];
    unsigned long maxs[MAX_TIMERS] = {0};
    unsigned long maxs_cycle[MAX_TIMERS] = {0};
    uint32_t counts[MAX_TIMERS] = {0};
    bool active[MAX_TIMERS] = {false};
    bool activeFlag_b = false;

    // Double buffers for printing
    unsigned long reportDurations[MAX_TIMERS] = {0};
    unsigned long reportLast[MAX_TIMERS] = {0};
    unsigned long reportMins[MAX_TIMERS];
    unsigned long reportMaxs[MAX_TIMERS] = {0};
    unsigned long reportMaxsCycle[MAX_TIMERS] = {0};
    uint32_t reportCounts[MAX_TIMERS] = {0};

    SemaphoreHandle_t reportMutex;

    void printReportInternal() {
        ActiveSerial->printf("\n------ FunctionProfiler Report (by ID) for task: %s ------\n", taskName.c_str());
        for (int i = 0; i < MAX_TIMERS; ++i) {
            if (reportCounts[i] > 0) {
                unsigned long avg = reportDurations[i] / reportCounts[i];
                ActiveSerial->printf("ID %2d: calls=%lu | avg=%lu us | min=%lu us | max=%lu us | last=%lu us | maxs_cycle=%lu\n",
                    i, reportCounts[i], avg, reportMins[i], reportMaxs[i], reportLast[i], reportMaxsCycle[i]);
            }
        }
        ActiveSerial->println(F("---------------------------------------------"));
        ActiveSerial->flush();
        delay(10);
    }

    static void printTaskStatic(void* pvParameters) {
        FunctionProfiler* profiler = static_cast<FunctionProfiler*>(pvParameters);
        for (;;) {
            // This task is simple; it just yields, allowing the main task
            // to run and call report() which then triggers the print.
            // This design offloads the long-running print calls to a
            // separate thread without complex queue management.
            // vTaskDelay(pdMS_TO_TICKS(100)); // Yield to other tasks

            delay(1000);

            profiler->report();
        }
    }
};