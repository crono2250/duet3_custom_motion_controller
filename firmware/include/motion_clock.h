#ifndef OPENPNP_MOTION_CLOCK_H
#define OPENPNP_MOTION_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

#define MOTION_CLOCK_HZ 750000UL
#define MOTION_CLOCK_MIN_LEAD_TICKS 3UL

typedef void (*motion_clock_callback_t)(uint32_t now, void *context);

static inline int32_t motion_clock_delta(uint32_t lhs, uint32_t rhs)
{
    return (int32_t)(lhs - rhs);
}

static inline bool motion_clock_reached(uint32_t now, uint32_t deadline)
{
    return motion_clock_delta(now, deadline) >= 0;
}

void motion_clock_init(void);
uint32_t motion_clock_now(void);
bool motion_clock_schedule(uint32_t deadline, motion_clock_callback_t callback, void *context);
void motion_clock_cancel(void);
void motion_clock_irq_handler(void);

#endif
