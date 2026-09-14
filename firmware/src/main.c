#include "target_contract.h"

#include "stm32g4xx.h"

/*
 * Bring-up image only. It deliberately leaves all board I/O unconfigured.
 * Referencing the contract keeps the build-time peripheral checks in the
 * linked image while the MCU remains idle for SWD inspection.
 */
int main(void)
{
    volatile const target_contract_t *const contract = &g_target_contract;
    (void)contract;

    for (;;)
    {
        __WFI();
    }
}

