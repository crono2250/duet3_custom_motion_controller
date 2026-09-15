#ifndef OPENPNP_FDCAN_LOOPBACK_H
#define OPENPNP_FDCAN_LOOPBACK_H

#include <stdint.h>

typedef enum
{
    FDCAN_LOOPBACK_OK = 0,
    FDCAN_LOOPBACK_INIT_FAILED,
    FDCAN_LOOPBACK_FILTER_FAILED,
    FDCAN_LOOPBACK_TIMESTAMP_FAILED,
    FDCAN_LOOPBACK_START_FAILED,
    FDCAN_LOOPBACK_TRANSMIT_FAILED,
    FDCAN_LOOPBACK_TIMEOUT,
    FDCAN_LOOPBACK_RECEIVE_FAILED,
    FDCAN_LOOPBACK_CONTENT_MISMATCH
} fdcan_loopback_result_t;

fdcan_loopback_result_t fdcan_loopback_run(uint16_t *receive_timestamp,
                                           uint16_t *timestamp_after_receive);

#endif
