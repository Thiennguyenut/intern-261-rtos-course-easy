/* Week 1 / Ex4 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

void ram_report_task(void *argument);

/* TODO 1: static storage for stat_task — TCB and stack.
 *         The stack array type matters; it is not char[]. */

static StaticTask_t stat_tcb;
static StackType_t stat_stack[STATIC_TASK_STACK_WORDS];

TaskHandle_t dyn_task_handle = NULL;
TaskHandle_t stat_task_handle = NULL;
TaskHandle_t report_task_handle = NULL;

#define REPORT_STACK_WORDS (256U)

static void shared_work(void *argument)
{
    (void)argument;
    for (;;) {
        /* TODO 2: identical trivial work in both tasks, so that any difference
         *         in high water mark comes from how they were created, not from
         *         what they do. */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    /* TODO 3: dyn_task  via xTaskCreate,       stack DYN_TASK_STACK_WORDS
     *         stat_task via xTaskCreateStatic, stack STATIC_TASK_STACK_WORDS
     *         ram_report_task via xTaskCreate
     *
     * Check every return value. xTaskCreateStatic returns a handle, not a
     * BaseType_t — explain the difference in REPORT.md. */

    if (xTaskCreate(shared_work, "dyn_task", DYN_TASK_STACK_WORDS, NULL, 1U, &dyn_task_handle) != pdPASS) {
        Error_Handler();
    }

    stat_task_handle = xTaskCreateStatic(shared_work, "stat_task", STATIC_TASK_STACK_WORDS, NULL, 1U, stat_stack, &stat_tcb);

    if (stat_task_handle == NULL) {
        Error_Handler();
    }

    if (xTaskCreate(ram_report_task, "ram_report", REPORT_STACK_WORDS, NULL, 1U, &report_task_handle) != pdPASS) {
        Error_Handler();
    }
    /* TODO 4 (step 6 of the README, enable only for that run):
     *         declare a HOG_DEPTH_BYTES local array inside dyn_task's work
     *         function and write to every element, so the overflow is real and
     *         not optimised away. Think about what -O2 does to a dead array. */

    vTaskStartScheduler();

    /* Scheduler returned => not enough heap for the idle/timer task. */
    for (;;) {
    }
}
