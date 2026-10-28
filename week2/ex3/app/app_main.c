/* Week 2 / Ex3 — priority inversion.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks_lmh.h"
#include "dwt.h"

void app_main(void)
{
    dwt_init();
    resource_init();

    /* TODO: xTaskCreate task_low, task_mid, task_high with three DISTINCT
     *       priorities, low < mid < high. Justify the exact numbers in
     *       REPORT.md — they must leave room for configMAX_PRIORITIES and not
     *       collide with any other task in the system. */
    BaseType_t result = xTaskCreate(task_low, "L", 128U, NULL, 1U, NULL);
    configASSERT(result == pdPASS);
    
    result = xTaskCreate(task_mid, "M", 128U, NULL, 2U, NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(task_high, "H", 128U, NULL, 3U, NULL);
    configASSERT(result == pdPASS);

    vTaskStartScheduler();

    configASSERT(0);

    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    configASSERT(0);
}

void vApplicationStackOverflowHook(
    TaskHandle_t task,
    char *task_name
)
{
    (void)task;
    (void)task_name;

    configASSERT(0);
}