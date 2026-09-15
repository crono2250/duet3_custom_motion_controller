#include "platform.h"

#include "motion_clock.h"
#include "stm32g4xx_hal.h"

#define SYSTEM_CLOCK_HZ 168000000UL

__attribute__((used)) volatile bringup_diagnostics_t g_bringup_diagnostics;

static bool configure_system_clock(void)
{
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
    {
        return false;
    }

    RCC_OscInitTypeDef oscillator = {0};
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    oscillator.PLL.PLLM = RCC_PLLM_DIV4;
    oscillator.PLL.PLLN = 84U;
    oscillator.PLL.PLLP = RCC_PLLP_DIV2;
    oscillator.PLL.PLLQ = RCC_PLLQ_DIV2;
    oscillator.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
    {
        return false;
    }

    RCC_ClkInitTypeDef clocks = {0};
    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV1;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_4) != HAL_OK)
    {
        return false;
    }

    RCC_PeriphCLKInitTypeDef peripheral_clock = {0};
    peripheral_clock.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    peripheral_clock.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&peripheral_clock) != HAL_OK)
    {
        return false;
    }

    SystemCoreClockUpdate();
    return (SystemCoreClock == SYSTEM_CLOCK_HZ) &&
           (HAL_RCC_GetPCLK1Freq() == SYSTEM_CLOCK_HZ);
}

bool platform_init(void)
{
    g_bringup_diagnostics = (bringup_diagnostics_t){0};
    g_bringup_diagnostics.magic = BRINGUP_DIAGNOSTICS_MAGIC;
    g_bringup_diagnostics.version = BRINGUP_DIAGNOSTICS_VERSION;
    g_bringup_diagnostics.stage = BRINGUP_STAGE_RESET;

    if (HAL_Init() != HAL_OK)
    {
        g_bringup_diagnostics.stage = BRINGUP_STAGE_FAULT;
        return false;
    }
    g_bringup_diagnostics.stage = BRINGUP_STAGE_HAL_READY;

    if (!configure_system_clock())
    {
        g_bringup_diagnostics.stage = BRINGUP_STAGE_FAULT;
        return false;
    }
    g_bringup_diagnostics.system_clock_hz = SystemCoreClock;
    g_bringup_diagnostics.stage = BRINGUP_STAGE_CLOCK_READY;

    motion_clock_init();
    g_bringup_diagnostics.motion_clock_hz = MOTION_CLOCK_HZ;
    g_bringup_diagnostics.motion_clock_sample = motion_clock_now();
    g_bringup_diagnostics.stage = BRINGUP_STAGE_MOTION_CLOCK_READY;

    clock_sync_init((clock_sync_state_t *)&g_bringup_diagnostics.clock_sync);
    step_engine_init((step_engine_t *)&g_bringup_diagnostics.step_engine);

    g_bringup_diagnostics.stage = BRINGUP_STAGE_FDCAN_TESTING;
    uint16_t receive_timestamp = 0U;
    uint16_t timestamp_after_receive = 0U;
    const fdcan_loopback_result_t result =
        fdcan_loopback_run(&receive_timestamp, &timestamp_after_receive);
    g_bringup_diagnostics.fdcan_result = (uint32_t)result;
    g_bringup_diagnostics.fdcan_receive_timestamp = receive_timestamp;
    g_bringup_diagnostics.fdcan_timestamp_after_receive = timestamp_after_receive;

    if (result != FDCAN_LOOPBACK_OK)
    {
        g_bringup_diagnostics.stage = BRINGUP_STAGE_FAULT;
        return false;
    }

    g_bringup_diagnostics.stage = BRINGUP_STAGE_READY;
    return true;
}
