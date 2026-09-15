#include "motion_clock.h"

#include <stddef.h>

#include "stm32g4xx_hal.h"

#define STM32G431_TIMER_CLOCK_HZ 168000000UL
#define MOTION_TIMER_PRESCALER ((STM32G431_TIMER_CLOCK_HZ / MOTION_CLOCK_HZ) - 1UL)

_Static_assert((STM32G431_TIMER_CLOCK_HZ % MOTION_CLOCK_HZ) == 0U,
               "TIM2 clock must divide exactly to 750 kHz");
_Static_assert(MOTION_TIMER_PRESCALER <= UINT16_MAX, "TIM2 prescaler is out of range");

static motion_clock_callback_t scheduled_callback;
static void *scheduled_context;

void motion_clock_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();

    TIM2->CR1 = 0U;
    TIM2->PSC = MOTION_TIMER_PRESCALER;
    TIM2->ARR = UINT32_MAX;
    TIM2->CCR1 = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->DIER = 0U;

    HAL_NVIC_SetPriority(TIM2_IRQn, 2U, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 = TIM_CR1_CEN;
}

uint32_t motion_clock_now(void)
{
    return TIM2->CNT;
}

bool motion_clock_schedule(uint32_t deadline, motion_clock_callback_t callback, void *context)
{
    if (callback == NULL)
    {
        return false;
    }

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    scheduled_callback = callback;
    scheduled_context = context;
    TIM2->CCR1 = deadline;
    TIM2->SR = ~TIM_SR_CC1IF;
    TIM2->DIER |= TIM_DIER_CC1IE;

    const int32_t lead = motion_clock_delta(deadline, TIM2->CNT);
    if (lead < (int32_t)MOTION_CLOCK_MIN_LEAD_TICKS)
    {
        TIM2->DIER &= ~TIM_DIER_CC1IE;
        scheduled_callback = NULL;
        scheduled_context = NULL;
    }

    __set_PRIMASK(primask);
    return lead >= (int32_t)MOTION_CLOCK_MIN_LEAD_TICKS;
}

void motion_clock_cancel(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    TIM2->DIER &= ~TIM_DIER_CC1IE;
    scheduled_callback = NULL;
    scheduled_context = NULL;
    __set_PRIMASK(primask);
}

void motion_clock_irq_handler(void)
{
    if (((TIM2->SR & TIM_SR_CC1IF) == 0U) || ((TIM2->DIER & TIM_DIER_CC1IE) == 0U))
    {
        return;
    }

    TIM2->SR = ~TIM_SR_CC1IF;
    TIM2->DIER &= ~TIM_DIER_CC1IE;
    motion_clock_callback_t const callback = scheduled_callback;
    void *const context = scheduled_context;
    scheduled_callback = NULL;
    scheduled_context = NULL;

    if (callback != NULL)
    {
        callback(TIM2->CNT, context);
    }
}
