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
#include <stddef.h>
#include "os_types.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
    FAMILY_BITCOIN  = 0x00,
    FAMILY_ETHEREUM = 0x01,
    FAMILY_SOLANA   = 0x02,
    FAMILY_POLKADOT = 0x03,
    FAMILY_COSMOS   = 0x04,
    FAMILY_CARDANO  = 0x05,
    FAMILY_MAX
} blockchain_family_e;

#define FAMILY_STR(x)                    \
    (x == FAMILY_BITCOIN    ? "Bitcoin"  \
     : x == FAMILY_ETHEREUM ? "Ethereum" \
     : x == FAMILY_SOLANA   ? "Solana"   \
     : x == FAMILY_POLKADOT ? "Polkadot" \
     : x == FAMILY_COSMOS   ? "Cosmos"   \
     : x == FAMILY_CARDANO  ? "Cardano"  \
                            : "Unknown")

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1);
