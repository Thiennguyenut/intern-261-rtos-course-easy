/* Week 1 / Ex4 — print the live RAM picture over USART1.
 *
 * This is your measurement instrument. It must be honest: read the values from
 * FreeRTOS, never from a constant you typed in.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

extern TaskHandle_t extra_task_handle;
extern TaskHandle_t dyn_task_handle;
extern TaskHandle_t stat_task_handle;
extern TaskHandle_t report_task_handle;

/* TODO 1: a blocking UART print helper. It is called from a task, so it may
 *         block — but say in REPORT.md what would break if you called it from
 *         the stack overflow hook instead. */
static void uart_print(const char *text)
{
    size_t length = strlen(text);

    if (length == 0U) {
        return;
    }

    if (length > UINT16_MAX || HAL_UART_Transmit(&huart1, (const uint8_t *)text, (uint16_t)length, 100U) != HAL_OK) {
        Error_Handler();
    }
}

static void print_hwm(const char *name, TaskHandle_t handle)
{
    char line[80];

    if (handle == NULL) {
        Error_Handler();
    }

    UBaseType_t words = uxTaskGetStackHighWaterMark(handle);

    int length = snprintf(
        line, sizeof(line), "%s HWM=%lu words / %lu bytes\r\n", name, (unsigned long)words, (unsigned long)(words * sizeof(StackType_t)));

    if (length < 0 || (size_t)length >= sizeof(line)) {
        Error_Handler();
    }

    uart_print(line);
}

void ram_report_task(void *argument)
{
    (void)argument;

    TaskHandle_t idle_handle = xTaskGetIdleTaskHandle();
    TaskHandle_t timer_handle = xTimerGetTimerDaemonTaskHandle();

    for (;;) {
        /* TODO 2: print, every 2 seconds:
         *   - xPortGetFreeHeapSize()
         *   - xPortGetMinimumEverFreeHeapSize()
         *   - uxTaskGetStackHighWaterMark() for: dyn_task, stat_task,
         *     ram_report_task, the idle task (xTaskGetIdleTaskHandle()),
         *     and the timer task (xTimerGetTimerDaemonTaskHandle()).
         *
         * High water mark is returned in WORDS. Print both words and bytes so
         * your report cannot silently mix the two units. */
        
        char line[96];

        size_t free_heap = xPortGetFreeHeapSize();
        size_t minimum_heap = xPortGetMinimumEverFreeHeapSize();

        int length = snprintf(
            line, sizeof(line),
            "\r\nheap_free=%lu bytes; heap_min=%lu bytes\r\n"
            "target=%lu bytes: %s\r\n",
            (unsigned long)free_heap,
            (unsigned long)minimum_heap,
            (unsigned long)TARGET_FREE_HEAP_BYTES,
            free_heap >= TARGET_FREE_HEAP_BYTES ? "PASS" : "FAIL");

        if (length < 0 || (size_t)length >= sizeof(line)) {
            Error_Handler();
        }

        uart_print(line);

        print_hwm("dyn_task", dyn_task_handle);
        print_hwm("stat_task", stat_task_handle);
        print_hwm("extra_task", extra_task_handle);
        print_hwm("ram_report", report_task_handle);
        print_hwm("idle", idle_handle);
        print_hwm("timer", timer_handle);
        

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
