/*****************************************************************************
 *   (c) 2026 Ledger SAS.
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
#include "os_types.h"
#include "address_book.h"
#include "bip32.h"

/* Exported defines   --------------------------------------------------------*/
#define ACCOUNT_NAME_LENGTH 33

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    DERIVATION_SCHEME_BIP44              = 0x01,  // BIP44 Standard
    DERIVATION_SCHEME_BIP84              = 0x02,  // BIP84 (Native SegWit)
    DERIVATION_SCHEME_BIP86              = 0x03,  // BIP86 (Taproot)
    DERIVATION_SCHEME_GENERIC_DESCRIPTOR = 0x04   // Generic descriptor
} derivation_scheme_e;

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
    const char          account_name[ACCOUNT_NAME_LENGTH];
    uint32_t            derivation_path[MAX_BIP32_PATH];
    uint8_t             derivation_path_length;
    derivation_scheme_e derivation_scheme;
    uint64_t            chain_id;
} ledger_account_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t add_ledger_account(uint8_t *buffer_in, size_t buffer_in_length);
