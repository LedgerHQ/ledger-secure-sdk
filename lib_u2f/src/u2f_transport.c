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
#include <string.h>

#include "os.h"
#include "os_math.h"
#include "os_utils.h"
#include "os_io.h"

#include "u2f_types.h"
#include "u2f_transport.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#ifdef HAVE_PRINTF
// #define LOG_IO PRINTF
#define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static u2f_error_t process_packet(u2f_transport_t *handle, uint8_t *buffer, uint16_t length);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
static u2f_error_t process_packet(u2f_transport_t *handle, uint8_t *buffer, uint16_t length)
{
    u2f_error_t error = CTAP1_ERR_SUCCESS;

    // Check CID for USB HID transport
    if (handle->type == U2F_TRANSPORT_TYPE_USB_HID) {
        if (length < 4) {
            // CID not complete, answer with broadcast CID?
            error       = CTAP1_ERR_OTHER;
            handle->cid = U2F_BROADCAST_CID;
            goto end;
        }
        uint32_t message_cid = U4BE(buffer, 0);
        handle->tx_cid       = message_cid;
        if (message_cid == U2F_FORBIDDEN_CID) {
            // Forbidden CID
            error = CTAP1_ERR_INVALID_CHANNEL;
            goto end;
        }
        else if ((message_cid == U2F_BROADCAST_CID) && (length >= 5)
                 && (buffer[4] != (U2F_COMMAND_HID_INIT | 0x80))) {
            // Broadcast CID but not an init message
            error       = CTAP1_ERR_INVALID_CHANNEL;
            handle->cid = message_cid;
            goto end;
        }
        else if ((handle->cid != U2F_FORBIDDEN_CID) && (handle->cid != message_cid)) {
            // CID is already set
            error = CTAP1_ERR_CHANNEL_BUSY;
            goto end;
        }
        else if ((handle->state > U2F_STATE_CMD_FRAMING) && (length >= 5)
                 && (buffer[4] != (U2F_COMMAND_HID_CANCEL | 0x80))
                 && handle->state != U2F_STATE_CMD_PROCESSING_CANCEL) {
            // Good CID but a request is already in process.
            // If previous command is CTAPHID_CANCEL (supposedly without response) a new one is
            // still authorized.
            // TODO: to check the use case when on-going CTAPHID_CANCEL needs to interrupt UI and to
            // respond with ERROR_KEEPALIVE_CANCEL while a new U2F_TRANSPORT_TYPE_USB_HID is coming.
            error = CTAP1_ERR_CHANNEL_BUSY;
            goto end;
        }
        else if (handle->cid == U2F_FORBIDDEN_CID) {
            // Set new CID
            handle->cid = message_cid;
        }
        buffer += 4;
        length -= 4;
    }

    // Check header length
    if (length < 1) {
        error = CTAP1_ERR_OTHER;
        goto end;
    }

    if (buffer[0] & 0x80) {
        // Initialization packet
        if (length < 3) {
            error = CTAP1_ERR_OTHER;
            goto end;
        }

        // Check if packet will fit in the rx buffer
        handle->rx_message_length = (uint16_t) U2BE(buffer, 1) + 3;
        if (handle->rx_message_length > handle->rx_message_buffer_size) {
            error = CTAP1_ERR_INVALID_LENGTH;
            goto end;
        }

        if ((handle->rx_message_length <= 3) && (buffer[0] == (U2F_COMMAND_HID_CBOR | 0x80))) {
            handle->rx_message_buffer[0] = U2F_COMMAND_HID_CBOR;
            error                        = CTAP2_ERR_INVALID_CBOR;
            goto end;
        }

        handle->state                                          = U2F_STATE_CMD_FRAMING;
        handle->rx_message_offset                              = 0;
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[0] & 0x7F;  // CMD
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[1];         // BCNTH
        handle->rx_message_buffer[handle->rx_message_offset++] = buffer[2];         // BCNTL
        handle->rx_message_expected_sequence_number            = 0;
        buffer += 3;
        length -= 3;
    }
    else {
        // Continuation packet
        if (handle->state != U2F_STATE_CMD_FRAMING) {
            error = CTAP1_ERR_OTHER;
            goto end;
        }
        else if (buffer[0] != handle->rx_message_expected_sequence_number) {
            error = CTAP1_ERR_INVALID_SEQ;
            goto end;
        }
        handle->rx_message_expected_sequence_number++;
        buffer += 1;
        length -= 1;
    }

    // prevent integer underflows in the rest of the operations
    if (handle->rx_message_length < handle->rx_message_offset) {
        error = CTAP1_ERR_OTHER;
        goto end;
    }
    if (length > handle->rx_message_length - handle->rx_message_offset) {
        length = handle->rx_message_length - handle->rx_message_offset;
    }
    memcpy(&handle->rx_message_buffer[handle->rx_message_offset], buffer, length);
    handle->rx_message_offset += length;

    if (handle->rx_message_offset == handle->rx_message_length) {
        handle->state = U2F_STATE_CMD_COMPLETE;
    }

end:
    return error;
}

