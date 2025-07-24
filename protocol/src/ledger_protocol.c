/*****************************************************************************
 *   (c) 2022-2025 Ledger SAS.
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

#include "ledger_protocol.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct protocol_header_s {
    uint16_t channel_id;
    uint8_t  tag;
    uint16_t seq_num;
    uint16_t size;
} protocol_header_t;

/* Private defines------------------------------------------------------------*/
#define TAG_GET_PROTOCOL_VERSION (0x00)
#define TAG_ALLOCATE_CHANNEL     (0x01)
#define TAG_PING                 (0x02)
#define TAG_ABORT                (0x03)
#define TAG_APDU                 (0x05)
#define TAG_MTU                  (0x08)
#define MTU_MIN_SIZE             (3)

#ifdef HAVE_PRINTF
// #define LOG_IO PRINTF
#define LOG_IO(...)
#else  // !HAVE_PRINTF
#define LOG_IO(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const uint8_t protocol_version[4] = {0x00, 0x00, 0x00, 0x00};

/* Private functions ---------------------------------------------------------*/
static ledger_protocol_result_t process_apdu_chunk(ledger_protocol_t *handle,
                                                   uint8_t           *buffer,
                                                   uint16_t           length,
                                                   uint8_t           *apdu_buffer,
                                                   uint16_t           apdu_buffer_size)
{
    ledger_protocol_result_t result = LP_ERROR_INVALID_PARAMETER;
    // Check the sequence number
    if ((length < 2) || ((uint16_t) U2BE(buffer, 0) != handle->rx_apdu_sequence_number)) {
        if (handle->rx_apdu_status == APDU_STATUS_NEED_MORE_DATA) {
            handle->rx_apdu_status = APDU_STATUS_WAITING;
            result                 = LP_ERROR_NOT_ENOUGH_SPACE;
            goto error;
        }
        else {
            // Ledger Live is well known for sending empty apdu chunk
            // after the last apdu chunk fits totally in the MTU
            return LP_SUCCESS;
        }
    }
    // Check total length presence
    if ((length < 4) && (handle->rx_apdu_sequence_number == 0)) {
        handle->rx_apdu_status = APDU_STATUS_WAITING;
        result                 = LP_ERROR_NOT_ENOUGH_SPACE;
        goto error;
    }

    if (handle->rx_apdu_sequence_number == 0) {
        // First chunk
        apdu_buffer[0]         = handle->type;
        handle->rx_apdu_status = APDU_STATUS_NEED_MORE_DATA;
        handle->rx_apdu_length = (uint16_t) U2BE(buffer, 2);
        // Check if we have enough space to store the apdu
        if (handle->rx_apdu_length > apdu_buffer_size) {
            LOG_IO("APDU WAITING - %d\n", handle->rx_apdu_length);
            handle->rx_apdu_status = APDU_STATUS_WAITING;
            handle->rx_apdu_length = apdu_buffer_size;
            result                 = LP_ERROR_NOT_ENOUGH_SPACE;
            goto error;
        }
        handle->rx_apdu_offset = 0;
        buffer                 = &buffer[4];
        length -= 4;
    }
    else {
        // Next chunk
        buffer = &buffer[2];
        length -= 2;
    }

    // Remove padding bytes if any
    if ((handle->rx_apdu_offset + length) > handle->rx_apdu_length) {
        length = handle->rx_apdu_length - handle->rx_apdu_offset;
    }
    if ((1 + handle->rx_apdu_offset + length) > apdu_buffer_size) {
        result = LP_ERROR_NOT_ENOUGH_SPACE;
        goto error;
    }

    memcpy(&apdu_buffer[1 + handle->rx_apdu_offset], buffer, length);
    handle->rx_apdu_offset += length;

    if (handle->rx_apdu_offset == handle->rx_apdu_length) {
        handle->rx_apdu_length++;  // include the type
        handle->rx_apdu_sequence_number = 0;
        handle->rx_apdu_status          = APDU_STATUS_COMPLETE;
        LOG_IO("APDU COMPLETE\n");
    }
    else {
        handle->rx_apdu_sequence_number++;
        handle->rx_apdu_status = APDU_STATUS_NEED_MORE_DATA;
        LOG_IO("APDU NEED MORE DATA\n");
    }
    result = LP_SUCCESS;

error:
    return result;
}

/* Exported functions --------------------------------------------------------*/
ledger_protocol_result_t LEDGER_PROTOCOL_init(ledger_protocol_t *handle, uint8_t type)
{
    ledger_protocol_result_t result = LP_ERROR_INVALID_PARAMETER;

    if (!handle) {
        goto error;
    }

    handle->rx_apdu_status          = APDU_STATUS_WAITING;
    handle->rx_apdu_sequence_number = 0;
    handle->type                    = type;
    result                          = LP_SUCCESS;

error:
    return result;
}

