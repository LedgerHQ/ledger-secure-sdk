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
 * @file verify_address.h
 * @brief Verify a signed payment address against a registered Contact (V2)
 *
 * At payment time, the receiver produces a signed address: the address is
 * signed with their identity private key.  The host presents this signed
 * address to the device along with the contact's identity public key and the
 * HMAC Proof of Registration obtained during contact registration.
 *
 * The device:
 *   1. Re-derives the HMAC and verifies the Proof of Registration (proves
 *      the contact was registered on this device).
 *   2. Verifies the ECDSA signature over the address using the identity key
 *      (proves the intended receiver produced the address).
 *   3. If both checks pass, displays the contact name instead of the raw
 *      address so the user can confirm with high confidence.
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
#include "contact.h"
#include "bip32.h"

/* Exported defines   --------------------------------------------------------*/
#define TYPE_VERIFY_ADDRESS     0x16
#define MAX_ADDRESS_LENGTH      64  // re-use from external_address.h if included
#define HMAC_PROOF_LENGTH       32  // HMAC-SHA256 output
#define MAX_IDENTITY_SIG_LENGTH 72  // max DER-encoded ECDSA signature

/* Exported types, structures, unions ----------------------------------------*/

/**
 * @brief Data extracted from a Verify Signed Address TLV payload.
 */
typedef struct {
    char         contact_name[CONTACT_NAME_LENGTH];        ///< Name to display to the user
    uint8_t      identity_pubkey[IDENTITY_PUBKEY_LENGTH];  ///< Contact's compressed secp256r1 key
    uint8_t      hmac_proof[HMAC_PROOF_LENGTH];         ///< Proof of Registration from registration
    uint8_t      address_raw[MAX_ADDRESS_LENGTH];       ///< Payment address to verify
    uint8_t      address_length;                        ///< Actual length of address_raw
    uint8_t      address_sig[MAX_IDENTITY_SIG_LENGTH];  ///< ECDSA sig over address by identity key
    uint8_t      address_sig_len;
    path_bip32_t bip32_path;                ///< Same path used at registration
    blockchain_family_e blockchain_family;  ///< For coin-app address validation
    uint64_t            chain_id;           ///< Mandatory for FAMILY_ETHEREUM
} verify_address_t;

/* Exported functions prototypes--------------------------------------------- */
bolos_err_t verify_signed_address(uint8_t *buffer_in, size_t buffer_in_length);

#endif  // HAVE_ADDRESS_BOOK_CONTACTS
