#include "fdcan_port.h"

#include <stddef.h>

#include "board_pins.h"

#define LOOPBACK_IDENTIFIER 0x01ABCDEUL
#define LOOPBACK_TIMEOUT_MS 100UL

_Static_assert(FDCAN_DLC_BYTES_8 == 8UL,
               "STM32 HAL FDCAN DLC encoding changed");
_Static_assert(FDCAN_DLC_BYTES_64 == 15UL,
               "STM32 HAL FDCAN DLC encoding changed");

static FDCAN_HandleTypeDef fdcan_handle;
static fdcan_port_mode_t active_mode;
static volatile uint8_t receive_head;
static volatile uint8_t receive_count;
static fdcan_port_frame_t receive_queue[FDCAN_PORT_RX_QUEUE_CAPACITY];
volatile fdcan_port_diagnostics_t g_fdcan_port_diagnostics;

static uint8_t dlc_to_length(uint32_t dlc)
{
    static const uint8_t lengths[16] =
    {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
        8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U,
    };
    return lengths[dlc & 0x0FU];
}

static uint32_t length_to_dlc(uint8_t length)
{
    if (length <= 8U)
    {
        return length;
    }
    if (length <= 12U) { return FDCAN_DLC_BYTES_12; }
    if (length <= 16U) { return FDCAN_DLC_BYTES_16; }
    if (length <= 20U) { return FDCAN_DLC_BYTES_20; }
    if (length <= 24U) { return FDCAN_DLC_BYTES_24; }
    if (length <= 32U) { return FDCAN_DLC_BYTES_32; }
    if (length <= 48U) { return FDCAN_DLC_BYTES_48; }
    return FDCAN_DLC_BYTES_64;
}

static bool valid_data_length(uint8_t length)
{
    return (length <= 8U) || (length == 12U) || (length == 16U) ||
           (length == 20U) || (length == 24U) || (length == 32U) ||
           (length == 48U) || (length == 64U);
}

static bool copy_received_frame(fdcan_port_frame_t *frame)
{
    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t data[FDCAN_PORT_MAX_DATA_BYTES] = {0};
    if (HAL_FDCAN_GetRxMessage(&fdcan_handle, FDCAN_RX_FIFO0, &header, data) != HAL_OK)
    {
        return false;
    }

    frame->identifier = header.Identifier;
    frame->length = dlc_to_length(header.DataLength);
    frame->timestamp = (uint16_t)header.RxTimestamp;
    frame->bit_rate_switch = header.BitRateSwitch == FDCAN_BRS_ON;
    frame->fd_format = header.FDFormat == FDCAN_FD_CAN;
    for (uint32_t index = 0U; index < frame->length; ++index)
    {
        frame->data[index] = data[index];
    }
    return true;
}

static void configure_normal_mode_pins(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(BOARD_FDCAN_STANDBY_PORT, BOARD_FDCAN_STANDBY_PIN, GPIO_PIN_SET);
    const GPIO_InitTypeDef standby_pin =
    {
        .Pin = BOARD_FDCAN_STANDBY_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0U,
    };
    HAL_GPIO_Init(BOARD_FDCAN_STANDBY_PORT, &standby_pin);

    const GPIO_InitTypeDef can_pins =
    {
        .Pin = BOARD_FDCAN_RX_PIN | BOARD_FDCAN_TX_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = BOARD_FDCAN_GPIO_AF,
    };
    HAL_GPIO_Init(GPIOA, &can_pins);
}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *handle)
{
    if ((handle != NULL) && (handle->Instance == FDCAN1))
    {
        if (active_mode == FDCAN_PORT_MODE_NORMAL)
        {
            configure_normal_mode_pins();
        }
        __HAL_RCC_FDCAN_CLK_ENABLE();
        __HAL_RCC_FDCAN_FORCE_RESET();
        __HAL_RCC_FDCAN_RELEASE_RESET();
    }
}

