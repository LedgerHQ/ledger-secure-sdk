/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/
typedef enum {
    CCID_STATE_IDLE = 0x00,
} ccid_state_t;

typedef enum {
    CCID_PIN_OP_VERIFICATION = 0x00,
    CCID_PIN_OP_MODIFICATION = 0x01,
    CCID_PIN_OP_TRANSFER     = 0x02,
    CCID_PIN_OP_WAIT_ICC_RSP = 0x03,
    CCID_PIN_OP_CANCEL       = 0x04,
} ccid_pin_operation_t;

typedef enum {
    // Bulk OUT Messages
    CCID_COMMAND_PC_TO_RDR_SET_PARAMETERS   = 0x61,
    CCID_COMMAND_PC_TO_RDR_ICC_POWER_ON     = 0x62,
    CCID_COMMAND_PC_TO_RDR_ICC_POWER_OFF    = 0x63,
    CCID_COMMAND_PC_TO_RDR_GET_SLOT_STATUS  = 0x65,
    CCID_COMMAND_PC_TO_RDR_SECURE           = 0x69,
    CCID_COMMAND_PC_TO_RDR_T0_APDU          = 0x6A,
    CCID_COMMAND_PC_TO_RDR_ESCAPE           = 0x6B,
    CCID_COMMAND_PC_TO_RDR_GET_PARAMETERS   = 0x6C,
    CCID_COMMAND_PC_TO_RDR_RESET_PARAMETERS = 0x6D,
    CCID_COMMAND_PC_TO_RDR_ICC_CLOCK        = 0x6E,
    CCID_COMMAND_PC_TO_RDR_XFR_BLOCK        = 0x6F,
    CCID_COMMAND_PC_TO_RDR_MECHANICAL       = 0x71,
    CCID_COMMAND_PC_TO_RDR_ABORT            = 0x72,
    CCID_COMMAND_PC_TO_RDR_SET_DR_CLK_FREQ  = 0x73,

    // Bulk IN Messages
    CCID_COMMAND_RDR_TO_PC_DATA_BLOCK  = 0x80,
    CCID_COMMAND_RDR_TO_PC_SLOT_STATUS = 0x81,
    CCID_COMMAND_RDR_TO_PC_PARAMETERS  = 0x82,
    CCID_COMMAND_RDR_TO_PC_ESCAPE      = 0x83,
    CCID_COMMAND_RDR_TO_PC_DR_CLK_FREQ = 0x84,
} ccid_command_t;

typedef enum {
    // ICC status
    CCID_SLOT_STATUS_ICC_PRESENT_AND_ACTIVE   = 0x00,
    CCID_SLOT_STATUS_ICC_PRESENT_AND_INACTIVE = 0x01,
    CCID_SLOT_STATUS_ICC_NOT_PRESENT          = 0x02,
    // Command status
    CCID_SLOT_STATUS_CMD_PROCESSED_WITHOUT_ERROR  = 0x00,
    CCID_SLOT_STATUS_CMD_FAILED                   = 0x40,
    CCID_SLOT_STATUS_CMD_TIME_EXTENSION_REQUESTED = 0x80,
} ccid_rx_slot_status_t;

