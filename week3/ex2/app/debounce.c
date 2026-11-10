/* Week 3 / Ex2 -- debounce via one-shot timer + long-press via auto-reload
 * timer. Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "debounce.h"

static TimerHandle_t s_debounce_timer;
static TimerHandle_t s_longpress_timer;
static QueueHandle_t s_timer_log_queue;

static volatile uint32_t s_dropped_timer_logs;

static uint8_t s_button_stable_pressed = 0U;

extern UART_HandleTypeDef huart1;

typedef struct {
    const char *label;
    TickType_t tick;
} TimerLogEvent;

static void timer_log_post(const char *label, TickType_t tick)
{
    const TimerLogEvent event = {label, tick};

    if (xQueueSend(s_timer_log_queue, &event, 0) != pdPASS) {
        ++s_dropped_timer_logs;
    }
}

static void timer_log_task(void *argument)
{
    (void)argument;
    TimerLogEvent event;

    for (;;) {
        if (xQueueReceive(s_timer_log_queue, &event, portMAX_DELAY) == pdPASS) {
            char line[32];
            const int length = snprintf(line, sizeof(line), "%s %lu\r\n",
                                        event.label, (unsigned long)event.tick);

            if (length > 0 && (size_t)length < sizeof(line)) {
                HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)length, 100U);
            }
        }
    }
}

static void debounce_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 1: printf/HAL_UART "PRESS" + xTaskGetTickCount(). This is the
     *         stable-press event, DEBOUNCE_MS after the last edge.*/
    const TickType_t tick = xTaskGetTickCount();
    const uint8_t pressed = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_SET;
    if (pressed == s_button_stable_pressed) {
        return;
    }

    s_button_stable_pressed = pressed;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, pressed ? GPIO_PIN_SET : GPIO_PIN_RESET);

    if (pressed != 0U) {
    timer_log_post("PRESS", tick);
    }

    /* TODO 2 (temporary, for the README step-4 experiment ONLY): add
     *         vTaskDelay(pdMS_TO_TICKS(50)) here, capture what happens to
     *         longpress_timer's callback timing in your REPORT.md, then
     *         DELETE this line again before you submit. A callback that
     *         blocks here is blocking the ONE daemon task every other
     *         software timer in the system also depends on. */
    // if (pressed != 0U) {
    //     timer_log_post("BLOCK_BEGIN", tick);
    //     vTaskDelay(pdMS_TO_TICKS(50));
    //     timer_log_post("PRESS", xTaskGetTickCount());
    // }
}

static void longpress_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 3: printf/HAL_UART "HELD" + xTaskGetTickCount(). Fires once per
     *         LONGPRESS_MS while the button stays down. */
    timer_log_post("HELD", xTaskGetTickCount());
}

void debounce_init(void)
{
    /* TODO 4: s_debounce_timer = xTimerCreate(..., pdMS_TO_TICKS(DEBOUNCE_MS),
     *         pdFALSE, NULL, debounce_callback); -- pdFALSE = one-shot.
     *         s_longpress_timer = xTimerCreate(..., pdMS_TO_TICKS(LONGPRESS_MS),
     *         pdTRUE, NULL, longpress_callback); -- pdTRUE = auto-reload.
     *         Check both handles non-NULL. Do NOT xTimerStart() either here --
     *         they are driven entirely from debounce_on_edge(). */
    s_debounce_timer = xTimerCreate("debounce", pdMS_TO_TICKS(DEBOUNCE_MS), pdFALSE, NULL, debounce_callback);

    s_longpress_timer = xTimerCreate("longpress", pdMS_TO_TICKS(LONGPRESS_MS), pdTRUE, NULL, longpress_callback);

    if (s_debounce_timer == NULL || s_longpress_timer == NULL) {
        Error_Handler();
    }

    s_timer_log_queue = xQueueCreate(8U, sizeof(TimerLogEvent));
    if (s_timer_log_queue == NULL) {
        Error_Handler();
    }

    if (xTaskCreate(timer_log_task, "timer_log", 256U, NULL, tskIDLE_PRIORITY + 1U, NULL) != pdPASS) {
        Error_Handler();
    }
}

void debounce_on_edge(uint8_t pin_is_high)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* TODO 5: xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken)
     *         on every edge -- this is what swallows bounce: each new edge
     *         restarts the DEBOUNCE_MS countdown, so the callback only fires
     *         once the line has been quiet for DEBOUNCE_MS. */
    BaseType_t debounce_result = xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken);

    /* TODO 6: if (pin_is_high) xTimerStartFromISR(s_longpress_timer, ...);
     *         else xTimerStopFromISR(s_longpress_timer, ...); */
    BaseType_t longpress_result;
    if (pin_is_high != 0U) {
        longpress_result = xTimerStartFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
    } 
    else {
        longpress_result = xTimerStopFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
    }

    /* TODO 7: portYIELD_FROM_ISR(xHigherPriorityTaskWoken). */
    configASSERT(debounce_result == pdPASS && longpress_result == pdPASS);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
