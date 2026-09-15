#ifndef OPENPNP_STEP_ENGINE_H
#define OPENPNP_STEP_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#define STEP_ENGINE_QUEUE_CAPACITY 8U

typedef struct
{
    uint32_t first_step_time;
    uint32_t step_period_ticks;
    uint32_t step_count;
    uint32_t pulse_width_ticks;
    bool direction_positive;
} step_segment_t;

typedef enum
{
    STEP_ACTION_NONE = 0,
    STEP_ACTION_SET_DIRECTION = 1U << 0,
    STEP_ACTION_RAISE = 1U << 1,
    STEP_ACTION_LOWER = 1U << 2,
    STEP_ACTION_SEGMENT_DONE = 1U << 3,
    STEP_ACTION_QUEUE_EMPTY = 1U << 4
} step_action_t;

typedef struct
{
    uint32_t actions;
    uint32_t next_deadline;
    uint32_t late_by_ticks;
    bool direction_positive;
    bool has_deadline;
} step_event_t;

typedef enum
{
    STEP_PHASE_IDLE = 0,
    STEP_PHASE_WAITING_FOR_RISE,
    STEP_PHASE_WAITING_FOR_FALL
} step_phase_t;

typedef struct
{
    step_segment_t queue[STEP_ENGINE_QUEUE_CAPACITY];
    step_segment_t active;
    uint32_t next_deadline;
    uint32_t remaining_steps;
    uint8_t head;
    uint8_t count;
    step_phase_t phase;
    bool direction_positive;
} step_engine_t;

void step_engine_init(step_engine_t *engine);
bool step_engine_enqueue(step_engine_t *engine, const step_segment_t *segment);
bool step_engine_start(step_engine_t *engine, uint32_t now, step_event_t *event);
bool step_engine_on_deadline(step_engine_t *engine, uint32_t now, step_event_t *event);
uint8_t step_engine_queued(const step_engine_t *engine);

#endif
