/* Week 3 / Ex4 -- supervisor task: waits for all monitored tasks to check in
 * via an event group every cycle; refreshes IWDG only if they all did.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "params.h"
#include "supervisor.h"

EventGroupHandle_t supervisor_events;

extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart1;

#define ALL_BITS ((1u << SUPERVISOR_BITS) - 1u)

#define SUPERVISOR_WAIT_MS   (100U)

volatile TickType_t supervisor_max_gap_ms = 0;

void supervisor_init(void)
{
    /* TODO 1: supervisor_events = xEventGroupCreate(); check non-NULL.*/
    supervisor_events = xEventGroupCreate();
    configASSERT(supervisor_events != NULL);
    /* TODO 2: HAL IWDG init here (IWDG_TIMEOUT_MS -> prescaler + reload per
     *         RM0008 IWDG chapter; IWDG runs off LSI, ~40 kHz, compute the
     *         reload value and show the computation in REPORT.md). */
    MX_IWDG_Init();
}

void supervisor_task(void *argument)
{
    (void)argument;

    for (;;) {
        /* TODO 3: xEventGroupWaitBits(supervisor_events, ALL_BITS, pdTRUE,
         *         pdTRUE, <timeout>) -- clear-on-exit, wait-for-all. Pick a
         *         timeout tighter than IWDG_TIMEOUT_MS but loose enough that
         *         normal scheduling jitter does not cause a false trip;
         *         justify the number in REPORT.md.*/
        EventBits_t got = xEventGroupWaitBits(supervisor_events, ALL_BITS,
                                              pdTRUE,   /* xóa bit khi đủ */
                                              pdTRUE,   /* chờ ĐỦ tất cả bit */
                                              pdMS_TO_TICKS(SUPERVISOR_WAIT_MS));


        /* TODO 4: if the wait actually collected ALL_BITS (check the return
         *         value against ALL_BITS, do not assume success just because
         *         the call returned), HAL_IWDG_Refresh(&hiwdg). Otherwise do
         *         NOTHING -- that is what lets IWDG_TIMEOUT_MS expire and
         *         reset the board when a task stops checking in. */
        
        if ((got & ALL_BITS) == ALL_BITS) {
            static TickType_t last_ok = 0;
            TickType_t now = xTaskGetTickCount();
            if (last_ok != 0 && (now - last_ok) > supervisor_max_gap_ms) {
                supervisor_max_gap_ms = now - last_ok;
            }
            last_ok = now;
            HAL_IWDG_Refresh(&hiwdg);
        }
        // if ((got & ALL_BITS) == ALL_BITS) {
        //     HAL_IWDG_Refresh(&hiwdg);    /* mọi task còn sống -> reset bộ đếm IWDG */
        // }
        
        else {
            char line[48];
            snprintf(line, sizeof line, "WDT MISS t=%lu bits=0x%02lx\r\n",
                     (unsigned long)xTaskGetTickCount(), (unsigned long)(got & ALL_BITS));

            HAL_UART_Transmit(&huart1, (const uint8_t *)line, (uint16_t)strlen(line), 50U);
        }
    }
}
