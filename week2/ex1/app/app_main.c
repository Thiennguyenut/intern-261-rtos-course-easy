/* Week 2 / Ex1 — sensor pipeline: sensor_task -> queue_raw -> filter_task ->
 * queue_filtered -> uart_task.
 *
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

#define DEPTH_LOG_PERIOD_MS (5U)
#define DEPTH_LOG_COUNT     (1000U)

QueueHandle_t queue_raw;
QueueHandle_t queue_filtered;

typedef struct {
    TickType_t elapsed_ticks;
    uint16_t raw_depth;
    uint16_t filtered_depth;
} depth_record_t;

static volatile depth_record_t depth_records[DEPTH_LOG_COUNT];
static volatile uint32_t depth_record_count = 0U;
static volatile BaseType_t depth_capture_done = pdFALSE;

void app_main(void)
{
    /* TODO 1: xQueueCreate(QUEUE_LENGTH, sizeof(raw_sample_t)) for queue_raw,
     *         and the equivalent for queue_filtered with filtered_sample_t.
     *         Check both handles are non-NULL before continuing. */
    queue_raw = xQueueCreate(QUEUE_LENGTH, sizeof(raw_sample_t));
    configASSERT(queue_raw != NULL);

    queue_filtered = xQueueCreate(QUEUE_LENGTH, sizeof(filtered_sample_t));
    configASSERT(queue_filtered != NULL);

    /* TODO 2: xTaskCreate sensor_task, filter_task, uart_task.
     *         Pick priorities you can justify in REPORT.md: who must never be
     *         starved by whom? */
    BaseType_t result;

    result = xTaskCreate(sensor_task, "sensor", 128U, NULL, 3U, NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(filter_task, "filter", 128U, NULL, 2U, NULL);
    configASSERT(result == pdPASS);

    result = xTaskCreate(uart_task, "uart", 256U, NULL, 1U, NULL);
    configASSERT(result == pdPASS);
    /* TODO 3: xTaskCreate depth_logger_task — must not perturb the pipeline it
     *         is measuring (think about its own priority and period). */
    
    result = xTaskCreate(depth_logger_task, "depth", 128U, NULL, 2U, NULL);
    configASSERT(result == pdPASS);
    
    vTaskStartScheduler();

    for (;;) {
    }
}

void depth_logger_task(void *argument)
{
    (void)argument;

    TickType_t start_tick = xTaskGetTickCount();
    TickType_t last_wake = start_tick;
    TickType_t period = pdMS_TO_TICKS(DEPTH_LOG_PERIOD_MS);

    configASSERT(period > 0U);

    for (uint32_t i = 0U; i < DEPTH_LOG_COUNT; ++i) {
        depth_records[i].elapsed_ticks = xTaskGetTickCount() - start_tick;

        depth_records[i].raw_depth = (uint16_t)uxQueueMessagesWaiting(queue_raw);

        depth_records[i].filtered_depth = (uint16_t)uxQueueMessagesWaiting(queue_filtered);

        depth_record_count = i + 1U;

        vTaskDelayUntil(&last_wake, period);
    }

    depth_capture_done = pdTRUE;

    for (;;) {
        vTaskSuspend(NULL);
    }
}

void vApplicationMallocFailedHook(void)
{
    configASSERT(0);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                  char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    configASSERT(0);
}
