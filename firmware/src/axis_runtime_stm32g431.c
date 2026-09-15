#include "axis_runtime.h"

#include <stddef.h>

#include "board_pins.h"
#include "motion_clock.h"

#define MAX_IMMEDIATE_EVENTS 4U

volatile axis_runtime_diagnostics_t g_axis_runtime_diagnostics;

static step_engine_t step_engine;

static void write_step(bool high)
{
    BOARD_AXIS0_STEP_PORT->BSRR = high ? BOARD_AXIS0_STEP_PIN
                                       : ((uint32_t)BOARD_AXIS0_STEP_PIN << 16U);
    g_axis_runtime_diagnostics.step_high = high;
}

static void write_direction(bool positive)
{
    BOARD_AXIS0_DIRECTION_PORT->BSRR = positive ? BOARD_AXIS0_DIRECTION_PIN
                                                : ((uint32_t)BOARD_AXIS0_DIRECTION_PIN << 16U);
    g_axis_runtime_diagnostics.direction_positive = positive;
}

static void apply_event(const step_event_t *event)
{
    if ((event->actions & STEP_ACTION_SET_DIRECTION) != 0U)
    {
        write_direction(event->direction_positive);
    }
    if ((event->actions & STEP_ACTION_RAISE) != 0U)
    {
        write_step(true);
        ++g_axis_runtime_diagnostics.emitted_steps;
    }
    if ((event->actions & STEP_ACTION_LOWER) != 0U)
    {
        write_step(false);
    }
    if ((event->actions & STEP_ACTION_SEGMENT_DONE) != 0U)
    {
        ++g_axis_runtime_diagnostics.completed_segments;
    }
    if (event->late_by_ticks != 0U)
    {
        ++g_axis_runtime_diagnostics.late_events;
        if (event->late_by_ticks > g_axis_runtime_diagnostics.maximum_lateness_ticks)
        {
            g_axis_runtime_diagnostics.maximum_lateness_ticks = event->late_by_ticks;
        }
    }
    g_axis_runtime_diagnostics.queued_segments = step_engine_queued(&step_engine);
}

static void service_deadline(uint32_t now, void *context)
{
    (void)context;

    for (uint32_t attempt = 0U; attempt < MAX_IMMEDIATE_EVENTS; ++attempt)
    {
        step_event_t event = {0};
        if (!step_engine_on_deadline(&step_engine, now, &event))
        {
            if (event.has_deadline &&
                motion_clock_schedule(event.next_deadline, service_deadline, NULL))
            {
                return;
            }
        }
        else
        {
            apply_event(&event);
            if (!event.has_deadline)
            {
                return;
            }
            if (motion_clock_schedule(event.next_deadline, service_deadline, NULL))
            {
                return;
            }
        }
        now = motion_clock_now();
    }

    g_axis_runtime_diagnostics.fault = AXIS_RUNTIME_DEADLINE_OVERRUN;
    axis_runtime_emergency_stop();
}

void axis_runtime_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    write_step(false);
    BOARD_AXIS0_DIRECTION_PORT->BSRR = (uint32_t)BOARD_AXIS0_DIRECTION_PIN << 16U;
    BOARD_AXIS0_ENABLE_PORT->BSRR = BOARD_AXIS0_ENABLE_PIN;

    const GPIO_InitTypeDef step_pin =
    {
        .Pin = BOARD_AXIS0_STEP_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = 0U,
    };
    HAL_GPIO_Init(BOARD_AXIS0_STEP_PORT, &step_pin);

    const GPIO_InitTypeDef control_pins =
    {
        .Pin = BOARD_AXIS0_DIRECTION_PIN | BOARD_AXIS0_ENABLE_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = 0U,
    };
    HAL_GPIO_Init(GPIOB, &control_pins);

    step_engine_init(&step_engine);
    g_axis_runtime_diagnostics = (axis_runtime_diagnostics_t){0};
}

void axis_runtime_enable(bool enable)
{
    if (!enable)
    {
        motion_clock_cancel();
        write_step(false);
        step_engine_init(&step_engine);
        g_axis_runtime_diagnostics.queued_segments = 0U;
    }
    HAL_GPIO_WritePin(BOARD_AXIS0_ENABLE_PORT,
                      BOARD_AXIS0_ENABLE_PIN,
                      enable ? BOARD_AXIS0_ENABLE_ACTIVE_STATE
                             : BOARD_AXIS0_ENABLE_INACTIVE_STATE);
    g_axis_runtime_diagnostics.enabled = enable;
}

axis_runtime_result_t axis_runtime_enqueue(const step_segment_t *segment)
{
    if (segment == NULL)
    {
        return AXIS_RUNTIME_INVALID_SEGMENT;
    }

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (!g_axis_runtime_diagnostics.enabled)
    {
        __set_PRIMASK(primask);
        return AXIS_RUNTIME_DISABLED;
    }
    const bool queued = step_engine_enqueue(&step_engine, segment);
    if (!queued)
    {
        __set_PRIMASK(primask);
        return (step_engine_queued(&step_engine) >= STEP_ENGINE_QUEUE_CAPACITY)
                   ? AXIS_RUNTIME_QUEUE_FULL
                   : AXIS_RUNTIME_INVALID_SEGMENT;
    }

    step_event_t event;
    const uint32_t now = motion_clock_now();
    const bool started = step_engine_start(&step_engine, now, &event);
    g_axis_runtime_diagnostics.queued_segments = step_engine_queued(&step_engine);
    __set_PRIMASK(primask);

    if (started)
    {
        apply_event(&event);
        if (!motion_clock_schedule(event.next_deadline, service_deadline, NULL))
        {
            service_deadline(motion_clock_now(), NULL);
        }
    }
    return AXIS_RUNTIME_OK;
}

void axis_runtime_emergency_stop(void)
{
    axis_runtime_enable(false);
}
