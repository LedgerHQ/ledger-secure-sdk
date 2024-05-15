
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2022 Ledger
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
 ********************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "os.h"
#include "os_math.h"
#include "os_utils.h"
#include "os_io_seproxyhal.h"

#include "ledger_protocol.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#define TAG_GET_PROTOCOL_VERSION (0x00)
#define TAG_ALLOCATE_CHANNEL     (0x01)
#define TAG_PING                 (0x02)
#define TAG_ABORT                (0x03)
#define TAG_APDU                 (0x05)
#define TAG_MTU                  (0x08)

#ifdef HAVE_PRINTF
#define LOG_BLE_PROTOCOL PRINTF
#else  // !HAVE_PRINTF
#define LOG_BLE_PROTOCOL(...)
#endif  // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void process_apdu_chunk(ledger_protocol_t *ctx, const uint8_t *buffer, uint16_t length);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const uint8_t protocol_version[4] = {0x00, 0x00, 0x00, 0x00};

/* Private functions ---------------------------------------------------------*/
static void process_apdu_chunk(ledger_protocol_t *ctx, const uint8_t *buffer, uint16_t length)
{
    // Check the sequence number
    if ((length < 2) || ((uint16_t) U2BE(buffer, 0) != ctx->rx_apdu_sequence_number)) {
        ctx->rx_apdu_status = APDU_STATUS_WAITING;
        return;
    }
    // Check total length presence
    if ((length < 4) && (ctx->rx_apdu_sequence_number == 0)) {
        ctx->rx_apdu_status = APDU_STATUS_WAITING;
        return;
    }

    if (ctx->rx_apdu_sequence_number == 0) {
        // First chunk
        ctx->rx_apdu_status = APDU_STATUS_NEED_MORE_DATA;
        ctx->rx_apdu_length = (uint16_t) U2BE(buffer, 2);
        ctx->rx_apdu_offset = 0;
        // Check if we have enough space to store the apdu
        if (ctx->rx_apdu_length > ctx->rx_apdu_buffer_max_length) {
            LOG_BLE_PROTOCOL("APDU WAITING - %d\n", ctx->rx_apdu_length);
            ctx->rx_apdu_length = 0;
            ctx->rx_apdu_status = APDU_STATUS_WAITING;
            return;
        }
        buffer = &buffer[4];
        length -= 4;
    }
    else {
        // Next chunk
        buffer = &buffer[2];
        length -= 2;
    }

    if ((ctx->rx_apdu_offset + length) > ctx->rx_apdu_length) {
        length = ctx->rx_apdu_length - ctx->rx_apdu_offset;
    }

    memcpy(&ctx->rx_apdu_buffer[ctx->rx_apdu_offset], buffer, length);
    ctx->rx_apdu_offset += length;

    if (ctx->rx_apdu_offset == ctx->rx_apdu_length) {
        ctx->rx_apdu_sequence_number = 0;
        ctx->rx_apdu_status          = APDU_STATUS_COMPLETE;
        LOG_BLE_PROTOCOL("APDU COMPLETE\n");
    }
    else {
        ctx->rx_apdu_sequence_number++;
        ctx->rx_apdu_status = APDU_STATUS_NEED_MORE_DATA;
        LOG_BLE_PROTOCOL("APDU NEED MORE DATA\n");
    }
}

/* Exported functions --------------------------------------------------------*/
void LEDGER_PROTOCOL_init(ledger_protocol_t *ctx)
{
    ctx->rx_apdu_status          = APDU_STATUS_WAITING;
    ctx->rx_apdu_sequence_number = 0;
}

