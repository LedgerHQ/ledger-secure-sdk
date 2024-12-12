
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

#include "os.h"
#include "os_settings.h"
#include "os_io_seproxyhal.h"

#include "errors.h"
#include "exceptions.h"
#ifdef HAVE_NFC

#if defined(DEBUG_OS_STACK_CONSUMPTION)
#include "os_debug.h"
#endif  // DEBUG_OS_STACK_CONSUMPTION

#include "os_io.h"
#include "os_io_nfc.h"
#include "os_utils.h"
#include "os_io_seproxyhal.h"
#include <string.h>

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
#ifdef HAVE_NFC_READER
    memset((void *) &G_io_reader_ctx, 0, sizeof(G_io_reader_ctx));
    G_io_reader_ctx.apdu_rx          = rx_apdu_buffer;
    G_io_reader_ctx.apdu_rx_max_size = sizeof(rx_apdu_buffer);
#endif  // HAVE_NFC_READER
}

void io_nfc_recv_event(void)
{
    size_t size = U2BE(G_io_seproxyhal_spi_buffer, 1);

    LEDGER_PROTOCOL_rx(&ledger_protocol_data, G_io_seproxyhal_spi_buffer + 3, size);

    // Full apdu is received, copy it to global apdu buffer
    if (ledger_protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
#ifdef HAVE_NFC_READER
        if (G_io_reader_ctx.reader_mode) {
            G_io_reader_ctx.response_received = true;
            G_io_reader_ctx.apdu_rx_len       = ledger_protocol_data.rx_apdu_length;
            return;
        }
#endif  // HAVE_NFC_READER

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

#ifdef HAVE_NFC_READER

void io_nfc_event(void)
{
    size_t size = U2BE(G_io_seproxyhal_spi_buffer, 1);

    if (size >= 1) {
        switch (G_io_seproxyhal_spi_buffer[3]) {
            case SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED: {
                G_io_reader_ctx.event_happened = true;
                G_io_reader_ctx.last_event     = CARD_DETECTED;
                G_io_reader_ctx.card.tech
                    = (G_io_seproxyhal_spi_buffer[4] == SEPROXYHAL_TAG_NFC_EVENT_CARD_DETECTED_A)
                          ? NFC_A
                          : NFC_B;
                G_io_reader_ctx.card.nfcid_len = MIN(size - 2, sizeof(G_io_reader_ctx.card.nfcid));
                memcpy((void *) G_io_reader_ctx.card.nfcid,
                       G_io_seproxyhal_spi_buffer + 5,
                       G_io_reader_ctx.card.nfcid_len);
            } break;

            case SEPROXYHAL_TAG_NFC_EVENT_CARD_LOST:
                if (G_io_reader_ctx.evt_callback != NULL) {
                    G_io_reader_ctx.event_happened = true;
                    G_io_reader_ctx.last_event     = CARD_REMOVED;
                }
                break;
        }
    }
}

void io_nfc_process_events(void)
{
    if (G_io_reader_ctx.response_received) {
        G_io_reader_ctx.response_received = false;
        if (G_io_reader_ctx.resp_callback != NULL) {
            nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
            G_io_reader_ctx.resp_callback = NULL;
            resp_cb(false, false, G_io_reader_ctx.apdu_rx, G_io_reader_ctx.apdu_rx_len);
        }
    }

    if (G_io_reader_ctx.resp_callback != NULL && G_io_reader_ctx.remaining_ms == 0) {
        nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
        G_io_reader_ctx.resp_callback = NULL;
        resp_cb(false, true, NULL, 0);
    }

    if (G_io_reader_ctx.event_happened) {
        G_io_reader_ctx.event_happened = 0;

        // If card is removed during an APDU processing, call the resp_callback with an error
        if (G_io_reader_ctx.resp_callback != NULL && G_io_reader_ctx.last_event == CARD_REMOVED) {
            nfc_resp_callback_t resp_cb   = G_io_reader_ctx.resp_callback;
            G_io_reader_ctx.resp_callback = NULL;
            resp_cb(true, false, NULL, 0);
        }

        if (G_io_reader_ctx.evt_callback != NULL) {
            G_io_reader_ctx.evt_callback(G_io_reader_ctx.last_event,
                                         (struct card_info *) &G_io_reader_ctx.card);
        }
        if (G_io_reader_ctx.last_event == CARD_REMOVED) {
            memset((void *) &G_io_reader_ctx.card, 0, sizeof(G_io_reader_ctx.card));
        }
    }
}

void io_nfc_ticker(void)
{
    if (G_io_reader_ctx.resp_callback != NULL) {
        if (G_io_reader_ctx.remaining_ms <= 100) {
            G_io_reader_ctx.remaining_ms = 0;
        }
        else {
            G_io_reader_ctx.remaining_ms -= 100;
        }
    }
}

bool io_nfc_reader_send(const uint8_t      *cmd_data,
                        size_t              cmd_len,
                        nfc_resp_callback_t callback,
                        int                 timeout_ms)
{
    G_io_reader_ctx.resp_callback = PIC(callback);
    io_nfc_send_response(PIC(cmd_data), cmd_len);

    G_io_reader_ctx.response_received = false;
    G_io_reader_ctx.remaining_ms      = timeout_ms;

    return true;
}

void io_nfc_reader_power(void)
{
    uint8_t buffer[4];
    buffer[0] = SEPROXYHAL_TAG_NFC_POWER;
    buffer[1] = 0;
    buffer[2] = 1;
    buffer[3] = SEPROXYHAL_TAG_NFC_POWER_ON_READER;
    io_seproxyhal_spi_send(buffer, 4);
}

bool io_nfc_reader_start(nfc_evt_callback_t callback)
{
    G_io_reader_ctx.evt_callback      = PIC(callback);
    G_io_reader_ctx.reader_mode       = true;
    G_io_reader_ctx.event_happened    = false;
    G_io_reader_ctx.resp_callback     = NULL;
    G_io_reader_ctx.response_received = false;
    io_nfc_reader_power();
    return true;
}

void io_nfc_reader_stop()
{
    G_io_reader_ctx.evt_callback      = NULL;
    G_io_reader_ctx.reader_mode       = false;
    G_io_reader_ctx.event_happened    = false;
    G_io_reader_ctx.resp_callback     = NULL;
    G_io_reader_ctx.response_received = false;
    io_seproxyhal_nfc_power(false);
}

bool io_nfc_is_reader(void)
{
    return G_io_reader_ctx.reader_mode;
}

#endif  // HAVE_NFC_READER

#endif  // HAVE_NFC
