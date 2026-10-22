/* Week 2 / Ex1 — moving-average filter, window = FILTER_WINDOW samples.
 * queue_raw -> queue_filtered.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

static volatile uint32_t filter_output_dropped = 0U;

void filter_task(void *argument)
{
    (void)argument;

    /* TODO 1: ring buffers for ax/ay/az, size FILTER_WINDOW, plus running sums.
     *         Do NOT recompute the full sum every sample — that is the whole
     *         point of a moving-average window. */
    static int16_t history_x[FILTER_WINDOW] = {0};
    static int16_t history_y[FILTER_WINDOW] = {0};
    static int16_t history_z[FILTER_WINDOW] = {0};

    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;

    uint32_t index = 0U;
    uint32_t sample_count = 0U;

    for (;;) {
        raw_sample_t in;

        /* TODO 2: xQueueReceive(queue_raw, &in, portMAX_DELAY). */
        BaseType_t receive_result = xQueueReceive(queue_raw, &in, portMAX_DELAY);
        
        if (receive_result != pdPASS) {
            continue;
        }

        /* TODO 3: push into the ring buffer, update running sums, compute the
         *         averaged ax/ay/az. */
        sum_x -= history_x[index];
        history_x[index] = in.ax;
        sum_x += history_x[index];

        sum_y -= history_y[index];
        history_y[index] = in.ay;
        sum_y += history_y[index];

        sum_z -= history_z[index];
        history_z[index] = in.az;
        sum_z += history_z[index];

        if (sample_count < FILTER_WINDOW) {
            ++sample_count;
        }

        index = (index + 1U) % FILTER_WINDOW;

        filtered_sample_t out;
        out.seq = in.seq;
        /* TODO 4: fill out.ax/ay/az from the running average. */
        out.ax = (int16_t)(sum_x / (int32_t)sample_count);
        out.ay = (int16_t)(sum_y / (int32_t)sample_count);
        out.az = (int16_t)(sum_z / (int32_t)sample_count);  
        /* TODO 5: xQueueSend(queue_filtered, &out, <timeout>) — same question
         *         as sensor_task: what should happen here if uart_task is
         *         slow and queue_filtered is full? */

        BaseType_t send_result = xQueueSend(queue_filtered, &out, 0U);

        if (send_result != pdPASS) {
            ++filter_output_dropped;
        }

    }
}
