/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
enum {
    APDU_STATUS_WAITING,
    APDU_STATUS_NEED_MORE_DATA,
    APDU_STATUS_COMPLETE,
};

/* Exported types, structures, unions ----------------------------------------*/
typedef enum ledger_protocol_result_e {
    LP_SUCCESS,
    LP_ERROR_INVALID_PARAMETER,
    LP_ERROR_INVALID_STATE,
    LP_ERROR_NOT_ENOUGH_SPACE,
    LP_ERROR_NOT_SUPPORTED,
} ledger_protocol_result_t;

typedef struct ledger_protocol_s {
    uint8_t type;

    const uint8_t *tx_apdu_buffer;
    uint16_t       tx_apdu_length;
    uint16_t       tx_apdu_sequence_number;
    uint16_t       tx_apdu_offset;

    uint8_t tx_chunk_length;

    uint8_t  rx_apdu_status;
    uint16_t rx_apdu_sequence_number;
    uint16_t rx_apdu_length;
    uint16_t rx_apdu_offset;

    uint16_t mtu;
    uint8_t  mtu_negotiated;
} ledger_protocol_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
ledger_protocol_result_t LEDGER_PROTOCOL_init(ledger_protocol_t *data, uint8_t type);
ledger_protocol_result_t LEDGER_PROTOCOL_rx(ledger_protocol_t *data,
                                            uint8_t           *buffer,
                                            uint16_t           length,
                                            uint8_t           *proto_buf,
                                            uint8_t            proto_buff_size,
                                            uint8_t           *apdu_buffer,
                                            uint16_t           apdu_buffer_size);
ledger_protocol_result_t LEDGER_PROTOCOL_tx(ledger_protocol_t *data,
                                            const uint8_t     *buffer,
                                            uint16_t           length,
                                            uint8_t           *proto_buf,
                                            uint8_t            proto_buf_size);
