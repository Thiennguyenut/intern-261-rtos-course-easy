/* Week 1 / Ex3 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

void periodic_task(void *argument);
void dwt_init(void);

void app_main(void)
{
    dwt_init();

    /* TODO: create periodic_task. Its priority must be the highest in the system
     *       — justify why in REPORT.md, and say what the measurement would look
     *       like if it were not. */
    BaseType_t result = xTaskCreate(
        periodic_task,
        "Periodic",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 1U,
        NULL
    );

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

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    configASSERT(0);
}
