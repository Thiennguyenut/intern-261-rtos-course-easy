/* Week 3 / Ex1 -- ISR -> task wakeup, NVIC priority.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "exti_button.h"

void app_main(void)
{
    exti_button_init();

    /* TODO: xTaskCreate ack_task. Keep its priority high enough that it is
     *       the thing that actually determines your measured latency, not
     *       scheduling jitter from something else. Justify the number in
     *       REPORT.md. */
    BaseType_t result = xTaskCreate(ack_task, "ACK", 256U, NULL, 2U, NULL);
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