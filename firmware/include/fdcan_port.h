#ifndef OPENPNP_FDCAN_PORT_H
#define OPENPNP_FDCAN_PORT_H

#include <stdbool.h>
#include <stdint.h>

#define FDCAN_PORT_MAX_DATA_BYTES 64U
#define FDCAN_PORT_RX_QUEUE_CAPACITY 4U

typedef enum
{
    FDCAN_PORT_MODE_INTERNAL_LOOPBACK = 0,
    FDCAN_PORT_MODE_NORMAL
} fdcan_port_mode_t;

typedef enum
{
    FDCAN_PORT_OK = 0,
    FDCAN_PORT_INIT_FAILED,
    FDCAN_PORT_FILTER_FAILED,
    FDCAN_PORT_TIMESTAMP_FAILED,
    FDCAN_PORT_START_FAILED,
    FDCAN_PORT_NOTIFICATION_FAILED,
    FDCAN_PORT_TRANSMIT_FAILED,
    FDCAN_PORT_TIMEOUT,
    FDCAN_PORT_RECEIVE_FAILED,
    FDCAN_PORT_CONTENT_MISMATCH
} fdcan_port_result_t;

typedef struct
{
    uint32_t identifier;
    uint8_t data[FDCAN_PORT_MAX_DATA_BYTES];
    uint8_t length;
    uint16_t timestamp;
    bool bit_rate_switch;
    bool fd_format;
} fdcan_port_frame_t;

typedef struct
{
    uint32_t received_frames;
    uint32_t dropped_frames;
    uint32_t receive_errors;
    uint32_t transmitted_frames;
    uint32_t transmit_errors;
} fdcan_port_diagnostics_t;

extern volatile fdcan_port_diagnostics_t g_fdcan_port_diagnostics;

fdcan_port_result_t fdcan_port_init(fdcan_port_mode_t mode);
fdcan_port_result_t fdcan_port_run_loopback_test(uint16_t *receive_timestamp,
                                                 uint16_t *timestamp_after_receive);
bool fdcan_port_send(const fdcan_port_frame_t *frame);
bool fdcan_port_receive(fdcan_port_frame_t *frame);
uint16_t fdcan_port_timestamp_now(void);
void fdcan_port_irq_handler(void);

#endif
