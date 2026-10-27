#ifndef UART_GUARD_H
#define UART_GUARD_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"

/*WRITER TANG GAP DOI*/
#define UART_WRITER_MULTIPLIER 2U
#define UART_ACTIVE_WRITER_COUNT (WRITER_COUNT * UART_WRITER_MULTIPLIER)

/* 0 = none (the bug), 1 = mutex, 2 = critical section, 3 = gatekeeper task.
 * Build this file four times over the exercise, once per mode. */
#ifndef UART_GUARD_MODE
#define UART_GUARD_MODE 3
#endif

/* Measurement setting, not a generated personal parameter. */
#define UART_TIMING_SAMPLES_PER_WRITER 512U

typedef struct {
    uint32_t count;
    uint32_t first_latency_cycles;
    uint32_t max_latency_cycles;
    uint32_t first_wait_cycles;
    uint32_t max_wait_cycles;
    uint32_t first_send_cycles;
    uint32_t max_send_cycles;
    uint32_t stack_min_free_words;
    uint64_t total_latency_cycles;
    uint64_t total_wait_cycles;
    uint64_t total_send_cycles;
} uart_timing_stats_t;

/* Writers update their own rows in modes 1/2.
 * Gatekeeper updates all rows in mode 3.
 * Read after all counts reach the sample limit. */
extern volatile uart_timing_stats_t g_uart_timing[];

void uart_guard_init(void);

/* Blocking: sends `len` bytes out USART1 under whatever UART_GUARD_MODE picks.
 * In gatekeeper mode this enqueues to the gatekeeper instead of transmitting
 * directly — same call site either way. */
void uart_guard_send(const char *buf, size_t len);

#if UART_GUARD_MODE == 3
void gatekeeper_task(void *argument);
#endif

#endif /* UART_GUARD_H */
