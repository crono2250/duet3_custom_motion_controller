#include <stdio.h>

#include "clock_sync.h"

#define CHECK(expression)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
        {                                                                                          \
            (void)fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expression); \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_wrap_helpers(void)
{
    CHECK(clock_sync_fdcan_delay_ticks(0x0010U, 0xFFF0U) == 24U);
    CHECK(motion_clock_reached(3U, UINT32_MAX - 2U));
    CHECK(!motion_clock_reached(UINT32_MAX - 3U, 3U));
    return 0;
}

static int test_lock_and_timeout(void)
{
    clock_sync_state_t state;
    clock_sync_init(&state);

    const uint32_t expected_offset = 500U;
    const uint32_t acknowledge_delay = 20U;
    uint32_t previous_master = 0U;

    for (uint32_t index = 0U; index < CLOCK_SYNC_REQUIRED_SAMPLES; ++index)
    {
        const uint32_t master = 100000U + (index * 1000U);
        const uint32_t local_at_receive = master + acknowledge_delay + expected_offset;
        const clock_sync_message_t message =
        {
            .time_sent = master,
            .last_time_sent = previous_master,
            .last_time_acknowledge_delay = (index == 0U) ? 0U : acknowledge_delay,
        };

        const clock_sync_result_t result = clock_sync_observe(&state,
                                                               &message,
                                                               local_at_receive + 30U,
                                                               1040U,
                                                               1000U,
                                                               index * 100U);
        if (index == 0U)
        {
            CHECK(result == CLOCK_SYNC_SAMPLE_PENDING);
        }
        previous_master = master;
    }

    CHECK(clock_sync_is_locked(&state));
    CHECK(state.local_time_offset == expected_offset);
    CHECK(state.peak_receive_delay == 30U);
    CHECK(clock_sync_to_master_time(&state, 123456U) == 122956U);
    CHECK(!clock_sync_check_timeout(&state, state.last_sync_millis + CLOCK_SYNC_TIMEOUT_MS));
    CHECK(clock_sync_check_timeout(&state, state.last_sync_millis + CLOCK_SYNC_TIMEOUT_MS + 1U));
    CHECK(!clock_sync_is_locked(&state));
    CHECK(state.timeout_resets == 1U);
    return 0;
}

static int test_jitter_reset(void)
{
    clock_sync_state_t state;
    clock_sync_init(&state);
    state.sample_count = 2U;
    state.local_time_offset = 500U;
    state.previous_master_time = 1000U;
    state.previous_local_receive_time = 1000U + 20U + 500U + CLOCK_SYNC_MAX_JITTER_TICKS + 1U;

    const clock_sync_message_t message =
    {
        .time_sent = 2000U,
        .last_time_sent = 1000U,
        .last_time_acknowledge_delay = 20U,
    };
    CHECK(clock_sync_observe(&state, &message, 3000U, 0U, 0U, 10U) ==
          CLOCK_SYNC_SAMPLE_REJECTED_JITTER);
    CHECK(state.sample_count == 0U);
    CHECK(state.jitter_resets == 1U);
    return 0;
}

int main(void)
{
    if ((test_wrap_helpers() != 0) ||
        (test_lock_and_timeout() != 0) ||
        (test_jitter_reset() != 0))
    {
        return 1;
    }

    (void)puts("clock_sync_test: PASS");
    return 0;
}