fdcan_port_result_t fdcan_port_init(fdcan_port_mode_t mode)
{
    active_mode = mode;
    receive_head = 0U;
    receive_count = 0U;
    g_fdcan_port_diagnostics = (fdcan_port_diagnostics_t){0};
    fdcan_handle = (FDCAN_HandleTypeDef){0};
    fdcan_handle.Instance = FDCAN1;
    fdcan_handle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    fdcan_handle.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    fdcan_handle.Init.Mode = (mode == FDCAN_PORT_MODE_INTERNAL_LOOPBACK)
                                 ? FDCAN_MODE_INTERNAL_LOOPBACK
                                 : FDCAN_MODE_NORMAL;
    fdcan_handle.Init.AutoRetransmission = ENABLE;
    fdcan_handle.Init.TransmitPause = DISABLE;
    fdcan_handle.Init.ProtocolException = DISABLE;

    /* 168 MHz / 7 / (1 + 18 + 5) = 1 Mbit/s, 79.2% sample point. */
    fdcan_handle.Init.NominalPrescaler = 7U;
    fdcan_handle.Init.NominalSyncJumpWidth = 5U;
    fdcan_handle.Init.NominalTimeSeg1 = 18U;
    fdcan_handle.Init.NominalTimeSeg2 = 5U;

    /* 168 MHz / 2 / (1 + 21 + 6) = 3 Mbit/s, 78.6% sample point. */
    fdcan_handle.Init.DataPrescaler = 2U;
    fdcan_handle.Init.DataSyncJumpWidth = 6U;
    fdcan_handle.Init.DataTimeSeg1 = 21U;
    fdcan_handle.Init.DataTimeSeg2 = 6U;
    fdcan_handle.Init.StdFiltersNbr = 0U;
    fdcan_handle.Init.ExtFiltersNbr = (mode == FDCAN_PORT_MODE_INTERNAL_LOOPBACK) ? 1U : 0U;
    fdcan_handle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&fdcan_handle) != HAL_OK)
    {
        return FDCAN_PORT_INIT_FAILED;
    }

    if (mode == FDCAN_PORT_MODE_INTERNAL_LOOPBACK)
    {
        const FDCAN_FilterTypeDef filter =
        {
            .IdType = FDCAN_EXTENDED_ID,
            .FilterIndex = 0U,
            .FilterType = FDCAN_FILTER_MASK,
            .FilterConfig = FDCAN_FILTER_TO_RXFIFO0,
            .FilterID1 = LOOPBACK_IDENTIFIER,
            .FilterID2 = 0x1FFFFFFFUL,
        };
        if ((HAL_FDCAN_ConfigFilter(&fdcan_handle, &filter) != HAL_OK) ||
            (HAL_FDCAN_ConfigGlobalFilter(&fdcan_handle,
                                          FDCAN_REJECT,
                                          FDCAN_REJECT,
                                          FDCAN_REJECT_REMOTE,
                                          FDCAN_REJECT_REMOTE) != HAL_OK))
        {
            return FDCAN_PORT_FILTER_FAILED;
        }
    }
    else if (HAL_FDCAN_ConfigGlobalFilter(&fdcan_handle,
                                           FDCAN_REJECT,
                                           FDCAN_ACCEPT_IN_RX_FIFO0,
                                           FDCAN_REJECT_REMOTE,
                                           FDCAN_REJECT_REMOTE) != HAL_OK)
    {
        return FDCAN_PORT_FILTER_FAILED;
    }

    if (HAL_FDCAN_ConfigTimestampCounter(&fdcan_handle, FDCAN_TIMESTAMP_PRESC_1) != HAL_OK)
    {
        return FDCAN_PORT_TIMESTAMP_FAILED;
    }
    if (HAL_FDCAN_Start(&fdcan_handle) != HAL_OK)
    {
        return FDCAN_PORT_START_FAILED;
    }

    if (mode == FDCAN_PORT_MODE_NORMAL)
    {
        HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 3U, 0U);
        HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
        if (HAL_FDCAN_ActivateNotification(&fdcan_handle,
                                            FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                            0U) != HAL_OK)
        {
            return FDCAN_PORT_NOTIFICATION_FAILED;
        }
        HAL_GPIO_WritePin(BOARD_FDCAN_STANDBY_PORT, BOARD_FDCAN_STANDBY_PIN, GPIO_PIN_RESET);
    }

    return FDCAN_PORT_OK;
}

