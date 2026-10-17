/* Week 1 / Ex2 — priorities, preemption, starvation.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks.h"

#define TASK_STACK_WORDS (128U)

void app_main(void)
{
    dwt_init();

    /* TODO: create the three tasks below with the priorities from params.h.
     *       Keep the stacks as small as you can still justify — ex4 will make
     *       you prove the number. */
    BaseType_t result;

    result = xTaskCreate(
        task_a, "A", TASK_STACK_WORDS, NULL, TASK_A_PRIO, NULL
    );
    configASSERT(result == pdPASS);

    result = xTaskCreate(
        task_b, "B", TASK_STACK_WORDS, NULL, TASK_B_PRIO, NULL
    );
    configASSERT(result == pdPASS);

    result = xTaskCreate(
        task_c, "C", TASK_STACK_WORDS, NULL, TASK_C_PRIO, NULL
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
