#ifndef DWT_H
#define DWT_H

#include <stdint.h>

extern uint32_t g_dwt_clock_hz;

void dwt_init(void);
uint32_t dwt_get_cycles(void);
void dwt_busy_wait_us(uint32_t microseconds);

#endif /* DWT_H */