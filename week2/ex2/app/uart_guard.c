/* Week 2 / Ex2 — four ways to serialize access to USART1 from WRITER_COUNT
 * tasks. Only one UART_GUARD_MODE is compiled at a time.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"
#include "dwt.h"

volatile uart_timing_stats_t g_uart_timing[WRITER_COUNT];

#if UART_GUARD_MODE == 1
static SemaphoreHandle_t s_uart_mutex;
#elif UART_GUARD_MODE == 3
static QueueHandle_t s_gatekeeper_queue;
static volatile uint32_t s_request_cycles[WRITER_COUNT];
#endif

static void uart_raw_send(const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while ((USART1->SR & USART_SR_TXE) == 0U) {
        }
        USART1->DR = (uint8_t)buf[i];
    }

    while ((USART1->SR & USART_SR_TC) == 0U) {
    }
}

#if UART_GUARD_MODE == 1 || UART_GUARD_MODE == 2 || UART_GUARD_MODE == 3
static void uart_timing_record(uint32_t id, uint32_t request, uint32_t acquired, uint32_t sent)
{
    volatile uart_timing_stats_t *stats = &g_uart_timing[id];
    const uint32_t count = stats->count;

    if (count >= UART_TIMING_SAMPLES_PER_WRITER) {
        return;
    }

    const uint32_t latency = (uint32_t)(sent - request);
    const uint32_t wait = (uint32_t)(acquired - request);
    const uint32_t send = (uint32_t)(sent - acquired);

    if (count == 0U) {
        stats->first_latency_cycles = latency;
        stats->first_wait_cycles = wait;
        stats->first_send_cycles = send;
    }

    if (latency > stats->max_latency_cycles) {
        stats->max_latency_cycles = latency;
    }

    if (wait > stats->max_wait_cycles) {
        stats->max_wait_cycles = wait;
    }

    if (send > stats->max_send_cycles) {
        stats->max_send_cycles = send;
    }

    stats->total_latency_cycles += latency;
    stats->total_wait_cycles += wait;
    stats->total_send_cycles += send;

    if ((count + 1U) == UART_TIMING_SAMPLES_PER_WRITER) {
        stats->stack_min_free_words = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    }

    /* Publish the completed sample only after its fields have been stored. */
    __DMB();
    stats->count = count + 1U;
}
#endif

void uart_guard_init(void)
{
#if UART_GUARD_MODE == 1
    /* TODO 1: xSemaphoreCreateMutex(). Check for NULL. */
    s_uart_mutex = xSemaphoreCreateMutex();
    configASSERT(s_uart_mutex != NULL);
#elif UART_GUARD_MODE == 3
    /* TODO 2: create s_gatekeeper_queue (length: pick and justify in
     *         REPORT.md), item size = MSG_LEN bytes.
     *         xTaskCreate(gatekeeper_task, ..., GATEKEEPER_PRIO). */
    s_gatekeeper_queue = xQueueCreate(WRITER_COUNT, MSG_LEN);
    configASSERT(s_gatekeeper_queue != NULL);

    BaseType_t status = xTaskCreate(gatekeeper_task, "Gatekeeper", 200U, NULL, GATEKEEPER_PRIO, NULL);
    configASSERT(status == pdPASS);
#endif
}


void uart_guard_send(const char *buf, size_t len)
{
#if UART_GUARD_MODE == 0
    /* The bug: no serialization at all. */
    /* TODO 3: HAL_UART_Transmit directly. */
    /* User-authorized deviation from TODO 3: register TX avoids shared HAL
     * transfer-state races. The whole message remains unprotected. */
    configASSERT(buf != NULL);
    uart_raw_send(buf, len);

#elif UART_GUARD_MODE == 1
    /* TODO 4: xSemaphoreTake(s_uart_mutex, portMAX_DELAY), transmit, Give.
     *         Every return path must Give — no early return between Take and
     *         Give. */
    configASSERT(buf != NULL);
    configASSERT(s_uart_mutex != NULL);
    configASSERT(len == MSG_LEN);
    configASSERT(buf[0] == 'W');

    const uint32_t id = (uint32_t)(buf[1] - '0');
    configASSERT(id < WRITER_COUNT);

    const uint32_t request = dwt_get_cycles();
    BaseType_t take_result = xSemaphoreTake(s_uart_mutex, portMAX_DELAY);
    const uint32_t acquired = dwt_get_cycles();

    configASSERT(take_result == pdTRUE);

    if (take_result == pdTRUE) {
        uart_raw_send(buf, len);
        const uint32_t sent = dwt_get_cycles();

        BaseType_t give_result = xSemaphoreGive(s_uart_mutex);
        configASSERT(give_result == pdTRUE);

        /* Bookkeeping is outside both the measured interval and mutex hold. */
        uart_timing_record(id, request, acquired, sent);
    }

#elif UART_GUARD_MODE == 2
    /* TODO 5: taskENTER_CRITICAL(), transmit, taskEXIT_CRITICAL().
     *         Measure in REPORT.md how long the critical section holds IRQs
     *         off, as a function of MSG_LEN and UART_BAUD. */
    configASSERT(buf != NULL);
    configASSERT(len == MSG_LEN);
    configASSERT(buf[0] == 'W');

    const uint32_t id = (uint32_t)(buf[1] - '0');
    configASSERT(id < WRITER_COUNT);

    const uint32_t request = dwt_get_cycles();

    taskENTER_CRITICAL();
    const uint32_t entered = dwt_get_cycles();

    uart_raw_send(buf, len);
    
    const uint32_t sent = dwt_get_cycles();

    taskEXIT_CRITICAL();

    uart_timing_record(id, request, entered, sent);

#elif UART_GUARD_MODE == 3
    /* TODO 6: xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY).
     *         The writer never touches UART directly in this mode. */
    configASSERT(buf != NULL);
    configASSERT(len == MSG_LEN);
    configASSERT(s_gatekeeper_queue != NULL);

    configASSERT(buf[0] == 'W');

    const uint32_t id = (uint32_t)(buf[1] - '0');
    configASSERT(id < WRITER_COUNT);

    s_request_cycles[id] = dwt_get_cycles();
    
    BaseType_t status = xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY);

    configASSERT(status == pdPASS);
#endif
}

#if UART_GUARD_MODE == 3
void gatekeeper_task(void *argument)
{
    (void)argument;
    for (;;) {
        char msg[MSG_LEN];
        /* TODO 7: xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY), then
         *         HAL_UART_Transmit. This is the only task allowed to call
         *         HAL_UART_Transmit in this mode. */
        BaseType_t status = xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY);
        configASSERT(status == pdPASS);

        if (status == pdPASS) {
            configASSERT(msg[0] == 'W');

            const uint32_t id = (uint32_t)(msg[1] - '0');
            configASSERT(id < WRITER_COUNT);

            const uint32_t request = s_request_cycles[id];
            const uint32_t acquired = dwt_get_cycles();

            uart_raw_send(msg, MSG_LEN);

            const uint32_t sent = dwt_get_cycles();

            uart_timing_record(id, request, acquired, sent);
        }
    }
}
#endif
