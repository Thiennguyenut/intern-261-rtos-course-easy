/* Week 3 / Ex4 -- CPU load, deadlock, watchdog.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#include "main.h"
#include "params.h"
#include "runtime_stats.h"
#include "deadlock_demo.h"
#include "supervisor.h"

extern UART_HandleTypeDef huart1;

static void boot_msg(void)
{
    const char *msg = __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)
                      ? "\r\n*** BOOT: reset by IWDG ***\r\n"
                      : "\r\n*** BOOT: normal reset ***\r\n";

    HAL_UART_Transmit(&huart1, (const uint8_t *)msg, (uint16_t)strlen(msg), 100U);
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

static void load_task(void *argument)          /* bit 2: busy_wait */
{
    (void)argument;
    const uint32_t busy = 2U * (SystemCoreClock / 1000U);   /* 2 ms */

    for (;;) {
        uint32_t start = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - start) < busy) {
        }

        supervisor_checkin(SUPERVISOR_BIT_GAMMA);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void heartbeat_task(void *argument)     /* bit 3: nháy PA8 */
{
    (void)argument;
    for (;;) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
        supervisor_checkin(SUPERVISOR_BIT_DELTA);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    boot_msg();
    runtime_stats_timer_init();
    deadlock_demo_init();
    supervisor_init();

    /* TODO: xTaskCreate task_alpha, task_beta, supervisor_task,
     *       runtime_stats_task. Run deadlock_demo exactly as given first
     *       (step 2 of README) before touching task_alpha/task_beta. */
    if (xTaskCreate(task_alpha, "alpha", configMINIMAL_STACK_SIZE, NULL, 2U, NULL) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(task_beta, "beta", configMINIMAL_STACK_SIZE, NULL, 2U, NULL) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(runtime_stats_task, "stats", 512U, NULL, 1U, NULL) != pdPASS) {
        Error_Handler();
    }
        if (xTaskCreate(load_task, "load", configMINIMAL_STACK_SIZE, NULL, 2U, NULL) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(heartbeat_task, "hb", configMINIMAL_STACK_SIZE, NULL, 2U, NULL) != pdPASS) {
        Error_Handler();
    }

    if (xTaskCreate(supervisor_task, "sup", configMINIMAL_STACK_SIZE, NULL, 3U, NULL) != pdPASS) {
        Error_Handler();
    }
    
    vTaskStartScheduler();

    for (;;) {
    }
}