/* Exported functions --------------------------------------------------------*/
void U2F_TRANSPORT_init(u2f_transport_t *handle, uint8_t type)
{
    if (!handle) {
        return;
    }

    handle->state = U2F_STATE_IDLE;
    handle->cid   = U2F_FORBIDDEN_CID;
    handle->type  = type;
}

void U2F_TRANSPORT_rx(u2f_transport_t *handle, uint8_t *buffer, uint16_t length)
{
    if (!handle || !buffer || length < 3) {
        return;
    }

    handle->error = process_packet(handle, buffer, length);
}

void U2F_TRANSPORT_tx(u2f_transport_t *handle,
                      uint8_t          cmd,
                      const uint8_t   *buffer,
                      uint16_t         length,
                      uint8_t         *tx_packet_buffer,
                      uint16_t         tx_packet_buffer_size)
{
    if (!handle || (!buffer && !handle->tx_message_buffer)) {
        return;
    }

    if (buffer) {
        LOG_IO("Tx : INITIALIZATION PACKET\n");
        handle->tx_message_buffer          = buffer;
        handle->tx_message_length          = length;
        handle->tx_message_sequence_number = 0;
        handle->tx_message_offset          = 0;
        handle->tx_packet_length           = 0;
        memset(tx_packet_buffer, 0, tx_packet_buffer_size);
    }
    else {
        LOG_IO("Tx : CONTINUATION PACKET\n");
    }

    uint16_t tx_packet_offset = 0;
    memset(tx_packet_buffer, 0, tx_packet_buffer_size);

    // Add CID if necessary
    if (handle->type == U2F_TRANSPORT_TYPE_USB_HID) {
        U4BE_ENCODE(tx_packet_buffer, 0, handle->tx_cid);
        tx_packet_offset += 4;
    }

    // Fill header
    if (buffer) {
        tx_packet_buffer[tx_packet_offset++] = cmd | 0x80;        // CMD
        U2BE_ENCODE(tx_packet_buffer, tx_packet_offset, length);  // BCNT
        tx_packet_offset += 2;
    }
    else {
        tx_packet_buffer[tx_packet_offset++] = handle->tx_message_sequence_number++;  // SEQ
    }

    if ((handle->tx_message_length + tx_packet_offset)
        > (tx_packet_buffer_size + handle->tx_message_offset)) {
        // Remaining message length doesn't fit the max packet size
        memcpy(&tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               tx_packet_buffer_size - tx_packet_offset);
        handle->tx_message_offset += tx_packet_buffer_size - tx_packet_offset;
        tx_packet_offset = tx_packet_buffer_size;
    }
    else {
        // Remaining message fits the max packet size
        memcpy(&tx_packet_buffer[tx_packet_offset],
               &handle->tx_message_buffer[handle->tx_message_offset],
               handle->tx_message_length - handle->tx_message_offset);
        tx_packet_offset += (handle->tx_message_length - handle->tx_message_offset);
        handle->tx_message_offset = handle->tx_message_length;
        handle->tx_message_buffer = NULL;
        handle->cid               = U2F_FORBIDDEN_CID;
    }

    handle->tx_packet_length = tx_packet_offset;
    LOG_IO(" %d\n", handle->tx_packet_length);
}
