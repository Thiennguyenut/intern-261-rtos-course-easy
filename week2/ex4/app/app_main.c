/* Week 2 / Ex4 — event group & task notification.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "sync_tasks.h"
#include "dwt.h"

void app_main(void)
{
    dwt_init();
    sync_init();
    /* TODO 1: xTaskCreate sync_task first, store its handle into
     *         sync_task_handle (cast to void*) — producer_0 needs it when
     *         PRODUCER0_USE_NOTIFY is set. */
    TaskHandle_t handle = NULL;

    BaseType_t result = xTaskCreate(sync_task, "Sync", 256U, NULL, 2U, &handle);
    configASSERT(result == pdPASS);

    sync_task_handle = (void *)handle;

    /* TODO 2: xTaskCreate EVENT_BIT_COUNT instances of producer_task, each
     *         given its own index (0..EVENT_BIT_COUNT-1) as the task
     *         parameter. */
    for (uint32_t idx = 0U; idx < EX4_PRODUCER_COUNT; idx++) {
        
        result = xTaskCreate(producer_task, "Producer", 128U, (void *)(uintptr_t)idx, 1U, NULL);
        configASSERT(result == pdPASS);
    }

    vTaskStartScheduler();

    configASSERT(0);
    
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    configASSERT(0);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    configASSERT(0);
}