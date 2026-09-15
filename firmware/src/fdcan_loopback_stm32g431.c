#include "fdcan_loopback.h"

#include "stm32g4xx_hal.h"

#define LOOPBACK_IDENTIFIER 0x01ABCDEUL
#define LOOPBACK_TIMEOUT_MS 100UL

static FDCAN_HandleTypeDef fdcan_handle;

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *handle)
{
    if ((handle != NULL) && (handle->Instance == FDCAN1))
    {
        __HAL_RCC_FDCAN_CLK_ENABLE();
        __HAL_RCC_FDCAN_FORCE_RESET();
        __HAL_RCC_FDCAN_RELEASE_RESET();
    }
}

fdcan_loopback_result_t fdcan_loopback_run(uint16_t *receive_timestamp,
                                           uint16_t *timestamp_after_receive)
{
    if ((receive_timestamp == NULL) || (timestamp_after_receive == NULL))
    {
        return FDCAN_LOOPBACK_INIT_FAILED;
    }

    fdcan_handle = (FDCAN_HandleTypeDef){0};
    fdcan_handle.Instance = FDCAN1;
    fdcan_handle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    fdcan_handle.Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    fdcan_handle.Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
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
    fdcan_handle.Init.ExtFiltersNbr = 1U;
    fdcan_handle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&fdcan_handle) != HAL_OK)
    {
        return FDCAN_LOOPBACK_INIT_FAILED;
    }

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
        return FDCAN_LOOPBACK_FILTER_FAILED;
    }

    if (HAL_FDCAN_ConfigTimestampCounter(&fdcan_handle, FDCAN_TIMESTAMP_PRESC_1) != HAL_OK)
    {
        return FDCAN_LOOPBACK_TIMESTAMP_FAILED;
    }
    if (HAL_FDCAN_Start(&fdcan_handle) != HAL_OK)
    {
        return FDCAN_LOOPBACK_START_FAILED;
    }

    const FDCAN_TxHeaderTypeDef transmit_header =
    {
        .Identifier = LOOPBACK_IDENTIFIER,
        .IdType = FDCAN_EXTENDED_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES_8,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_ON,
        .FDFormat = FDCAN_FD_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0U,
    };
    static const uint8_t transmit_data[8] = {0x47U, 0x34U, 0x33U, 0x31U, 0x75U, 0x00U, 0x03U, 0x00U};

    if (HAL_FDCAN_AddMessageToTxFifoQ(&fdcan_handle, &transmit_header, transmit_data) != HAL_OK)
    {
        return FDCAN_LOOPBACK_TRANSMIT_FAILED;
    }

    const uint32_t started_at = HAL_GetTick();
    while (HAL_FDCAN_GetRxFifoFillLevel(&fdcan_handle, FDCAN_RX_FIFO0) == 0U)
    {
        if ((uint32_t)(HAL_GetTick() - started_at) > LOOPBACK_TIMEOUT_MS)
        {
            return FDCAN_LOOPBACK_TIMEOUT;
        }
    }

    FDCAN_RxHeaderTypeDef receive_header = {0};
    uint8_t receive_data[8] = {0};
    if (HAL_FDCAN_GetRxMessage(&fdcan_handle, FDCAN_RX_FIFO0, &receive_header, receive_data) != HAL_OK)
    {
        return FDCAN_LOOPBACK_RECEIVE_FAILED;
    }

    *receive_timestamp = (uint16_t)receive_header.RxTimestamp;
    *timestamp_after_receive = HAL_FDCAN_GetTimestampCounter(&fdcan_handle);

    if ((receive_header.Identifier != LOOPBACK_IDENTIFIER) ||
        (receive_header.IdType != FDCAN_EXTENDED_ID) ||
        (receive_header.FDFormat != FDCAN_FD_CAN) ||
        (receive_header.BitRateSwitch != FDCAN_BRS_ON))
    {
        return FDCAN_LOOPBACK_CONTENT_MISMATCH;
    }

    for (uint32_t index = 0U; index < sizeof(transmit_data); ++index)
    {
        if (receive_data[index] != transmit_data[index])
        {
            return FDCAN_LOOPBACK_CONTENT_MISMATCH;
        }
    }

    return FDCAN_LOOPBACK_OK;
}