typedef enum {
    CCID_ERROR_CMD_ABORTED                = 0xFF,
    CCID_ERROR_ICC_MUTE                   = 0xFE,
    CCID_ERROR_XFR_PARITY_ERROR           = 0xFD,
    CCID_ERROR_XFR_OVERRUN                = 0xFC,
    CCID_ERROR_HW_ERROR                   = 0xFB,
    CCID_ERROR_BAD_ATR_TS                 = 0xF8,
    CCID_ERROR_BAD_ATR_TCK                = 0xF7,
    CCID_ERROR_ICC_PROTOCOL_NOT_SUPPORTED = 0xF6,
    CCID_ERROR_ICC_CLASS_NOT_SUPPORTED    = 0xF5,
    CCID_ERROR_PROCEDURE_BYTE_CONFLICT    = 0xF4,
    CCID_ERROR_DEACTIVATED_PROTOCOL       = 0xF3,
    CCID_ERROR_BUSY_WITH_AUTO_SEQUENCE    = 0xF2,
    CCID_ERROR_PIN_TIMEOUT                = 0xF0,
    CCID_ERROR_PIN_CANCELLED              = 0xEF,
    CCID_ERROR_CMD_SLOT_BUSY              = 0xE0,
    CCID_ERROR_CMD_NOT_SUPPORTED          = 0x00,

    CCID_ERROR_CMD_BAD_DWLENGTH        = 0x01,
    CCID_ERROR_CMD_SLOT_DOES_NOT_EXIST = 0x05,
    CCID_ERROR_CMD_BAD_POWER_SELECT    = 0x07,
    CCID_ERROR_CMD_INVALID_PROTOCOL    = 0x07,
    CCID_ERROR_CMD_INVALID_WI          = 0x0D,
    CCID_ERROR_CMD_INVALID_CLOCK_STOP  = 0x0E,

    CCID_ERROR_CMD_NO_ERROR = 0x81,

} ccid_error_t;

// Transport
typedef enum {
    CCID_MSG_STATUS_WAITING,
    CCID_MSG_STATUS_NEED_MORE_DATA,
    CCID_MSG_STATUS_COMPLETE,
    CCID_MSG_STATUS_PROCESSING,
} ccid_msg_status_e;

typedef union {
    struct {
        uint8_t  msg_type;
        uint32_t length;
        uint8_t  slot_number;
        uint8_t  seq_number;
        uint8_t  specific[3];
    } out;

    struct {
        uint8_t  msg_type;
        uint32_t length;
        uint8_t  slot_number;
        uint8_t  seq_number;
        uint8_t  specific_backup[3];
        uint8_t  status;
        uint8_t  error;
        uint8_t  specific;
    } in;
} ccid_msg_bulk_header_t;

typedef struct {
    uint8_t bmFindexDindex;
    uint8_t bmTCCKST0;
    uint8_t bGuardTimeT0;
    uint8_t bWaitingIntegerT0;
    uint8_t bClockStop;
} ccid_protocol_data_t0_t;

typedef struct {
    uint8_t  rx_msg_status;  // ccid_msg_status_e
    uint8_t *rx_msg_buffer;
    uint16_t rx_msg_buffer_size;
    uint16_t rx_msg_length;
    uint16_t rx_msg_apdu_offset;

    ccid_msg_bulk_header_t bulk_msg_header;

    const uint8_t *tx_message_buffer;
    uint16_t       tx_message_length;
    uint16_t       tx_message_offset;

    uint8_t *tx_packet_buffer;
    uint8_t  tx_packet_length;

    ccid_protocol_data_t0_t protocol_data;

    uint32_t clock_frequency;
    uint32_t data_rate;

} ccid_transport_t;

// Abort parameters
typedef struct {
    uint8_t requested;
    uint8_t seq;
} ccid_abort_t;

// Device
typedef enum {
    CCID_SLOT_STATUS_IDLE,
    CCID_SLOT_STATUS_BUSY,
} ccid_slot_status_e;

typedef struct {
    uint8_t card_inserted;

    uint8_t slot_status;  // ccid_slot_status_e

    ccid_transport_t transport;
    ccid_abort_t     abort;

} ccid_device_t;

typedef enum {
    CCID_VOLTAGE_AUTOMATIC = 0x00,
    CCID_VOLTAGE_5_0V      = 0x01,
    CCID_VOLTAGE_3_0V      = 0x02,
    CCID_VOLTAGE_1_8V      = 0x03,
    CCID_VOLTAGE_MAX       = CCID_VOLTAGE_1_8V + 1,
} ccid_voltage_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
