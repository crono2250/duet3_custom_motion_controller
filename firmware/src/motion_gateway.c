#include "motion_gateway.h"

#include <stddef.h>

motion_gateway_result_t motion_gateway_prepare_segment(const clock_sync_state_t *clock_sync,
                                                       const motion_gateway_command_t *command,
                                                       step_segment_t *segment)
{
    if ((clock_sync == NULL) || (command == NULL) || (segment == NULL))
    {
        return MOTION_GATEWAY_INVALID_ARGUMENT;
    }
    if (!clock_sync_is_locked(clock_sync))
    {
        return MOTION_GATEWAY_CLOCK_NOT_LOCKED;
    }

    const step_segment_t prepared =
    {
        .first_step_time = clock_sync_to_local_time(clock_sync,
                                                    command->first_step_time_master),
        .step_period_ticks = command->step_period_ticks,
        .step_count = command->step_count,
        .pulse_width_ticks = command->pulse_width_ticks,
        .direction_positive = command->direction_positive,
    };
    if (!step_segment_is_valid(&prepared))
    {
        return MOTION_GATEWAY_INVALID_ARGUMENT;
    }

    *segment = prepared;
    return MOTION_GATEWAY_OK;
}
