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
    FAMILY_COUNT
} blockchain_family_e;

#define FAMILY_AS_STR(x)                 \
    (x == FAMILY_BITCOIN    ? "Bitcoin"  \
     : x == FAMILY_ETHEREUM ? "Ethereum" \
     : x == FAMILY_SOLANA   ? "Solana"   \
     : x == FAMILY_POLKADOT ? "Polkadot" \
     : x == FAMILY_COSMOS   ? "Cosmos"   \
     : x == FAMILY_CARDANO  ? "Cardano"  \
                            : "Unknown")

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/**
 * Minimum IO APDU buffer size when HAVE_ADDRESS_BOOK is defined.
 *
 * Worst case: CMD_EDIT_IDENTIFIER (P1=0x03), Ethereum, max path depth, max identifiers:
 *   7  (extended header: CLA INS P1 P2 0x00 Lc_H Lc_L)
 *   +  3 (struct_type)   +  3 (struct_version)
 *   + 66 (group_handle, 64 B)
 *   + 34 (contact_name, max 32 B)
 *   + 34 (scope, max 32 B)
 *   + 82 (new identifier, max 80 B)
 *   + 82 (previous identifier, max 80 B)
 *   + 34 (hmac_name, 32 B)
 *   + 34 (hmac_rest, 32 B)
 *   + 43 (derivation_path: depth(1) + 10×4 B)
 *   + 10 (chain_id, 8 B)
 *   +  3 (blockchain_family, 1 B)
 *   = 435 bytes; + 3 SEPH overhead = 438 B.
 *   A buffer of 448 B provides a 10-byte safety margin.
 *
 * Note: Chunk mechanism allows splitting across several APDUs
 */
#ifdef CUSTOM_IO_APDU_BUFFER_SIZE
_Static_assert(CUSTOM_IO_APDU_BUFFER_SIZE >= OS_IO_SEPH_BUFFER_SIZE,
               "CUSTOM_IO_APDU_BUFFER_SIZE is too small for HAVE_ADDRESS_BOOK: "
               "CMD_EDIT_IDENTIFIER requires up to 448 bytes (SEPH buffer).");
#endif  // CUSTOM_IO_APDU_BUFFER_SIZE

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t addr_book_handle_apdu(uint8_t *buffer, size_t buffer_len, uint8_t p1, uint8_t p2);
