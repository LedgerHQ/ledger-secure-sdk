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

#ifdef HAVE_NFC_READER

enum card_tech {
    NFC_A,
    NFC_B
};

enum nfc_event {
    CARD_DETECTED,
    CARD_REMOVED,
};

struct card_info {
    enum card_tech tech;
    uint8_t        nfcid[7];
    size_t         nfcid_len;
};

typedef void (*nfc_evt_callback_t)(enum nfc_event event, struct card_info *info);
typedef void (*nfc_resp_callback_t)(bool error, bool timeout, uint8_t *resp_data, size_t resp_len);

bool os_io_nfc_reader_send(const uint8_t      *cmd_data,
                           size_t              cmd_len,
                           nfc_resp_callback_t callback,
                           int                 timeout_ms);

/* Return false if nfc reader can not be started in current conditions */
bool os_io_nfc_reader_start(nfc_evt_callback_t callback);
void os_io_nfc_reader_stop(void);
#endif  // HAVE_NFC_READER
