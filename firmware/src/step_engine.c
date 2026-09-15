#include "step_engine.h"

#include <stddef.h>

#include "motion_clock.h"

static void clear_event(step_event_t *event)
{
    *event = (step_event_t){0};
}

static bool load_next_segment(step_engine_t *engine, step_event_t *event)
{
    if (engine->count == 0U)
    {
        engine->phase = STEP_PHASE_IDLE;
        event->actions |= STEP_ACTION_QUEUE_EMPTY;
        event->has_deadline = false;
        return false;
    }

    engine->active = engine->queue[engine->head];
    engine->head = (uint8_t)((engine->head + 1U) % STEP_ENGINE_QUEUE_CAPACITY);
    --engine->count;
    engine->remaining_steps = engine->active.step_count;
    engine->next_deadline = engine->active.first_step_time;
    engine->phase = STEP_PHASE_WAITING_FOR_RISE;

    if ((engine->direction_positive != engine->active.direction_positive) ||
        (event->actions == STEP_ACTION_NONE))
    {
        engine->direction_positive = engine->active.direction_positive;
        event->actions |= STEP_ACTION_SET_DIRECTION;
        event->direction_positive = engine->direction_positive;
    }

    event->next_deadline = engine->next_deadline;
    event->has_deadline = true;
    return true;
}

void step_engine_init(step_engine_t *engine)
{
    if (engine != NULL)
    {
        *engine = (step_engine_t){0};
    }
}

bool step_segment_is_valid(const step_segment_t *segment)
{
    return (segment != NULL) &&
           (segment->step_count != 0U) &&
           (segment->step_period_ticks != 0U) &&
           (segment->pulse_width_ticks != 0U) &&
           (segment->pulse_width_ticks < segment->step_period_ticks);
}

bool step_engine_enqueue(step_engine_t *engine, const step_segment_t *segment)
{
    if ((engine == NULL) || (engine->count >= STEP_ENGINE_QUEUE_CAPACITY) ||
        !step_segment_is_valid(segment))
    {
        return false;
    }

    const uint8_t tail = (uint8_t)((engine->head + engine->count) % STEP_ENGINE_QUEUE_CAPACITY);
    engine->queue[tail] = *segment;
    ++engine->count;
    return true;
}

bool step_engine_start(step_engine_t *engine, uint32_t now, step_event_t *event)
{
    if ((engine == NULL) || (event == NULL) || (engine->phase != STEP_PHASE_IDLE))
    {
        return false;
    }

    clear_event(event);
    if (!load_next_segment(engine, event))
    {
        return false;
    }

    if (motion_clock_reached(now, engine->next_deadline))
    {
        event->late_by_ticks = (uint32_t)motion_clock_delta(now, engine->next_deadline);
    }
    return true;
}

bool step_engine_on_deadline(step_engine_t *engine, uint32_t now, step_event_t *event)
{
    if ((engine == NULL) || (event == NULL) || (engine->phase == STEP_PHASE_IDLE))
    {
        return false;
    }

    clear_event(event);
    if (!motion_clock_reached(now, engine->next_deadline))
    {
        event->next_deadline = engine->next_deadline;
        event->has_deadline = true;
        return false;
    }

    event->late_by_ticks = (uint32_t)motion_clock_delta(now, engine->next_deadline);
    if (engine->phase == STEP_PHASE_WAITING_FOR_RISE)
    {
        event->actions = STEP_ACTION_RAISE;
        engine->next_deadline += engine->active.pulse_width_ticks;
        engine->phase = STEP_PHASE_WAITING_FOR_FALL;
    }
    else
    {
        event->actions = STEP_ACTION_LOWER;
        --engine->remaining_steps;
        if (engine->remaining_steps == 0U)
        {
            event->actions |= STEP_ACTION_SEGMENT_DONE;
            (void)load_next_segment(engine, event);
            return true;
        }

        engine->next_deadline += engine->active.step_period_ticks - engine->active.pulse_width_ticks;
        engine->phase = STEP_PHASE_WAITING_FOR_RISE;
    }

    event->next_deadline = engine->next_deadline;
    event->has_deadline = true;
    return true;
}

uint8_t step_engine_queued(const step_engine_t *engine)
{
    return (engine == NULL) ? 0U : engine->count;
}