void LEDGER_PROTOCOL_rx(ledger_protocol_t *ctx, const uint8_t *buffer, uint16_t length)
{
    if (!ctx || !buffer || length < 3) {
        return;
    }

    memset(ctx->tx_chunk, 0, sizeof(ctx->tx_chunk));

    // For all calls to this function, the buffer was pre-initialized to the same constant
    // In order for the input buffer to be 'const', this constant is forced directly here
    ctx->tx_chunk[0] = 0xDE;
    ctx->tx_chunk[1] = 0xF1;

    switch (buffer[2]) {
        case TAG_GET_PROTOCOL_VERSION:
            LOG_BLE_PROTOCOL("TAG_GET_PROTOCOL_VERSION\n");
            ctx->tx_chunk[2]     = TAG_GET_PROTOCOL_VERSION;
            ctx->tx_chunk_length = MIN(sizeof(protocol_version), (sizeof(ctx->tx_chunk) - 3));
            memcpy(&ctx->tx_chunk[3], protocol_version, ctx->tx_chunk_length);
            ctx->tx_chunk_length += 3;
            break;

        case TAG_ALLOCATE_CHANNEL:
            LOG_BLE_PROTOCOL("TAG_ALLOCATE_CHANNEL\n");
            ctx->tx_chunk[2]     = TAG_ALLOCATE_CHANNEL;
            ctx->tx_chunk_length = 3;
            break;

        case TAG_PING:
            LOG_BLE_PROTOCOL("TAG_PING\n");
            ctx->tx_chunk_length = MIN(sizeof(ctx->tx_chunk), length);
            memcpy(ctx->tx_chunk, buffer, ctx->tx_chunk_length);
            break;

        case TAG_APDU:
            LOG_BLE_PROTOCOL("TAG_APDU\n");
            process_apdu_chunk(ctx, &buffer[3], length - 3);
            break;

        case TAG_MTU:
            LOG_BLE_PROTOCOL("TAG_MTU\n");
            ctx->tx_chunk[2]     = TAG_MTU;
            ctx->tx_chunk[3]     = 0x00;
            ctx->tx_chunk[4]     = 0x00;
            ctx->tx_chunk[5]     = 0x00;
            ctx->tx_chunk[6]     = 0x01;
            ctx->tx_chunk[7]     = ctx->mtu - 2;
            ctx->tx_chunk_length = 8;
            break;

        default:
            // Unsupported command
            break;
    }
}

void LEDGER_PROTOCOL_tx(ledger_protocol_t *ctx, const uint8_t *buffer, uint16_t length)
{
    if (!ctx || (!buffer && !ctx->tx_apdu_buffer)) {
        return;
    }
    if (buffer) {
        LOG_BLE_PROTOCOL("FIRST CHUNK");
        ctx->tx_apdu_buffer          = buffer;
        ctx->tx_apdu_length          = length;
        ctx->tx_apdu_sequence_number = 0;
        ctx->tx_apdu_offset          = 0;
        memset(ctx->tx_chunk, 0, sizeof(ctx->tx_chunk));
    }
    else {
        LOG_BLE_PROTOCOL("NEXT CHUNK");
    }

    uint16_t tx_chunk_offset = 2;  // Because channel id has been already filled beforehand

    ctx->tx_chunk[tx_chunk_offset++] = TAG_APDU;

    U2BE_ENCODE(ctx->tx_chunk, tx_chunk_offset, ctx->tx_apdu_sequence_number);
    tx_chunk_offset += 2;

    if (ctx->tx_apdu_sequence_number == 0) {
        U2BE_ENCODE(ctx->tx_chunk, tx_chunk_offset, ctx->tx_apdu_length);
        tx_chunk_offset += 2;
    }
    if ((ctx->tx_apdu_length + tx_chunk_offset) >= (ctx->mtu + ctx->tx_apdu_offset)) {
        // Remaining buffer length doesn't fit the chunk
        memcpy(&ctx->tx_chunk[tx_chunk_offset],
               &ctx->tx_apdu_buffer[ctx->tx_apdu_offset],
               ctx->mtu - tx_chunk_offset);
        ctx->tx_apdu_offset += ctx->mtu - tx_chunk_offset;
        ctx->tx_apdu_sequence_number++;
        tx_chunk_offset = ctx->mtu;
    }
    else {
        // Remaining buffer fits the chunk TODO pad for usb
        memcpy(&ctx->tx_chunk[tx_chunk_offset],
               &ctx->tx_apdu_buffer[ctx->tx_apdu_offset],
               ctx->tx_apdu_length - ctx->tx_apdu_offset);
        tx_chunk_offset += (ctx->tx_apdu_length - ctx->tx_apdu_offset);
        ctx->tx_apdu_offset = ctx->tx_apdu_length;
        ctx->tx_apdu_buffer = NULL;
    }
    ctx->tx_chunk_length = tx_chunk_offset;
    LOG_BLE_PROTOCOL(" %d\n", ctx->tx_chunk_length);
}
