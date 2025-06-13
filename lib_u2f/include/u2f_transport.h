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
#include "u2f_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    U2F_TRANSPORT_TYPE_USB_HID = 0x00,
    U2F_TRANSPORT_TYPE_BLE     = 0x01,
} u2f_transport_type_t;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    u2f_transport_type_t type;

    uint32_t cid;
    uint8_t  state;

    u2f_error_t error;
    uint32_t    tx_cid;

    const uint8_t *tx_message_buffer;
    uint16_t       tx_message_length;
    uint16_t       tx_message_sequence_number;
    uint16_t       tx_message_offset;

    uint8_t *tx_packet_buffer;
    uint16_t tx_packet_buffer_size;
    uint8_t  tx_packet_length;

    uint8_t *rx_message_buffer;
    uint16_t rx_message_buffer_size;
    uint16_t rx_message_expected_sequence_number;
    uint16_t rx_message_length;
    uint16_t rx_message_offset;

} u2f_transport_t;

/* Exported defines   --------------------------------------------------------*/
#define U2F_FORBIDDEN_CID (0x00000000)
#define U2F_BROADCAST_CID (0xFFFFFFFF)

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void U2F_TRANSPORT_init(u2f_transport_t *handle, uint8_t type);
void U2F_TRANSPORT_rx(u2f_transport_t *handle, uint8_t *buffer, uint16_t length);
void U2F_TRANSPORT_tx(u2f_transport_t *handle, uint8_t cmd, const uint8_t *buffer, uint16_t length);
