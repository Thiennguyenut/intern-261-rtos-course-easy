/* Week 3 / Ex3 -- UART RX -> stream buffer -> parser.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"
#include "parser_task.h"

void app_main(void)
{
    uart_rx_init();

    /* TODO: xTaskCreate parser_task and demo_task. parser_task must run at a
     *       priority high enough that it drains rx_stream faster than bytes
     *       arrive at RX_BAUD -- justify the number in REPORT.md with the
     *       byte time you computed. */

    if (xTaskCreate(demo_task, "demo", configMINIMAL_STACK_SIZE, NULL, 1U, NULL) != pdPASS) {
        Error_Handler();
        return;
    }

    if (xTaskCreate(parser_task, "parser", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1U, NULL) != pdPASS) {
        Error_Handler();
        return;
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
