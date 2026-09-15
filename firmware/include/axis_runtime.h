#ifndef OPENPNP_AXIS_RUNTIME_H
#define OPENPNP_AXIS_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "step_engine.h"

typedef enum
{
    AXIS_RUNTIME_OK = 0,
    AXIS_RUNTIME_DISABLED,
    AXIS_RUNTIME_QUEUE_FULL,
    AXIS_RUNTIME_INVALID_SEGMENT,
    AXIS_RUNTIME_DEADLINE_OVERRUN
} axis_runtime_result_t;

typedef struct
{
    uint32_t emitted_steps;
    uint32_t completed_segments;
    uint32_t late_events;
    uint32_t maximum_lateness_ticks;
    uint32_t fault;
    uint8_t queued_segments;
    bool enabled;
    bool step_high;
    bool direction_positive;
} axis_runtime_diagnostics_t;

extern volatile axis_runtime_diagnostics_t g_axis_runtime_diagnostics;

void axis_runtime_init(void);
void axis_runtime_enable(bool enable);
axis_runtime_result_t axis_runtime_enqueue(const step_segment_t *segment);
void axis_runtime_emergency_stop(void);

#endif
