#include <stdio.h>

#include "motion_gateway.h"

#define CHECK(expression)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expression); \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static motion_gateway_command_t valid_command(void)
{
    const motion_gateway_command_t command =
    {
        .first_step_time_master = UINT32_MAX - 20U,
        .step_period_ticks = 10U,
        .step_count = 3U,
        .pulse_width_ticks = 2U,
        .direction_positive = true,
    };
    return command;
}

static int test_requires_clock_lock(void)
{
    clock_sync_state_t clock = {0};
    const motion_gateway_command_t command = valid_command();
    step_segment_t segment;
    CHECK(motion_gateway_prepare_segment(&clock, &command, &segment) ==
          MOTION_GATEWAY_CLOCK_NOT_LOCKED);
    return 0;
}

static int test_translates_master_deadline_across_wrap(void)
{
    clock_sync_state_t clock =
    {
        .local_time_offset = 50U,
        .sample_count = CLOCK_SYNC_REQUIRED_SAMPLES,
    };
    const motion_gateway_command_t command = valid_command();
    step_segment_t segment;
    CHECK(motion_gateway_prepare_segment(&clock, &command, &segment) == MOTION_GATEWAY_OK);
    CHECK(segment.first_step_time == 29U);
    CHECK(segment.step_period_ticks == command.step_period_ticks);
    CHECK(segment.step_count == command.step_count);
    CHECK(segment.pulse_width_ticks == command.pulse_width_ticks);
    CHECK(segment.direction_positive);
    return 0;
}

static int test_rejects_invalid_pulse(void)
{
    clock_sync_state_t clock = {.sample_count = CLOCK_SYNC_REQUIRED_SAMPLES};
    motion_gateway_command_t command = valid_command();
    command.pulse_width_ticks = command.step_period_ticks;
    step_segment_t segment;
    CHECK(motion_gateway_prepare_segment(&clock, &command, &segment) ==
          MOTION_GATEWAY_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    if ((test_requires_clock_lock() != 0) ||
        (test_translates_master_deadline_across_wrap() != 0) ||
        (test_rejects_invalid_pulse() != 0))
    {
        return 1;
    }

    (void)puts("motion_gateway_test: PASS");
    return 0;
}
