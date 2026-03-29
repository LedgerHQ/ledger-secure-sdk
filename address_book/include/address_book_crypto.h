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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "buffer.h"
#include "lcx_ecfp.h"
#include "ox_aes.h"
#include "bip32.h"

#ifdef HAVE_ADDRESS_BOOK

/* Constants -----------------------------------------------------------------*/

// Maximum remaining payload size calculation (type is now sent in cleartext)
// External: len(1) + name(33) + len(1) + addr(64) + chain(8) + family(1) = 108
// Ledger:   len(1) + path(40) + scheme(1) + chain(8) + family(1) = 51
#define MAX_EXTERNAL_REMAINING (1 + EXTERNAL_NAME_LENGTH + 1 + MAX_ADDRESS_LENGTH + 8 + 1)
#define MAX_LEDGER_REMAINING   (1 + (4 * MAX_BIP32_PATH) + 1 + 8 + 1)

/* Functions -----------------------------------------------------------------*/

bool build_name_payload(const char *name, uint8_t *buffer, size_t buffer_size, size_t *payload_len);

bool address_book_encrypt_and_send(uint8_t             type,
                                   const path_bip32_t *bip32_path,
                                   uint8_t            *name_payload,
                                   size_t              name_payload_len,
                                   uint8_t            *remaining_payload,
                                   size_t              remaining_payload_len);

bool address_book_decrypt(const buffer_t     *buffer,
                          const path_bip32_t *bip32_path,
                          char               *output_name);

#ifdef HAVE_ADDRESS_BOOK_CONTACTS

/**
 * @brief Compute an HMAC-SHA256 Proof of Registration for a Contact.
 *
 * The HMAC key is derived as SHA256("AddressBook-HMAC-Key" || privkey.d)
 * where privkey is the secp256r1 key at bip32_path.
 *
 * The message is: name_len(1) | name | identity_pubkey(33)
 *
 * @param[in]  bip32_path      BIP32 path used at registration
 * @param[in]  name            Contact name (null-terminated)
 * @param[in]  identity_pubkey Compressed secp256r1 identity public key (33 bytes)
 * @param[out] hmac_out        Output buffer for the 32-byte HMAC proof
 * @return true if successful, false otherwise
 */
bool address_book_compute_hmac_proof(const path_bip32_t *bip32_path,
                                     const char         *name,
                                     const uint8_t       identity_pubkey[33],
                                     uint8_t             hmac_out[32]);

/**
 * @brief Verify an HMAC Proof of Registration for a Contact.
 *
 * Re-derives and compares the HMAC. Constant-time comparison is used.
 *
 * @param[in] bip32_path      BIP32 path used at registration
 * @param[in] name            Contact name (null-terminated)
 * @param[in] identity_pubkey Compressed secp256r1 identity public key (33 bytes)
 * @param[in] hmac_expected   32-byte HMAC proof to verify against
 * @return true if the proof is valid, false otherwise
 */
bool address_book_verify_hmac_proof(const path_bip32_t *bip32_path,
                                    const char         *name,
                                    const uint8_t       identity_pubkey[33],
                                    const uint8_t       hmac_expected[32]);

/**
 * @brief Send an HMAC Proof of Registration response to the host.
 *
 * Response format: type(1) | hmac(32)
 *
 * @param[in] type      Message type (TYPE_REGISTER_CONTACT)
 * @param[in] hmac_proof 32-byte HMAC proof
 * @return true if sent successfully, false otherwise
 */
bool address_book_send_hmac_proof(uint8_t type, const uint8_t hmac_proof[32]);

#endif  // HAVE_ADDRESS_BOOK_CONTACTS

#endif  // HAVE_ADDRESS_BOOK
