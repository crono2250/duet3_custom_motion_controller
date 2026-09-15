#ifndef OPENPNP_MOTION_GATEWAY_H
#define OPENPNP_MOTION_GATEWAY_H

#include <stdbool.h>
#include <stdint.h>

#include "clock_sync.h"
#include "step_engine.h"

/*
 * Project-owned, protocol-neutral boundary between an application decoder and
 * the MIT motion core. Do not add Duet/CANlib wire structures to this header.
 */
typedef struct
{
    uint32_t first_step_time_master;
    uint32_t step_period_ticks;
    uint32_t step_count;
    uint32_t pulse_width_ticks;
    bool direction_positive;
} motion_gateway_command_t;

typedef enum
{
    MOTION_GATEWAY_OK = 0,
    MOTION_GATEWAY_INVALID_ARGUMENT,
    MOTION_GATEWAY_CLOCK_NOT_LOCKED
} motion_gateway_result_t;

motion_gateway_result_t motion_gateway_prepare_segment(const clock_sync_state_t *clock_sync,
                                                       const motion_gateway_command_t *command,
                                                       step_segment_t *segment);

#endif
