#ifndef OPENPNP_TARGET_CONTRACT_H
#define OPENPNP_TARGET_CONTRACT_H

#include <stdint.h>

typedef struct
{
    uint32_t flash_bytes;
    uint32_t sram_bytes;
    uint32_t maximum_core_clock_hz;
    uint32_t duet_motion_clock_hz;
    uintptr_t fdcan_base;
    uintptr_t primary_step_timer_base;
    uintptr_t secondary_step_timer_base;
    uint8_t planned_remote_drivers;
} target_contract_t;

extern const target_contract_t g_target_contract;

#endif
