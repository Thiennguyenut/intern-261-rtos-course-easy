/* Week 3 / Ex3 -- command parser: rx_stream -> change demo_task's
 * period/priority/suspend state at runtime.
 *
 * Commands (each terminated by '\r', max MAX_CMD_LEN bytes):
 *   P<ms>\r   -- set demo_task period to <ms>
 *   S<prio>\r -- vTaskPrioritySet(demo_task, <prio>)
 *   T<0|1>\r  -- 0 = vTaskSuspend(demo_task), 1 = vTaskResume(demo_task)
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"
#include "parser_task.h"

static TaskHandle_t s_demo_task_handle;
static volatile uint32_t s_demo_period_ms = 200;

// test gui lien tuc
static volatile uint32_t s_test_periods[100];
static volatile uint32_t s_test_count;

static void apply_command(const char *cmd, size_t len)
{
    /* TODO 1: switch on cmd[0]: 'P' -> parse decimal into s_demo_period_ms
     *         (demo_task reads this every loop, no restart needed);
     *         'S' -> parse decimal, vTaskPrioritySet(s_demo_task_handle, n);
     *         'T' -> cmd[1]=='0' ? vTaskSuspend(...) : vTaskResume(...).
     *         Ignore anything malformed -- do not crash on bad input. */
    if (len < 2U) {
        return;
    }

    uint32_t value = 0U;
    for (size_t i = 1U; i < len; i++) {
        if (cmd[i] < '0' || cmd[i] > '9') {
            return;
        }

        const uint32_t digit = (uint32_t)(cmd[i] - '0');
        if (value > (UINT32_MAX - digit) / 10U) {
            return;
        }
        value = value * 10U + digit;
    }

    switch (cmd[0]) {
    case 'P':
        if (pdMS_TO_TICKS(value) > 0U) {
            s_demo_period_ms = value;
        }
        // test gui lien tuc
        if (s_test_count < 100U) {
            s_test_periods[s_test_count] = value;
        }
        s_test_count++;
        
        break;

    case 'S':
        if (s_demo_task_handle != NULL && value < configMAX_PRIORITIES) {
            vTaskPrioritySet(s_demo_task_handle, (UBaseType_t)value);
        }
        break;

    case 'T':
        if (s_demo_task_handle != NULL && len == 2U) {
            if (value == 0U) {
                vTaskSuspend(s_demo_task_handle);
            } else if (value == 1U) {
                vTaskResume(s_demo_task_handle);
            }
        }
        break;

    default:
        break;
    }
}

void parser_task(void *argument)
{
    (void)argument;
    char cmd[MAX_CMD_LEN];
    size_t idx = 0;

    for (;;) {
        uint8_t byte;

        /* TODO 2: xStreamBufferReceive(rx_stream, &byte, 1, portMAX_DELAY). */
        if (xStreamBufferReceive(rx_stream, &byte, 1U, portMAX_DELAY) != 1U) {
            continue;
        }


        /* TODO 3: if byte == '\r', call apply_command(cmd, idx), idx = 0.
         *         else if idx < MAX_CMD_LEN - 1, cmd[idx++] = byte.
         *         else (overflow): discard the whole partial command, idx = 0
         *         -- explain in REPORT.md why silently truncating instead
         *         would be worse. */
        if (byte == '\r') {
            apply_command(cmd, idx);
            idx = 0U;
        } 
        else if (idx < MAX_CMD_LEN - 1U) {
            cmd[idx++] = (char)byte;
        } 
        else {
            idx = 0U;

            /* Discard the rest of the overlong command. */
            while (xStreamBufferReceive(rx_stream, &byte, 1U, portMAX_DELAY) != 1U ||byte != '\r') {
            }
        }
    }
}

void demo_task(void *argument)
{
    (void)argument;
    s_demo_task_handle = xTaskGetCurrentTaskHandle();

    for (;;) {
        /* TODO 4: toggle a GPIO pin here. */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
        vTaskDelay(pdMS_TO_TICKS(s_demo_period_ms));
    }
}
