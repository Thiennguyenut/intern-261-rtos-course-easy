#include "stm32f1xx.h"
#include "dwt.h"

uint32_t g_dwt_clock_hz;

void dwt_init(void)
{
    g_dwt_clock_hz = SystemCoreClock;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t dwt_get_cycles(void)
{
    return DWT->CYCCNT;
}

void dwt_busy_wait_us(uint32_t microseconds)
{
    const uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    const uint32_t cycles_to_wait = microseconds * cycles_per_us;
    const uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles_to_wait) {
    }
}