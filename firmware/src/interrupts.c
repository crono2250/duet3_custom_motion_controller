#include "motion_clock.h"
#include "stm32g4xx_hal.h"

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void TIM2_IRQHandler(void)
{
    motion_clock_irq_handler();
}

void HardFault_Handler(void)
{
    __disable_irq();
    for (;;)
    {
    }
}
