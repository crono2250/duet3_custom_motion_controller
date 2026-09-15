#include "target_contract.h"

#include "stm32g4xx.h"

#if !defined(STM32G431xx)
#error "This target must be compiled for STM32G431xx"
#endif

#if !defined(FDCAN1_BASE) || !defined(TIM1_BASE) || !defined(TIM2_BASE) || !defined(TIM8_BASE)
#error "STM32G431 FDCAN/TIM peripheral definitions are unavailable"
#endif

_Static_assert(sizeof(uintptr_t) == sizeof(uint32_t), "Expected a 32-bit target");
_Static_assert(FLASH_BASE == 0x08000000UL, "Unexpected STM32G431 Flash base");
_Static_assert(SRAM_BASE == 0x20000000UL, "Unexpected STM32G431 SRAM base");

/*
 * The timer assignment is intentionally a resource contract, not a PCB
 * pinout. The actual alternate-function mapping must be closed during the
 * hardware design phase.
 */
__attribute__((used, section(".target_contract")))
const target_contract_t g_target_contract =
{
    .flash_bytes = 128UL * 1024UL,
    .sram_bytes = 32UL * 1024UL,
    .maximum_core_clock_hz = 170000000UL,
    .duet_motion_clock_hz = 750000UL,
    .fdcan_base = FDCAN1_BASE,
    .motion_clock_timer_base = TIM2_BASE,
    .primary_step_timer_base = TIM1_BASE,
    .secondary_step_timer_base = TIM8_BASE,
    .planned_remote_drivers = 5U,
};
