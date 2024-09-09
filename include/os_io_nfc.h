
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

#ifndef OS_IO_NFC_H
#define OS_IO_NFC_H

#include "arch.h"

#include "os_io_seproxyhal.h"

/* Public API for reader application --------------------------------------- */
#ifdef HAVE_NFC_READER

enum card_tech {
    NFC_A,
    NFC_B
};

struct card_info {
    enum card_tech tech;
    uint8_t        nfcid[7];
    size_t         nfcid_len;
};

enum nfc_event {
    CARD_DETECTED,
    CARD_REMOVED,
};

typedef void (*nfc_evt_callback_t)(enum nfc_event event, struct card_info *info);
typedef void (*nfc_resp_callback_t)(bool error, bool timeout, uint8_t *resp_data, size_t resp_len);

/* Functions */

/* return false in case of error
   in that case, callback will not be called */
bool io_nfc_reader_send(const uint8_t      *cmd_data,
                        size_t              cmd_len,
                        nfc_resp_callback_t callback,
                        int                 timeout_ms);

/* Return false if nfc reader can not be started in current conditions */
bool io_nfc_reader_start(nfc_evt_callback_t callback);
void io_nfc_reader_stop(void);
bool io_nfc_is_reader(void);

#endif  // HAVE_NFC_READER

/* SDK internal API  ---------------------------------------  */

#ifdef HAVE_NFC_READER

struct nfc_reader_context {
    nfc_resp_callback_t resp_callback;
    nfc_evt_callback_t  evt_callback;
    bool                reader_mode;
    bool                event_happened;
    bool                response_received;
    unsigned int        remaining_ms;
    enum nfc_event      last_event;
    struct card_info    card;
};

extern struct nfc_reader_context G_io_reader_ctx;
#endif  // HAVE_NFC_READER

void io_nfc_init(void);
void io_nfc_recv_event(void);
void io_nfc_send_response(const uint8_t *packet, uint16_t packet_length);

#ifdef HAVE_NFC_READER
void io_nfc_event(void);
void io_nfc_ticker(void);
void io_nfc_process_events(void);
#endif  // HAVE_NFC_READER

#endif
