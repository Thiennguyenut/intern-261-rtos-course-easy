/* Week 1 / Ex4 — FreeRTOS hooks.
 *
 * Both hooks must be observable from outside the chip. A breakpoint is not a
 * deliverable: you need a UART line or a distinct LED pattern you can photograph.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

extern UART_HandleTypeDef huart1;

static void hook_uart_write(const char *text, uint16_t length)
{
    if (HAL_UART_Transmit(&huart1, (const uint8_t *)text, length, 100U) != HAL_OK) {
        Error_Handler();
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* TODO 1: report xTask / pcTaskName out of the chip, then stop.
     *
     * Careful: the stack of the offending task is already corrupt. Think about
     * what you are allowed to call here, and write the reasoning in REPORT.md.
     * Hint: this hook runs from the scheduler's context, not the task's.
     */
    (void)xTask;
    static const char prefix[] = "\r\nSTACK_OVERFLOW task=";
    static const char newline[] = "\r\n";

    hook_uart_write(prefix, sizeof(prefix) - 1U);

    if (pcTaskName != NULL) {
        uint16_t length = 0U;

        while (length < configMAX_TASK_NAME_LEN && pcTaskName[length] != '\0') {
            ++length;
        }

        if (length > 0U) {
            hook_uart_write(pcTaskName, length);
        }
    }

    hook_uart_write(newline, sizeof(newline) - 1U);

    Error_Handler();
}

void vApplicationMallocFailedHook(void)
{
    /* TODO 2: report, then stop. */
    static const char message[] = "\r\nMALLOC_FAILED\r\n";

    hook_uart_write(message, sizeof(message) - 1U);
    Error_Handler();
}

/* configSUPPORT_STATIC_ALLOCATION = 1 makes these two callbacks mandatory.
 * Provide the memory for the idle task and the timer task here. */

static StaticTask_t idle_tcb;
static StackType_t idle_stack[STATIC_TASK_STACK_WORDS];

static StaticTask_t timer_tcb;
static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* TODO 3: static buffers. Note the size is in WORDS, not bytes —
     *         getting this wrong is a silent overflow. */

    *ppxIdleTaskTCBBuffer = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize = STATIC_TASK_STACK_WORDS;

}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* TODO 4: same, for the software timer task. */
    

    *ppxTimerTaskTCBBuffer = &timer_tcb;
    *ppxTimerTaskStackBuffer = timer_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
