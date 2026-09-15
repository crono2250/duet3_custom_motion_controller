#ifndef OPENPNP_CLOCK_SYNC_H
#define OPENPNP_CLOCK_SYNC_H

#include <stdbool.h>
#include <stdint.h>

#include "motion_clock.h"

#define CLOCK_SYNC_REQUIRED_SAMPLES 10U
#define CLOCK_SYNC_TIMEOUT_MS 2000UL
#define CLOCK_SYNC_MAX_JITTER_TICKS (MOTION_CLOCK_HZ / 100UL)

typedef struct
{
    uint32_t time_sent;
    uint32_t last_time_sent;
    uint32_t last_time_acknowledge_delay;
} clock_sync_message_t;

typedef enum
{
    CLOCK_SYNC_SAMPLE_PENDING = 0,
    CLOCK_SYNC_SAMPLE_ACCEPTED,
    CLOCK_SYNC_SAMPLE_LOCKED,
    CLOCK_SYNC_SAMPLE_REJECTED_JITTER
} clock_sync_result_t;

typedef struct
{
    uint32_t local_time_offset;
    uint32_t previous_local_receive_time;
    uint32_t previous_master_time;
    uint32_t last_sync_millis;
    uint32_t peak_receive_delay;
    uint32_t jitter_resets;
    uint32_t timeout_resets;
    int32_t error_accumulator;
    int32_t peak_positive_jitter;
    int32_t peak_negative_jitter;
    uint8_t sample_count;
    bool has_jitter_sample;
} clock_sync_state_t;

void clock_sync_init(clock_sync_state_t *state);
uint32_t clock_sync_fdcan_delay_ticks(uint16_t timestamp_now, uint16_t timestamp_at_rx);
clock_sync_result_t clock_sync_observe(clock_sync_state_t *state,
                                      const clock_sync_message_t *message,
                                      uint32_t local_time_now,
                                      uint16_t timestamp_now,
                                      uint16_t timestamp_at_rx,
                                      uint32_t uptime_millis);
bool clock_sync_is_locked(const clock_sync_state_t *state);
bool clock_sync_check_timeout(clock_sync_state_t *state, uint32_t uptime_millis);
uint32_t clock_sync_to_master_time(const clock_sync_state_t *state, uint32_t local_time);

#endif