fdcan_port_result_t fdcan_port_run_loopback_test(uint16_t *receive_timestamp,
                                                 uint16_t *timestamp_after_receive)
{
    if ((active_mode != FDCAN_PORT_MODE_INTERNAL_LOOPBACK) ||
        (receive_timestamp == NULL) || (timestamp_after_receive == NULL))
    {
        return FDCAN_PORT_INIT_FAILED;
    }

    static const uint8_t transmit_data[8] = {0x47U, 0x34U, 0x33U, 0x31U, 0x75U, 0x00U, 0x03U, 0x00U};
    const fdcan_port_frame_t transmitted =
    {
        .identifier = LOOPBACK_IDENTIFIER,
        .data = {0x47U, 0x34U, 0x33U, 0x31U, 0x75U, 0x00U, 0x03U, 0x00U},
        .length = sizeof(transmit_data),
        .timestamp = 0U,
        .bit_rate_switch = true,
        .fd_format = true,
    };
    if (!fdcan_port_send(&transmitted))
    {
        return FDCAN_PORT_TRANSMIT_FAILED;
    }

    const uint32_t started_at = HAL_GetTick();
    while (HAL_FDCAN_GetRxFifoFillLevel(&fdcan_handle, FDCAN_RX_FIFO0) == 0U)
    {
        if ((uint32_t)(HAL_GetTick() - started_at) > LOOPBACK_TIMEOUT_MS)
        {
            return FDCAN_PORT_TIMEOUT;
        }
    }

    fdcan_port_frame_t received = {0};
    if (!copy_received_frame(&received))
    {
        ++g_fdcan_port_diagnostics.receive_errors;
        return FDCAN_PORT_RECEIVE_FAILED;
    }
    ++g_fdcan_port_diagnostics.received_frames;
    *receive_timestamp = received.timestamp;
    *timestamp_after_receive = fdcan_port_timestamp_now();

    if ((received.identifier != transmitted.identifier) ||
        (received.length != transmitted.length) ||
        !received.fd_format || !received.bit_rate_switch)
    {
        return FDCAN_PORT_CONTENT_MISMATCH;
    }
    for (uint32_t index = 0U; index < transmitted.length; ++index)
    {
        if (received.data[index] != transmit_data[index])
        {
            return FDCAN_PORT_CONTENT_MISMATCH;
        }
    }
    return FDCAN_PORT_OK;
}

bool fdcan_port_send(const fdcan_port_frame_t *frame)
{
    if ((frame == NULL) || !valid_data_length(frame->length) ||
        (frame->identifier > 0x1FFFFFFFUL) ||
        (!frame->fd_format && ((frame->length > 8U) || frame->bit_rate_switch)))
    {
        ++g_fdcan_port_diagnostics.transmit_errors;
        return false;
    }

    const FDCAN_TxHeaderTypeDef header =
    {
        .Identifier = frame->identifier,
        .IdType = FDCAN_EXTENDED_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = length_to_dlc(frame->length),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = frame->bit_rate_switch ? FDCAN_BRS_ON : FDCAN_BRS_OFF,
        .FDFormat = frame->fd_format ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0U,
    };
    if (HAL_FDCAN_AddMessageToTxFifoQ(&fdcan_handle, &header, frame->data) != HAL_OK)
    {
        ++g_fdcan_port_diagnostics.transmit_errors;
        return false;
    }
    ++g_fdcan_port_diagnostics.transmitted_frames;
    return true;
}

bool fdcan_port_receive(fdcan_port_frame_t *frame)
{
    if (frame == NULL)
    {
        return false;
    }

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (receive_count == 0U)
    {
        __set_PRIMASK(primask);
        return false;
    }

    *frame = receive_queue[receive_head];
    receive_head = (uint8_t)((receive_head + 1U) % FDCAN_PORT_RX_QUEUE_CAPACITY);
    --receive_count;
    __set_PRIMASK(primask);
    return true;
}

uint16_t fdcan_port_timestamp_now(void)
{
    return HAL_FDCAN_GetTimestampCounter(&fdcan_handle);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *handle, uint32_t interrupt_flags)
{
    if ((handle != &fdcan_handle) ||
        ((interrupt_flags & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U))
    {
        return;
    }

    while (HAL_FDCAN_GetRxFifoFillLevel(&fdcan_handle, FDCAN_RX_FIFO0) != 0U)
    {
        fdcan_port_frame_t frame;
        if (!copy_received_frame(&frame))
        {
            ++g_fdcan_port_diagnostics.receive_errors;
            break;
        }
        ++g_fdcan_port_diagnostics.received_frames;

        if (receive_count < FDCAN_PORT_RX_QUEUE_CAPACITY)
        {
            const uint8_t tail = (uint8_t)((receive_head + receive_count) % FDCAN_PORT_RX_QUEUE_CAPACITY);
            receive_queue[tail] = frame;
            ++receive_count;
        }
        else
        {
            ++g_fdcan_port_diagnostics.dropped_frames;
        }
    }
}

void fdcan_port_irq_handler(void)
{
    HAL_FDCAN_IRQHandler(&fdcan_handle);
}
