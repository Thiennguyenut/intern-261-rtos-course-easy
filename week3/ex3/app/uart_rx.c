/* Week 3 / Ex3 -- UART RX interrupt -> stream buffer.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"

StreamBufferHandle_t rx_stream;

static uint8_t s_rx_byte;
static volatile uint32_t s_rx_dropped_bytes;

extern UART_HandleTypeDef huart1;

void uart_rx_init(void)
{
    /* TODO 1: rx_stream = xStreamBufferCreate(STREAM_BUFFER_BYTES,
     *         TRIGGER_LEVEL). Check non-NULL. */
    // cau 6
    //rx_stream = xStreamBufferCreate(STREAM_BUFFER_BYTES * 2U, TRIGGER_LEVEL);
    rx_stream = xStreamBufferCreate(STREAM_BUFFER_BYTES, TRIGGER_LEVEL);

    if (rx_stream == NULL) {
        Error_Handler();
        return;
    }

    /* TODO 2: HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1) to arm the first
     *         byte. huart1 must already be MX_USART1_UART_Init()'d at
     *         RX_BAUD -- that part lives in template/, not here. */
     if (HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U) != HAL_OK) {
        Error_Handler();
    }
}

void uart_rx_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* TODO 3: xStreamBufferSendFromISR(rx_stream, &s_rx_byte, 1,
     *         &xHigherPriorityTaskWoken). Check the return value: what do you
     *         do with the byte if the stream buffer is full and it returns 0?
     *         Answer this in PREDICTION.md, not just here. */
    if (xStreamBufferSendFromISR(rx_stream, &s_rx_byte, 1U, &xHigherPriorityTaskWoken) == 0U) {
        s_rx_dropped_bytes++;
    } 
    else if (s_rx_byte == '\r'){
        xStreamBufferSendCompletedFromISR(rx_stream, &xHigherPriorityTaskWoken);
    }

    /* TODO 4: HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1) again -- re-arm for
     *         the next byte. Forgetting this is the single most common way
     *         to silently stop receiving after the first byte. */
    if (HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U) != HAL_OK) {
        Error_Handler();
    }
    /* TODO 5: portYIELD_FROM_ISR(xHigherPriorityTaskWoken). */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
