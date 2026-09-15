#include "target_contract.h"

#include "platform.h"

#include "stm32g4xx.h"

/*
 * Bring-up image only. External board I/O remains unconfigured because the
 * PCB pinout is not closed. Internal FDCAN loopback and the 750 kHz motion
 * clock are exercised before the MCU becomes idle for SWD inspection.
 */
int main(void)
{
    volatile const target_contract_t *const contract = &g_target_contract;
    (void)contract;

    (void)platform_init();

    for (;;)
    {
        __WFI();
    }
}
