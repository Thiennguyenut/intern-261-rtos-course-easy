/* Week 1 / Ex3 — periodic task: vTaskDelay vs xTaskDelayUntil.
 *
 * Build this file twice over the exercise, switching DELAY_MODE, and twice more
 * with configTICK_RATE_HZ changed in FreeRTOSConfig.h. Four runs total.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

void dwt_busy_wait_us(uint32_t microseconds);

/* 0 = vTaskDelay, 1 = xTaskDelayUntil */
#ifndef DELAY_MODE
#define DELAY_MODE 1
#endif

/* Fixed seed: the variable load must be reproducible across all four runs,
 * otherwise the four configurations are not comparable. */
static uint32_t s_rand_state = 0x12345678u;

static uint32_t next_work_us(void)
{
    /* TODO 1: xorshift32 on s_rand_state, mapped into [0, WORK_JITTER_US].
     *         Must give the identical sequence on every run. */
    uint32_t x = s_rand_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    s_rand_state = x;

    return x % (WORK_JITTER_US + 1U);
}

#if DELAY_MODE == 1
volatile uint32_t checked_cycles = 0U;
volatile uint32_t no_block_count = 0U;
#endif

void periodic_task(void *argument)
{
    (void)argument;

#if DELAY_MODE == 1
    /* TODO 2: declare and initialise the previous-wake-time variable.
     *         Initialising it in the wrong place is the classic mistake here —
     *         if your drift is not ~0, look at this first and say so in REPORT.md. */
    TickType_t previous_wake_time = xTaskGetTickCount();
    
#endif

    for (;;) {
        /* TODO 3: toggle PA<PERIODIC_PIN>, then burn next_work_us() of CPU
         *         with the DWT busy-wait from ex2. */
        HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1U << PERIODIC_PIN));

        const uint32_t work_us = next_work_us();
        dwt_busy_wait_us(work_us);
#if DELAY_MODE == 0
        /* TODO 4a: vTaskDelay(...) — what exactly do you pass, and relative to what? */
        vTaskDelay(pdMS_TO_TICKS(PERIOD_MS));
#else
    BaseType_t was_delayed = xTaskDelayUntil(
        &previous_wake_time,
        pdMS_TO_TICKS(PERIOD_MS)
    );

    if (checked_cycles < SAMPLE_COUNT) {
        if (was_delayed == pdFALSE) {
            no_block_count++;
        }

        checked_cycles++;
    }
#endif
    }
}
