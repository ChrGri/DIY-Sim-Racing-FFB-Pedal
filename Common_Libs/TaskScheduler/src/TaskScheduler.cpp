#include "TaskScheduler.h"
//initialized with null handle
TaskScheduler::TaskScheduler() : taskCount_u8(0), periodicTimerHandle_eth(nullptr)
{
    // Initialize task array handles to NULL for safety
    for (uint8_t taskIndex_u8 = 0U; taskIndex_u8 < MAX_TASKS_U8; ++taskIndex_u8)
    {
        tasks_ast[taskIndex_u8].handle_th = NULL;
    }
}

// Initializes the scheduler timer
void TaskScheduler::begin(uint8_t timerId_u8)
{
    (void)timerId_u8;
    // === Replace hw_timer with esp_timer ===
    const esp_timer_create_args_t periodicTimerArgs_st = {
        .callback = &TaskScheduler::timerCallback,
        .arg = this, // Pass instance pointer to the callback
        .name = "sched_timer"};

    esp_timer_create(&periodicTimerArgs_st, &periodicTimerHandle_eth);

    // Start periodic timer at BASE_TICK_US_U32 interval
    esp_timer_start_periodic(periodicTimerHandle_eth, BASE_TICK_US_U32);
}

// === Scheduler API ===
void TaskScheduler::addScheduledTask(TaskFunction_t fn_tf, const char *name_pch, uint16_t intervalUs_u16, UBaseType_t priority_ubt, BaseType_t core_bt, uint32_t stackSize_u32)
{
    if (taskCount_u8 >= MAX_TASKS_U8)
    {
        return; // limit reached
    }

    uint16_t intervalTicks_u16 = intervalUs_u16 / BASE_TICK_US_U32;
    if (intervalTicks_u16 == 0)
    {
        intervalTicks_u16 = 1; // minimum 1 tick
    }

    // Create task
    xTaskCreatePinnedToCore(fn_tf, name_pch, stackSize_u32, NULL, priority_ubt,
                            &tasks_ast[taskCount_u8].handle_th, core_bt);

    tasks_ast[taskCount_u8].intervalTicks_u16 = intervalTicks_u16;
    tasks_ast[taskCount_u8].counter_u16 = 0U;
    tasks_ast[taskCount_u8].name_pch = name_pch;
    taskCount_u8++;
}

// === Scheduler ISR ===
void IRAM_ATTR TaskScheduler::timerCallback(void *arg_pv)
{
    // The argument is the "this" pointer for the TaskScheduler instance.
    TaskScheduler *taskScheduler_pst = static_cast<TaskScheduler *>(arg_pv);
    taskScheduler_pst->onTimer();
}

void TaskScheduler::onTimer()
{
    BaseType_t xHigherPriorityWoken_bt = pdFALSE;

    for (uint8_t taskIndex_u8 = 0U; taskIndex_u8 < taskCount_u8; taskIndex_u8++)
    {
        tasks_ast[taskIndex_u8].counter_u16++;
        if (tasks_ast[taskIndex_u8].counter_u16 >= tasks_ast[taskIndex_u8].intervalTicks_u16)
        {
            tasks_ast[taskIndex_u8].counter_u16 = 0U;
            if (NULL != tasks_ast[taskIndex_u8].handle_th)
            {
                vTaskNotifyGiveFromISR(tasks_ast[taskIndex_u8].handle_th, &xHigherPriorityWoken_bt);
            }
        }
    }

    // Yield if a higher-priority task was woken.
    if (xHigherPriorityWoken_bt)
    {
        portYIELD_FROM_ISR();
    }
}