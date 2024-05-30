
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
#include "bolos_target.h"
#include "errors.h"
#include "exceptions.h"
#ifdef HAVE_NFC

#if defined(DEBUG_OS_STACK_CONSUMPTION)
#include "os_debug.h"
#endif  // DEBUG_OS_STACK_CONSUMPTION

#include "os_io.h"
#include "os_utils.h"
#include "os_io_seproxyhal.h"
#include <string.h>

#ifdef DEBUG
#define LOG printf
#else
#define LOG(...)
#endif

#include "os.h"
#include "ledger_protocol.h"

static uint8_t           rx_apdu_buffer[IO_APDU_BUFFER_SIZE];
static ledger_protocol_t ledger_protocol_data;

void io_nfc_init(void)
{
    memset(&rx_apdu_buffer, 0, sizeof(rx_apdu_buffer));
    memset(&ledger_protocol_data, 0, sizeof(ledger_protocol_data));
    ledger_protocol_data.rx_apdu_buffer            = rx_apdu_buffer;
    ledger_protocol_data.rx_apdu_buffer_max_length = sizeof(rx_apdu_buffer);
    ledger_protocol_data.mtu
        = MIN(sizeof(ledger_protocol_data.tx_chunk), sizeof(G_io_seproxyhal_spi_buffer) - 3);
#ifdef HAVE_LOCAL_APDU_BUFFER
    ledger_protocol_data.rx_dst_buffer = NULL;
#else
    ledger_protocol_data.rx_dst_buffer = G_io_apdu_buffer;
#endif
    LEDGER_PROTOCOL_init(&ledger_protocol_data);
}

void io_nfc_recv_event(void)
{
    size_t size = U2BE(G_io_seproxyhal_spi_buffer, 1);

    LEDGER_PROTOCOL_rx(&ledger_protocol_data, G_io_seproxyhal_spi_buffer + 3, size);

    // Full apdu is received, copy it to global apdu buffer
    if (ledger_protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
        memcpy(ledger_protocol_data.rx_dst_buffer,
               ledger_protocol_data.rx_apdu_buffer,
               ledger_protocol_data.rx_apdu_length);
        G_io_app.apdu_length                = ledger_protocol_data.rx_apdu_length;
        G_io_app.apdu_state                 = APDU_NFC;
        G_io_app.apdu_media                 = IO_APDU_MEDIA_NFC;
        ledger_protocol_data.rx_apdu_length = 0;
        ledger_protocol_data.rx_apdu_status = APDU_STATUS_WAITING;
    }
}

static void nfc_send_rapdu(const uint8_t *packet, uint16_t packet_length)
{
    if ((size_t) (packet_length + 3) > sizeof(G_io_seproxyhal_spi_buffer)) {
        return;
    }

    G_io_seproxyhal_spi_buffer[0] = SEPROXYHAL_TAG_NFC_RAPDU;
    G_io_seproxyhal_spi_buffer[1] = (packet_length & 0xff00) >> 8;
    G_io_seproxyhal_spi_buffer[2] = packet_length & 0xff;
    memcpy(G_io_seproxyhal_spi_buffer + 3, packet, packet_length);
    io_seproxyhal_spi_send(G_io_seproxyhal_spi_buffer, 3 + packet_length);
}

void io_nfc_send_response(const uint8_t *packet, uint16_t packet_length)
{
    LEDGER_PROTOCOL_tx(&ledger_protocol_data, packet, packet_length);
    if (ledger_protocol_data.tx_chunk_length >= 2) {
        // send the NFC APDU chunk
        nfc_send_rapdu(ledger_protocol_data.tx_chunk, ledger_protocol_data.tx_chunk_length);
    }

    while (ledger_protocol_data.tx_apdu_buffer) {
        LEDGER_PROTOCOL_tx(&ledger_protocol_data, NULL, 0);
        if (ledger_protocol_data.tx_chunk_length >= 2) {
            // send the NFC APDU chunk
            nfc_send_rapdu(ledger_protocol_data.tx_chunk, ledger_protocol_data.tx_chunk_length);
        }
    }
}

#endif  // HAVE_NFC
