#include <stdio.h>

#include "step_engine.h"

#define CHECK(expression)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expression); \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_validation(void)
{
    step_engine_t engine;
    step_engine_init(&engine);
    const step_segment_t invalid =
    {
        .first_step_time = 0U,
        .step_period_ticks = 4U,
        .step_count = 1U,
        .pulse_width_ticks = 4U,
        .direction_positive = true,
    };
    CHECK(!step_engine_enqueue(&engine, &invalid));
    return 0;
}

static int test_two_steps_across_wrap(void)
{
    step_engine_t engine;
    step_engine_init(&engine);
    const step_segment_t segment =
    {
        .first_step_time = UINT32_MAX - 2U,
        .step_period_ticks = 10U,
        .step_count = 2U,
        .pulse_width_ticks = 2U,
        .direction_positive = true,
    };
    CHECK(step_engine_enqueue(&engine, &segment));

    step_event_t event;
    CHECK(step_engine_start(&engine, UINT32_MAX - 10U, &event));
    CHECK(event.actions == STEP_ACTION_SET_DIRECTION);
    CHECK(event.direction_positive);
    CHECK(event.next_deadline == UINT32_MAX - 2U);

    CHECK(!step_engine_on_deadline(&engine, UINT32_MAX - 3U, &event));
    CHECK(event.actions == STEP_ACTION_NONE);

    CHECK(step_engine_on_deadline(&engine, UINT32_MAX - 2U, &event));
    CHECK(event.actions == STEP_ACTION_RAISE);
    CHECK(event.next_deadline == UINT32_MAX);

    CHECK(step_engine_on_deadline(&engine, UINT32_MAX, &event));
    CHECK(event.actions == STEP_ACTION_LOWER);
    CHECK(event.next_deadline == 7U);

    CHECK(step_engine_on_deadline(&engine, 7U, &event));
    CHECK(event.actions == STEP_ACTION_RAISE);
    CHECK(event.next_deadline == 9U);

    CHECK(step_engine_on_deadline(&engine, 9U, &event));
    CHECK((event.actions & STEP_ACTION_LOWER) != 0U);
    CHECK((event.actions & STEP_ACTION_SEGMENT_DONE) != 0U);
    CHECK((event.actions & STEP_ACTION_QUEUE_EMPTY) != 0U);
    CHECK(!event.has_deadline);
    return 0;
}

static int test_fifo_and_direction_change(void)
{
    step_engine_t engine;
    step_engine_init(&engine);
    const step_segment_t positive = {100U, 10U, 1U, 2U, true};
    const step_segment_t negative = {120U, 10U, 1U, 2U, false};
    CHECK(step_engine_enqueue(&engine, &positive));
    CHECK(step_engine_enqueue(&engine, &negative));
    CHECK(step_engine_queued(&engine) == 2U);

    step_event_t event;
    CHECK(step_engine_start(&engine, 90U, &event));
    CHECK(step_engine_on_deadline(&engine, 100U, &event));
    CHECK(step_engine_on_deadline(&engine, 102U, &event));
    CHECK((event.actions & STEP_ACTION_SEGMENT_DONE) != 0U);
    CHECK((event.actions & STEP_ACTION_SET_DIRECTION) != 0U);
    CHECK(!event.direction_positive);
    CHECK(event.next_deadline == 120U);
    return 0;
}

int main(void)
{
    if ((test_validation() != 0) ||
        (test_two_steps_across_wrap() != 0) ||
        (test_fifo_and_direction_change() != 0))
    {
        return 1;
    }

    (void)puts("step_engine_test: PASS");
    return 0;
}
