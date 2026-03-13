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

/**
 * @file identity.h
 * @brief Register / Rename Identity
 *
 * An Identity is a contact identified by a name, an IDENTIFIER (blockchain
 * address or public key, up to 80 bytes), an optional CONTACT_SCOPE (context
 * string, e.g. "Ethereum"), and a derivation path used to derive the HMAC key
 * for the Proof of Registration.
 *
 * Identities are blockchain-agnostic: the binding is between a name and an
 * identifier, not between a name and a specific blockchain.
 *
 * Active under HAVE_ADDRESS_BOOK (no sub-flag required).
 */

#if defined(HAVE_ADDRESS_BOOK)

/* Includes ------------------------------------------------------------------*/
#include "os_types.h"
#include "address_book.h"
#include "bip32.h"

/* Exported defines   --------------------------------------------------------*/
#define TYPE_REGISTER_IDENTITY 0x11
#define TYPE_RENAME_IDENTITY   0x12

#define CONTACT_NAME_LENGTH   33  ///< max 32 printable chars + null terminator
#define CONTACT_SCOPE_LENGTH  33  ///< max 32 printable chars + null terminator
#define IDENTIFIER_MAX_LENGTH 80  ///< blockchain address or public key, raw bytes

/* Exported types, structures, unions ----------------------------------------*/

/**
 * @brief Data extracted from a Register Identity TLV payload.
 */
typedef struct {
    char                contact_name[CONTACT_NAME_LENGTH];
    char                contact_scope[CONTACT_SCOPE_LENGTH];
    uint8_t             identifier[IDENTIFIER_MAX_LENGTH];
    uint8_t             identifier_len;
    path_bip32_t        bip32_path;  ///< Used to derive the HMAC key for the Proof of Registration
    blockchain_family_e blockchain_family;
    uint64_t            chain_id;  ///< Mandatory when blockchain_family == FAMILY_ETHEREUM
} identity_t;

/* Exported functions prototypes --------------------------------------------- */
bolos_err_t register_identity(uint8_t *buffer_in, size_t buffer_in_length);

#endif  // HAVE_ADDRESS_BOOK
