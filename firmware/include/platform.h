#ifndef OPENPNP_PLATFORM_H
#define OPENPNP_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "clock_sync.h"
#include "fdcan_port.h"

#define BRINGUP_DIAGNOSTICS_MAGIC 0x47343331UL
#define BRINGUP_DIAGNOSTICS_VERSION 1UL

typedef enum
{
    BRINGUP_STAGE_RESET = 0,
    BRINGUP_STAGE_HAL_READY,
    BRINGUP_STAGE_CLOCK_READY,
    BRINGUP_STAGE_MOTION_CLOCK_READY,
    BRINGUP_STAGE_FDCAN_TESTING,
    BRINGUP_STAGE_READY,
    BRINGUP_STAGE_FAULT
} bringup_stage_t;

typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t stage;
    uint32_t system_clock_hz;
    uint32_t motion_clock_hz;
    uint32_t motion_clock_sample;
    uint32_t fdcan_result;
    uint32_t fdcan_mode;
    uint32_t fdcan_receive_timestamp;
    uint32_t fdcan_timestamp_after_receive;
    clock_sync_state_t clock_sync;
} bringup_diagnostics_t;

extern volatile bringup_diagnostics_t g_bringup_diagnostics;

bool platform_init(void);

#endif
