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
 * @file contact.h
 * @brief Contact registration (V2 - "Contacts" proposal)
 *
 * A Contact is a person or entity identified by a name and an identity public
 * key (a compressed secp256r1 key, distinct from any blockchain payment key).
 * Contacts are blockchain-agnostic: the binding is between a name and a key,
 * not between a name and an address.
 *
 * After user confirmation, the device returns an HMAC-based Proof of
 * Registration that the host stores and presents back for future
 * address verification (@see verify_address.h).
 *
 * Requires HAVE_ADDRESS_BOOK and HAVE_ADDRESS_BOOK_CONTACTS to be defined.
 */

#if !defined(HAVE_ADDRESS_BOOK)
#error "HAVE_ADDRESS_BOOK_CONTACTS requires HAVE_ADDRESS_BOOK"
#endif

#ifdef HAVE_ADDRESS_BOOK_CONTACTS

/* Includes ------------------------------------------------------------------*/
#include "os_types.h"
#include "address_book.h"
#include "bip32.h"

/* Exported defines   --------------------------------------------------------*/
#define TYPE_REGISTER_CONTACT 0x14
#define CONTACT_NAME_LENGTH   33  // max 32 printable chars + null terminator

/**
 * Length of a compressed secp256r1 public key (identity key).
 * The identity key must never be used directly as a blockchain payment key.
 */
#define IDENTITY_PUBKEY_LENGTH 33

/* Exported types, structures, unions ----------------------------------------*/

/**
 * @brief Data extracted from a Register Contact TLV payload.
 */
typedef struct {
    char         contact_name[CONTACT_NAME_LENGTH];
    uint8_t      identity_pubkey[IDENTITY_PUBKEY_LENGTH];
    path_bip32_t bip32_path;  ///< Used to derive the HMAC key for the Proof of Registration
} contact_t;

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t register_contact(uint8_t *buffer_in, size_t buffer_in_length);

#endif  // HAVE_ADDRESS_BOOK_CONTACTS
