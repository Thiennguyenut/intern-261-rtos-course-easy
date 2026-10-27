/* Week 2 / Ex2 — race condition & mutex.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

#include "main.h"
#include "params.h"
#include "uart_guard.h"
#include "dwt.h"

void writer_task(void *argument);

void app_main(void)
{
    dwt_init();
    uart_guard_init();

    /* TODO: xTaskCreate WRITER_COUNT instances of writer_task, each given its
     *       own index (0..WRITER_COUNT-1) as the task parameter.
     *       Do not create WRITER_COUNT separately-named functions. */
    for (uint32_t id = 0; id < WRITER_COUNT; id++) {
        BaseType_t status = xTaskCreate(writer_task, "Writer", 200U, (void *)(uintptr_t)id, 1U, NULL);

        configASSERT(status == pdPASS);
    }

    vTaskStartScheduler();

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