ledger_protocol_result_t LEDGER_PROTOCOL_rx(ledger_protocol_t *handle,
                                            uint8_t           *buffer,
                                            uint16_t           length,
                                            uint8_t           *proto_buf,
                                            uint8_t            proto_buf_size,
                                            uint8_t           *apdu_buffer,
                                            uint16_t           apdu_buffer_size,
                                            uint16_t           mtu)
{
    ledger_protocol_result_t result = LP_ERROR_INVALID_PARAMETER;
    protocol_header_t        header = {0};
    if (!handle || !buffer || (length < (sizeof(header.channel_id) + sizeof(header.tag)))
        || !proto_buf || proto_buf_size < sizeof(header)) {
        goto error;
    }

    memset(proto_buf, 0, proto_buf_size);
    memcpy(proto_buf, buffer, sizeof(header.channel_id));  // Copy channel ID

    switch (buffer[2]) {
        case TAG_GET_PROTOCOL_VERSION:
            LOG_IO("TAG_GET_PROTOCOL_VERSION\n");
            proto_buf[2]            = TAG_GET_PROTOCOL_VERSION;
            handle->tx_chunk_length = MIN((uint8_t) sizeof(protocol_version), (proto_buf_size - 3));
            memcpy(&proto_buf[3], protocol_version, handle->tx_chunk_length);
            handle->tx_chunk_length += 3;
            result = LP_SUCCESS;
            break;

        case TAG_ALLOCATE_CHANNEL:
            LOG_IO("TAG_ALLOCATE_CHANNEL\n");
            proto_buf[2]            = TAG_ALLOCATE_CHANNEL;
            handle->tx_chunk_length = 3;
            result                  = LP_SUCCESS;
            break;

        case TAG_PING:
            LOG_IO("TAG_PING\n");
            handle->tx_chunk_length = MIN(proto_buf_size, length);
            memcpy(proto_buf, buffer, handle->tx_chunk_length);
            result = LP_SUCCESS;
            break;

        case TAG_APDU:
            LOG_IO("TAG_APDU\n");
            result
                = process_apdu_chunk(handle, &buffer[3], length - 3, apdu_buffer, apdu_buffer_size);
            break;

        case TAG_MTU:
            LOG_IO("TAG_MTU\n");
            if (!mtu) {
                mtu = handle->mtu;
            }
            if (mtu >= MTU_MIN_SIZE) {
                proto_buf[2]            = TAG_MTU;
                proto_buf[3]            = 0x00;
                proto_buf[4]            = 0x00;
                proto_buf[5]            = 0x00;
                proto_buf[6]            = 0x01;
                proto_buf[7]            = mtu - 2;
                handle->tx_chunk_length = 8;
                U2BE_ENCODE(proto_buf, 4, mtu - 2);
                result = LP_SUCCESS;
            }
            else {
                result = LP_ERROR_INVALID_PARAMETER;
            }
            break;

        default:
            // Unsupported command
            result = LP_ERROR_NOT_SUPPORTED;
            break;
    }

error:
    return result;
}

ledger_protocol_result_t LEDGER_PROTOCOL_tx(ledger_protocol_t *handle,
                                            const uint8_t     *buffer,
                                            uint16_t           length,
                                            uint8_t           *proto_buf,
                                            uint8_t            proto_buf_size,
                                            uint16_t           mtu)
{
    ledger_protocol_result_t result = LP_ERROR_INVALID_PARAMETER;
    protocol_header_t        header = {0};
    if (!handle || (!buffer && !handle->tx_apdu_buffer) || !proto_buf
        || proto_buf_size < sizeof(header)) {
        goto error;
    }
    if (buffer) {
        LOG_IO("FIRST CHUNK\n");
        handle->tx_apdu_buffer          = buffer;
        handle->tx_apdu_length          = length;
        handle->tx_apdu_sequence_number = 0;
        handle->tx_apdu_offset          = 0;
    }
    else {
        LOG_IO("NEXT CHUNK\n");
    }
    uint16_t tx_chunk_offset
        = sizeof(header.channel_id);  // Because channel id has been already filled beforehand
    memset(&proto_buf[tx_chunk_offset], 0, proto_buf_size - tx_chunk_offset);

    proto_buf[tx_chunk_offset++] = TAG_APDU;

    U2BE_ENCODE(proto_buf, tx_chunk_offset, handle->tx_apdu_sequence_number);
    tx_chunk_offset += 2;

    if (handle->tx_apdu_sequence_number == 0) {
        U2BE_ENCODE(proto_buf, tx_chunk_offset, handle->tx_apdu_length);
        tx_chunk_offset += 2;
    }
    if (!mtu) {
        mtu = handle->mtu;
    }
    if (mtu < MTU_MIN_SIZE) {
        goto error;
    }

    if ((handle->tx_apdu_length + tx_chunk_offset) > (mtu + handle->tx_apdu_offset)) {
        if (mtu < tx_chunk_offset) {
            goto error;
        }
        if (mtu - tx_chunk_offset > proto_buf_size - tx_chunk_offset) {
            goto error;
        }
        // Remaining buffer length doesn't fit the chunk
        memcpy(&proto_buf[tx_chunk_offset],
               &handle->tx_apdu_buffer[handle->tx_apdu_offset],
               mtu - tx_chunk_offset);
        handle->tx_apdu_offset += mtu - tx_chunk_offset;
        handle->tx_apdu_sequence_number++;
        tx_chunk_offset = mtu;
    }
    else {
        if (handle->tx_apdu_length - handle->tx_apdu_offset > proto_buf_size - tx_chunk_offset) {
            goto error;
        }
        // Remaining buffer fits the chunk
        memcpy(&proto_buf[tx_chunk_offset],
               &handle->tx_apdu_buffer[handle->tx_apdu_offset],
               handle->tx_apdu_length - handle->tx_apdu_offset);
        tx_chunk_offset += (handle->tx_apdu_length - handle->tx_apdu_offset);
        handle->tx_apdu_offset = handle->tx_apdu_length;
        handle->tx_apdu_buffer = NULL;
    }
    handle->tx_chunk_length = tx_chunk_offset;
    result                  = LP_SUCCESS;
    LOG_IO(" %d\n", handle->tx_chunk_length);

error:
    return result;
}
