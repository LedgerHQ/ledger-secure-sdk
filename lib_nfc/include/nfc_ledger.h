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

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    NFC_LEDGER_MODE_CARD_EMULATION = 0x00,
    NFC_LEDGER_MODE_READER         = 0x01,
} nfc_ledger_mode_e;

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

void NFC_LEDGER_init(uint8_t force_restart);
void NFC_LEDGER_start(uint8_t mode);  // nfc_ledger_mode_e
void NFC_LEDGER_stop(void);

// Rx
int NFC_LEDGER_rx_seph_apdu_evt(uint8_t *seph_buffer,
                                uint16_t seph_buffer_length,
                                uint8_t *apdu_buffer,
                                uint16_t apdu_buffer_max_length);

// Tx
uint32_t NFC_LEDGER_send(const uint8_t *packet, uint16_t packet_length, uint32_t timeout_ms);
