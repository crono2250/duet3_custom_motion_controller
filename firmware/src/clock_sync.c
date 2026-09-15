#include "clock_sync.h"

#include <limits.h>
#include <stddef.h>

static uint32_t absolute_i32(int32_t value)
{
    if (value >= 0)
    {
        return (uint32_t)value;
    }
    return (uint32_t)(-(value + 1)) + 1U;
}

static int32_t add_saturating_i32(int32_t lhs, int32_t rhs)
{
    if ((rhs > 0) && (lhs > (INT32_MAX - rhs)))
    {
        return INT32_MAX;
    }
    if ((rhs < 0) && (lhs < (INT32_MIN - rhs)))
    {
        return INT32_MIN;
    }
    return lhs + rhs;
}

void clock_sync_init(clock_sync_state_t *state)
{
    if (state == NULL)
    {
        return;
    }

    *state = (clock_sync_state_t){0};
}

uint32_t clock_sync_fdcan_delay_ticks(uint16_t timestamp_now, uint16_t timestamp_at_rx)
{
    const uint32_t elapsed_microseconds = (uint16_t)(timestamp_now - timestamp_at_rx);

    /* FDCAN timestamp increments at the 1 MHz nominal CAN bit rate. */
    return (elapsed_microseconds * 3U) / 4U;
}

clock_sync_result_t clock_sync_observe(clock_sync_state_t *state,
                                      const clock_sync_message_t *message,
                                      uint32_t local_time_now,
                                      uint16_t timestamp_now,
                                      uint16_t timestamp_at_rx,
                                      uint32_t uptime_millis)
{
    if ((state == NULL) || (message == NULL))
    {
        return CLOCK_SYNC_SAMPLE_PENDING;
    }

    const uint32_t receive_delay = clock_sync_fdcan_delay_ticks(timestamp_now, timestamp_at_rx);
    const uint32_t local_receive_time = local_time_now - receive_delay;
    const uint32_t previous_local_time = state->previous_local_receive_time;
    const uint32_t previous_master_time = state->previous_master_time;

    state->previous_local_receive_time = local_receive_time;
    state->previous_master_time = message->time_sent;
    if (receive_delay > state->peak_receive_delay)
    {
        state->peak_receive_delay = receive_delay;
    }

    if (state->sample_count == 0U)
    {
        state->sample_count = 1U;
        state->error_accumulator = 0;
        return CLOCK_SYNC_SAMPLE_PENDING;
    }

    if ((message->last_time_sent != previous_master_time) ||
        (message->last_time_acknowledge_delay == 0U))
    {
        return CLOCK_SYNC_SAMPLE_PENDING;
    }

    const uint32_t corrected_master_time = previous_master_time + message->last_time_acknowledge_delay;
    const uint32_t new_offset = previous_local_time - corrected_master_time;
    const int32_t error = (int32_t)(new_offset - state->local_time_offset);

    if ((absolute_i32(error) > CLOCK_SYNC_MAX_JITTER_TICKS) && (state->sample_count > 1U))
    {
        state->local_time_offset = new_offset;
        state->sample_count = 0U;
        state->error_accumulator = 0;
        state->has_jitter_sample = false;
        ++state->jitter_resets;
        return CLOCK_SYNC_SAMPLE_REJECTED_JITTER;
    }

    state->last_sync_millis = uptime_millis;
    if (state->sample_count < CLOCK_SYNC_REQUIRED_SAMPLES)
    {
        state->local_time_offset = new_offset;
        ++state->sample_count;
        return (state->sample_count == CLOCK_SYNC_REQUIRED_SAMPLES)
                   ? CLOCK_SYNC_SAMPLE_LOCKED
                   : CLOCK_SYNC_SAMPLE_ACCEPTED;
    }

    state->error_accumulator = add_saturating_i32(state->error_accumulator, error);
    state->local_time_offset += (uint32_t)((state->error_accumulator / 64) + (error / 4));

    if (!state->has_jitter_sample)
    {
        state->peak_positive_jitter = error;
        state->peak_negative_jitter = error;
        state->has_jitter_sample = true;
    }
    else if (error > state->peak_positive_jitter)
    {
        state->peak_positive_jitter = error;
    }
    else if (error < state->peak_negative_jitter)
    {
        state->peak_negative_jitter = error;
    }

    return CLOCK_SYNC_SAMPLE_LOCKED;
}

bool clock_sync_is_locked(const clock_sync_state_t *state)
{
    return (state != NULL) && (state->sample_count == CLOCK_SYNC_REQUIRED_SAMPLES);
}

bool clock_sync_check_timeout(clock_sync_state_t *state, uint32_t uptime_millis)
{
    if (!clock_sync_is_locked(state))
    {
        return false;
    }

    if ((uint32_t)(uptime_millis - state->last_sync_millis) <= CLOCK_SYNC_TIMEOUT_MS)
    {
        return false;
    }

    state->sample_count = 0U;
    state->error_accumulator = 0;
    state->has_jitter_sample = false;
    ++state->timeout_resets;
    return true;
}

uint32_t clock_sync_to_master_time(const clock_sync_state_t *state, uint32_t local_time)
{
    return (state == NULL) ? local_time : (local_time - state->local_time_offset);
}

uint32_t clock_sync_to_local_time(const clock_sync_state_t *state, uint32_t master_time)
{
    return (state == NULL) ? master_time : (master_time + state->local_time_offset);
}
