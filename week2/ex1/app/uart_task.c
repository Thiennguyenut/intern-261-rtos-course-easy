/* Week 2 / Ex1 — prints filtered samples to USART1 at UART_BAUD.
 * queue_filtered -> UART.
 *
 * This is also the task you deliberately starve in step 7 (temporarily set
 * UART_BAUD-equivalent hardware baud to 9600) to force queue_filtered to fill.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

#include "main.h"
#include "params.h"
#include "pipeline.h"

extern UART_HandleTypeDef huart1;

static volatile uint32_t uart_format_errors = 0U;
static volatile uint32_t uart_tx_errors = 0U;
static volatile uint32_t uart_rows_sent = 0U;

void uart_task(void *argument)
{
    (void)argument;

    char line[40];

    for (;;) {
        filtered_sample_t in;

        /* TODO 1: xQueueReceive(queue_filtered, &in, portMAX_DELAY). */
        BaseType_t receive_result = xQueueReceive(queue_filtered, &in, portMAX_DELAY);

        if (receive_result != pdPASS) {
            continue;
        }

        /* TODO 2: format "seq,ax,ay,az\r\n" and HAL_UART_Transmit on USART1.
         *         Transmit time at UART_BAUD is the thing that makes this
         *         task slow relative to sensor_task/filter_task — say so in
         *         REPORT.md with the actual byte time you computed. */
        int length = snprintf(line, sizeof(line), "%lu,%d,%d,%d\r\n",
        (unsigned long)in.seq,
        (int)in.ax,
        (int)in.ay,
        (int)in.az);

        if (length < 0 || (size_t)length >= sizeof(line)) {
            ++uart_format_errors;
            continue;
        }

        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (const uint8_t *)line, (uint16_t)length, 100U);

        if (status == HAL_OK) {
            ++uart_rows_sent;
        } 
        else {
            ++uart_tx_errors;
        }
    }
}
