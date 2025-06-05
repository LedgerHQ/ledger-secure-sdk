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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "os.h"

#include "ccid_types.h"
#include "ccid_transport.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#define CCID_HEADER_SIZE  (10)
#define CCID_DEFAULT_FIDI (0x11)

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void CCID_TRANSPORT_init(ccid_transport_t *handle)
{
    if (!handle) {
        return;
    }

    handle->rx_msg_status = CCID_MSG_STATUS_WAITING;
    CCID_TRANSPORT_reset_parameters(handle);
}

void CCID_TRANSPORT_rx(ccid_transport_t *handle, uint8_t *buffer, uint16_t length)
{
    if (!handle || !buffer) {
        return;
    }

    if (handle->rx_msg_status == CCID_MSG_STATUS_WAITING) {
        if (length >= CCID_HEADER_SIZE) {
            handle->bulk_msg_header.out.msg_type    = buffer[0];
            handle->bulk_msg_header.out.length      = U4LE(buffer, 1);
            handle->bulk_msg_header.out.slot_number = buffer[5];
            handle->bulk_msg_header.out.seq_number  = buffer[6];
            memcpy(handle->bulk_msg_header.out.specific, &buffer[7], 3);
            if (handle->bulk_msg_header.out.length > handle->rx_msg_buffer_size) {
                // Todoo : incorrect size
            }
            else {
                handle->rx_msg_length = length - CCID_HEADER_SIZE;
                if (handle->bulk_msg_header.out.length <= handle->rx_msg_length) {
                    // Msg is complete
                    LOG_IO("CCID complete\n");
                    handle->rx_msg_length      = handle->bulk_msg_header.out.length;
                    handle->rx_msg_status      = CCID_MSG_STATUS_COMPLETE;
                    handle->rx_msg_apdu_offset = 0;
                }
                else {
                    // Msg not complete
                    LOG_IO("CCID not complete\n");
                    handle->rx_msg_status = CCID_MSG_STATUS_NEED_MORE_DATA;
                }
                memmove(handle->rx_msg_buffer, &buffer[CCID_HEADER_SIZE], handle->rx_msg_length);
            }
        }
    }
    else if (handle->rx_msg_status == CCID_MSG_STATUS_NEED_MORE_DATA) {
        // used for secure memmove
        uint16_t movelength = 0;
        if (handle->bulk_msg_header.out.length <= (handle->rx_msg_length + length)) {
            // Msg is complete
            if (__builtin_sub_overflow(
                    handle->bulk_msg_header.out.length, handle->rx_msg_length, &movelength)
                || handle->rx_msg_buffer_size <= movelength) {
                return;
            }
            memmove(&handle->rx_msg_buffer[handle->rx_msg_length],
                    buffer,
                    handle->bulk_msg_header.out.length - handle->rx_msg_length);
            handle->rx_msg_length      = handle->bulk_msg_header.out.length;
            handle->rx_msg_status      = CCID_MSG_STATUS_COMPLETE;
            handle->rx_msg_apdu_offset = 0;
            LOG_IO("CCID complete\n");
        }
        else {
            // Msg not complete
            if (__builtin_add_overflow(length, handle->rx_msg_length, &movelength)
                || handle->rx_msg_buffer_size <= movelength) {
                return;
            }
            memmove(&handle->rx_msg_buffer[handle->rx_msg_length], buffer, length);
            handle->rx_msg_length += length;
            LOG_IO("CCID not complete\n");
        }
    }
}

void CCID_TRANSPORT_tx(ccid_transport_t *handle, const uint8_t *buffer, uint16_t length)
{
    if (!handle || (!buffer && !handle->tx_message_buffer)) {
        return;
    }

    if (buffer) {
        LOG_IO("INITIALIZATION PACKET\n");
        handle->tx_message_buffer         = buffer;
        handle->tx_message_length         = length;
        handle->tx_message_offset         = 0;
        handle->bulk_msg_header.in.length = length;
    }
    else {
        LOG_IO("CONTINUATION PACKET\n");
    }

    uint16_t tx_packet_offset = 0;
    memset(handle->tx_packet_buffer, 0, handle->tx_packet_buffer_size);

    // Fill header
    if (buffer) {
        handle->tx_packet_buffer[0] = handle->bulk_msg_header.in.msg_type;
        U4LE_ENCODE(handle->tx_packet_buffer, 1, handle->bulk_msg_header.in.length);
        handle->tx_packet_buffer[5] = handle->bulk_msg_header.in.slot_number;
        handle->tx_packet_buffer[6] = handle->bulk_msg_header.in.seq_number;
        handle->tx_packet_buffer[7] = handle->bulk_msg_header.in.status;
        handle->tx_packet_buffer[8] = handle->bulk_msg_header.in.error;
        handle->tx_packet_buffer[9] = handle->bulk_msg_header.in.specific;
        tx_packet_offset            = 10;
    }

    if ((handle->tx_message_length + tx_packet_offset)
        >= (handle->tx_packet_buffer_size + handle->tx_message_offset)) {
        // Remaining message length doesn't fit the max packet size
        memcpy(&handle->tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               handle->tx_packet_buffer_size - tx_packet_offset);
        handle->tx_message_offset += handle->tx_packet_buffer_size - tx_packet_offset;
        tx_packet_offset = handle->tx_packet_buffer_size;
    }
    else {
        // Remaining message fits the max packet size
        memcpy(&handle->tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               handle->tx_message_length - handle->tx_message_offset);
        tx_packet_offset += (handle->tx_message_length - handle->tx_message_offset);
        handle->tx_message_offset = handle->tx_message_length;
        handle->tx_message_buffer = NULL;
        handle->rx_msg_status     = CCID_MSG_STATUS_WAITING;
    }

    handle->tx_packet_length = tx_packet_offset;
    LOG_IO(" %d\n", handle->tx_packet_length);
}

void CCID_TRANSPORT_reset_parameters(ccid_transport_t *handle)
{
    if (!handle) {
        return;
    }

    handle->protocol_data.bmFindexDindex    = CCID_DEFAULT_FIDI;
    handle->protocol_data.bmTCCKST0         = 0x00;
    handle->protocol_data.bGuardTimeT0      = 0x00;
    handle->protocol_data.bWaitingIntegerT0 = 0x0A;
    handle->protocol_data.bClockStop        = 0x00;  // Stopping the Clock is not allowed
}
