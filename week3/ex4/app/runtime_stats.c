/* Week 3 / Ex4 -- run-time stats on DWT CYCCNT.
 * Native FreeRTOS API only.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "runtime_stats.h"
#include "semphr.h"

extern UART_HandleTypeDef huart1;

SemaphoreHandle_t deadlock_demo_mutex_a(void);
SemaphoreHandle_t deadlock_demo_mutex_b(void);

extern volatile TickType_t supervisor_max_gap_ms;

static void uart_puts(const char *s)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)s, (uint16_t)strlen(s), 200U);
}

void runtime_stats_timer_init(void)
{
    /* TODO 1: same DWT enable sequence as week1/ex2 dwt_init() (CoreDebug
     *         DEMCR TRCENA bit, zero DWT->CYCCNT, DWT->CTRL CYCCNTENA bit).
     *         If you already have week1/ex2's dwt_init(), the sequence is
     *         identical -- do not invent a second mechanism. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* bật khối trace/DWT */
    DWT->CYCCNT = 0U;                                  
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;               /* cho CYCCNT chạy */
}

uint32_t runtime_stats_timer_value(void)
{
    /* TODO 2: return DWT->CYCCNT. This is portGET_RUN_TIME_COUNTER_VALUE()'s
     *         implementation -- wire it up in FreeRTOSConfig.deltas. */
    return DWT->CYCCNT;
}

static void dump_system_state(void)
{
    static TaskStatus_t st[10];
    char line[64];
    UBaseType_t n = uxTaskGetSystemState(st, 10, NULL);

    for (UBaseType_t i = 0; i < n; i++) {
        snprintf(line, sizeof line, "%s state=%d rt=%lu\r\n", st[i].pcTaskName,
                 (int)st[i].eCurrentState, (unsigned long)st[i].ulRunTimeCounter);
        uart_puts(line);
    }

    TaskHandle_t ha = xSemaphoreGetMutexHolder(deadlock_demo_mutex_a());
    TaskHandle_t hb = xSemaphoreGetMutexHolder(deadlock_demo_mutex_b());
    snprintf(line, sizeof line, "A held by %s, B held by %s\r\n",
             ha ? pcTaskGetName(ha) : "none", hb ? pcTaskGetName(hb) : "none");
    uart_puts(line);

    // cau_6
    snprintf(line, sizeof line, "max refresh gap=%lu ms\r\n", (unsigned long)supervisor_max_gap_ms);
    uart_puts(line);
}

void runtime_stats_task(void *argument)
{
    (void)argument;
    static char buf[512];

    for (;;) {
        /* TODO 3: vTaskGetRunTimeStats(buf) and print buf over UART.
         *         Note: this walks every task's TCB -- it is itself not free,
         *         factor that into why STATS_WINDOW_MS should not be too
         *         short. */
        char head[48];
        snprintf(head, sizeof head, "\r\n==== t=%lu ms ====\r\n", (unsigned long)xTaskGetTickCount());
        
        uart_puts(head);

        vTaskGetRunTimeStats(buf);   /* điền bảng: Tên  thời_gian  % */
        
        uart_puts(buf);
        dump_system_state();
        vTaskDelay(pdMS_TO_TICKS(STATS_WINDOW_MS));
    }
}
