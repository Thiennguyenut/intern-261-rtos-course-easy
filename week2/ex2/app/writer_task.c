/* Week 2 / Ex2 — WRITER_COUNT identical writer tasks. Each prints an MSG_LEN
 * byte string with its own id + sequence number, every WRITER_PERIOD_MS.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdint.h>

#include "main.h"
#include "params.h"
#include "uart_guard.h"
#include "dwt.h"

typedef struct {
    uint32_t count;
    uint32_t max_delay_cycles;
} writer_delay_stats_t;

volatile writer_delay_stats_t g_writer_delay[WRITER_COUNT];
/* TODO 1: one writer_task(void *argument) taking the writer index via the
 *         task parameter (argument), NOT WRITER_COUNT copy-pasted bodies.
 *         Format a MSG_LEN-byte string containing the writer id and a
 *         per-writer sequence number, then uart_guard_send() it, then
 *         vTaskDelay(pdMS_TO_TICKS(WRITER_PERIOD_MS)). */
void writer_task(void *argument)
{
    uint32_t id = (uint32_t)(uintptr_t)argument;
    uint32_t seq = 0;
    char msg[MSG_LEN + 1];

    configASSERT(id < WRITER_COUNT);

    for (;;) {
        /* TODO 2 */
        (void)seq;
        int header_len = snprintf(msg, sizeof(msg), "W%lu:%06lu:", (unsigned long)id, (unsigned long)seq);

        configASSERT(header_len == 10);

        for (size_t i = (size_t)header_len; i < MSG_LEN - 2; i++) {
            msg[i] = (char)('A' + id);
        }

        msg[MSG_LEN - 2] = '\r';
        msg[MSG_LEN - 1] = '\n';
        msg[MSG_LEN] = '\0';

        uart_guard_send(msg, MSG_LEN);

        seq++;
        if (seq == 1000000U) {
            seq = 0;
        }
        

        const uint32_t before_delay = dwt_get_cycles();

        vTaskDelay(pdMS_TO_TICKS(WRITER_PERIOD_MS));

        const uint32_t after_delay = dwt_get_cycles();

        volatile writer_delay_stats_t *stats = &g_writer_delay[id];
        const uint32_t count = stats->count;

        if (count < UART_TIMING_SAMPLES_PER_WRITER) {
            const uint32_t elapsed =
                (uint32_t)(after_delay - before_delay);

            if (elapsed > stats->max_delay_cycles) {
                stats->max_delay_cycles = elapsed;
            }

            stats->count = count + 1U;
        }
    }
}
