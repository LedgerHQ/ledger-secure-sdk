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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "os_math.h"
#include "decorators.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    IO_APDU_MEDIA_NONE    = 0,  // not correctly in an apdu exchange
    IO_APDU_MEDIA_USB_HID = 1,
    IO_APDU_MEDIA_BLE,
    IO_APDU_MEDIA_NFC,
    IO_APDU_MEDIA_USB_CCID,
    IO_APDU_MEDIA_USB_WEBUSB,
    IO_APDU_MEDIA_RAW,
    IO_APDU_MEDIA_U2F,
    IO_APDU_MEDIA_CDC,
    IO_APDU_MEDIA_IAP,
} io_apdu_media_t;

typedef enum {
    APDU_IDLE,
    APDU_BLE,
    APDU_BLE_WAIT_NOTIFY,
    APDU_NFC,
    APDU_NFC_M24SR,
    APDU_NFC_M24SR_SELECT,
    APDU_NFC_M24SR_FIRST,
    APDU_NFC_M24SR_RAPDU,
    APDU_USB_HID,
    APDU_USB_CCID,
    APDU_U2F,
    APDU_U2F_CBOR,
    APDU_U2F_CANCEL,
    APDU_RAW,
    APDU_USB_WEBUSB,
} io_apdu_state_e;

#ifdef HAVE_NFC_READER
enum card_tech {
    NFC_A,
    NFC_B
};

enum nfc_event {
    CARD_DETECTED,
    CARD_REMOVED,
};
#endif  // HAVE_NFC_READER

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    io_apdu_state_e apdu_state;   // by default
    unsigned short  apdu_length;  // total length to be received
    unsigned short  io_flags;     // flags to be set when calling io_exchange
    io_apdu_media_t apdu_media;
#ifdef HAVE_BLE
    unsigned int plane_mode;
#endif  // HAVE_BLE
} io_seph_app_t;

#ifdef HAVE_NFC_READER
struct card_info {
    enum card_tech tech;
    uint8_t        nfcid[7];
    size_t         nfcid_len;
};

typedef void (*nfc_evt_callback_t)(enum nfc_event event, struct card_info *info);
typedef void (*nfc_resp_callback_t)(bool error, bool timeout, uint8_t *resp_data, size_t resp_len);

struct nfc_reader_context {
    nfc_resp_callback_t resp_callback;
    nfc_evt_callback_t  evt_callback;
    bool                reader_mode;
    bool                event_happened;
    bool                response_received;
    unsigned int        remaining_ms;
    enum nfc_event      last_event;
    struct card_info    card;
    uint8_t            *apdu_rx;
    size_t              apdu_rx_len;       // Used length
    size_t              apdu_rx_max_size;  // Max size of buffer
};
#endif  // HAVE_NFC_READER

/* Exported defines   --------------------------------------------------------*/
#ifdef IO_SEPROXYHAL_BUFFER_SIZE_B
#undef IO_SEPROXYHAL_BUFFER_SIZE_B
#endif  // IO_SEPROXYHAL_BUFFER_SIZE_B
#define IO_SEPROXYHAL_BUFFER_SIZE_B OS_IO_SEPH_BUFFER_SIZE

/* Exported macros------------------------------------------------------------*/
#define SWAP(a, b) \
    {              \
        a ^= b;    \
        b ^= a;    \
        a ^= b;    \
    }

/* Exported variables --------------------------------------------------------*/
extern io_seph_app_t G_io_app;

/* Exported functions prototypes--------------------------------------------- */
