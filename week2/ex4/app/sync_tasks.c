/* Week 2 / Ex4 — event group sync across EVENT_BIT_COUNT producers, with
 * producer_0's signal optionally replaced by a task notification.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "main.h"
#include "params.h"
#include "sync_tasks.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;

static EventGroupHandle_t s_events;
static volatile uint32_t s_signal_cycles[EX4_PRODUCER_COUNT];
/*static volatile uint32_t s_signal_cycles[EVENT_BIT_COUNT];*/
static volatile uint32_t s_wake_total_cycles;
static volatile uint32_t s_wake_samples;


void *sync_task_handle;

/*#define ALL_BITS_MASK ((1u << EVENT_BIT_COUNT) - 1u)*/
#define ALL_BITS_MASK ((1u << EX4_PRODUCER_COUNT) - 1u)
#define TEST_TIMEOUT 0

void sync_init(void)
{
    /* TODO 1: xEventGroupCreate(). Check for NULL.
     *         Measure and record in REPORT.md the sizeof the structure this
     *         allocates (see event_groups.c in the kernel sources) to answer
     *         PREDICTION.md question 1. */
    s_events = xEventGroupCreate();
    configASSERT(s_events != NULL);
}

void producer_task(void *argument)
{
    uint32_t idx = (uint32_t)(uintptr_t)argument;
    uint32_t rng_state = 0x12345678U ^ (idx + 1U);
    
#if !PRODUCER0_USE_NOTIFY
    uint32_t recorded_sample = UINT32_MAX;
#endif

    for (;;) {
        /* TODO 2: random small period (reuse a simple xorshift like ex3's
         *         periodic task, or vTaskDelay with a bounded random jitter). */
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;

        uint32_t delay_ms = 1U + (rng_state % 5U);
        
#if PRODUCER0_USE_NOTIFY
        if (idx == 0U) {
            delay_ms = NOTIFY_PERIOD_MS;
        }
#endif

#if TEST_TIMEOUT
        if (idx == 3U){
            delay_ms = SYNC_TIMEOUT_MS + 50U;
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(delay_ms));

#if PRODUCER0_USE_NOTIFY
        if (idx == 0) {
            /* TODO 3: xTaskNotifyGive((TaskHandle_t)sync_task_handle).
             *         Toggle a DWT-timestamped GPIO edge right before this
             *         call — this is your t0 for the wake-up latency
             *         measurement. */
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
            s_signal_cycles[idx] = DWT->CYCCNT;

            xTaskNotifyGive((TaskHandle_t)sync_task_handle);

        } else
#endif
        {
            /* TODO 4: xEventGroupSetBits(s_events, 1u << idx).
             *         Same timestamp discipline as TODO 3 for producers that
             *         still use the event group. */
            HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1U << idx));
            
            #if PRODUCER0_USE_NOTIFY
                s_signal_cycles[idx] = DWT->CYCCNT;
            #else
                if ((idx != 1U) || (recorded_sample != s_wake_samples)){
                    recorded_sample = s_wake_samples;
                    s_signal_cycles[idx] = DWT->CYCCNT;
                }
            #endif

            xEventGroupSetBits(s_events, (EventBits_t)(1U << idx));
        }
    }
}

void sync_task(void *argument)
{
    (void)argument;

    char line[96];
    uint32_t previous_sync_cycles = 0U;
    BaseType_t have_previous_sync = pdFALSE;

    for (;;) {
        BaseType_t sync_ok = pdFALSE;
        EventBits_t received_bits = 0U;
        TickType_t sync_tick = 0U;
        uint32_t sync_cycles = 0U;
        uint32_t signal_cycles = 0U;
        BaseType_t sample_valid = pdFALSE;

#if PRODUCER0_USE_NOTIFY
        /* TODO 5: ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS)) for
         *         producer_0's contribution, THEN xEventGroupWaitBits for the
         *         remaining producers' bits (mask without bit 0), clear-on-exit
         *         = pdTRUE, wait-for-all = pdTRUE, same timeout.
         *         Toggle a GPIO edge the moment both are satisfied — your t1
         *         for the latency measurement. */
        const EventBits_t remaining_mask = (EventBits_t)(ALL_BITS_MASK & ~1U);

        uint32_t previous_signal_cycles = s_signal_cycles[0];

        uint32_t notification_count = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));

        signal_cycles = s_signal_cycles[0];

        sample_valid =(notification_count > 0U) && (signal_cycles != previous_signal_cycles);

        received_bits = xEventGroupWaitBits(s_events, remaining_mask, pdTRUE, pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));

        if ((notification_count > 0U) &&((received_bits & remaining_mask) == remaining_mask)) {
            sync_ok = pdTRUE;
        }
#else
        /* TODO 6: xEventGroupWaitBits(s_events, ALL_BITS_MASK, pdTRUE, pdTRUE,
         *         pdMS_TO_TICKS(SYNC_TIMEOUT_MS)).
         *         Check the return value against ALL_BITS_MASK to tell
         *         "all bits arrived" apart from "timed out with some bits
         *         missing" — do not assume a non-zero return means success. */
        received_bits = xEventGroupWaitBits(s_events, (EventBits_t)ALL_BITS_MASK, pdTRUE, pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));

        signal_cycles = s_signal_cycles[1];
        sample_valid = pdTRUE;

        if ((received_bits & ALL_BITS_MASK) == ALL_BITS_MASK) {
            sync_ok = pdTRUE;
        }
#endif
        int length;

        if (sync_ok == pdTRUE)
        {
            sync_cycles = DWT->CYCCNT;  /* t1 */
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
            sync_tick = xTaskGetTickCount();

            if ((sample_valid == pdTRUE) && (s_wake_samples < 5U))
            {
                s_wake_total_cycles +=
                    (uint32_t)(sync_cycles - signal_cycles);

                s_wake_samples++;
            }
        /* TODO 7: print the sync instant and measured period to UART. */
            uint32_t sync_ms = (uint32_t)(((uint64_t)sync_tick * 1000U) / configTICK_RATE_HZ);

            uint32_t period_us = 0U;

            if (have_previous_sync == pdTRUE)
            {
                uint32_t period_cycles = (uint32_t)(sync_cycles - previous_sync_cycles);

                period_us = period_cycles / (SystemCoreClock / 1000000U);
            }

            length = snprintf(
                line, sizeof(line),
                "SYNC,t_ms=%lu,period_us=%lu,period_valid=%u\r\n",
                (unsigned long)sync_ms,
                (unsigned long)period_us,
                (unsigned int)(have_previous_sync == pdTRUE)
            );

            previous_sync_cycles = sync_cycles;
            have_previous_sync = pdTRUE;
        }
        else{
            #if PRODUCER0_USE_NOTIFY
                length = snprintf(
                    line, sizeof(line),
                    "TIMEOUT,bits=0x%lX,notify=%lu\r\n",
                    (unsigned long)received_bits,
                    (unsigned long)notification_count
                );
            #else
                length = snprintf(
                    line, sizeof(line),
                    "TIMEOUT,bits=0x%lX\r\n",
                    (unsigned long)received_bits
                );
            #endif
        }

        configASSERT(length > 0 && (size_t)length < sizeof(line));

        HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, (const uint8_t *)line, (uint16_t)length, 100U);

        configASSERT(status == HAL_OK);
    }
}