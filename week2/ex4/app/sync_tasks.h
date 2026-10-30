#ifndef SYNC_TASKS_H
#define SYNC_TASKS_H

#include <stdint.h>
#include "params.h"

#define EX4_EXTRA_PRODUCERS 0U
#define EX4_PRODUCER_COUNT (EVENT_BIT_COUNT + EX4_EXTRA_PRODUCERS)

/* Bit i of the event group corresponds to producer_i.
 * producer_0 is the one swapped to task notification in step 4 — guard it
 * with PRODUCER0_USE_NOTIFY so the two configurations build from the same
 * source. */
#ifndef PRODUCER0_USE_NOTIFY
#define PRODUCER0_USE_NOTIFY 0
#endif

void sync_init(void);

void producer_task(void *argument); /* argument = producer index 0..EVENT_BIT_COUNT-1 */
void sync_task(void *argument);

/* Handle of sync_task, needed by producer_0 to call xTaskNotifyGive when
 * PRODUCER0_USE_NOTIFY is set. */
extern void *sync_task_handle;

#endif /* SYNC_TASKS_H */
