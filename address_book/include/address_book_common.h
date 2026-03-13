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

#ifndef ADDRESS_BOOK_COMMON_H
#define ADDRESS_BOOK_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "lcx_sha256.h"
#include "tlv_library.h"
#include "bip32.h"
#include "address_book.h"

/**
 * @brief Verify PKI signature on address book payload
 *
 * Generic function to verify the SHA-256 hash against the signature using PKI.
 *
 * @param[in] hash_ctx Hash context containing the payload hash
 * @param[in] sig Signature buffer
 * @param[in] sig_size Signature size
 * @return true if signature is valid, false otherwise
 */
bool address_book_verify_signature(cx_sha256_t *hash_ctx, const uint8_t *sig, uint8_t sig_size);

/**
 * @brief Common TLV handler for hashing (excludes signature tag)
 *
 * Generic handler that hashes all TLV tags except the signature tag.
 * To be used with DEFINE_TLV_PARSER macro.
 *
 * @param[in] data TLV data
 * @param[in] hash_ctx Hash context (passed as void*)
 * @return true on success
 */
bool address_book_hash_handler(const tlv_data_t *data, void *hash_ctx);

/**
 * @brief Generic handler for BIP32 derivation path
 *
 * @param[in] data TLV data
 * @param[out] bip32_path Pointer to BIP32 path structure to fill
 * @return true if successful
 */
bool address_book_handle_derivation_path(const tlv_data_t *data, path_bip32_t *bip32_path);

/**
 * @brief Generic handler for chain ID
 *
 * @param[in] data TLV data
 * @param[out] chain_id Pointer to chain ID variable to fill
 * @return true if successful
 */
bool address_book_handle_chain_id(const tlv_data_t *data, uint64_t *chain_id);

/**
 * @brief Generic handler for blockchain family
 *
 * @param[in] data TLV data
 * @param[out] family Pointer to blockchain family enum to fill
 * @return true if successful
 */
bool address_book_handle_blockchain_family(const tlv_data_t *data, blockchain_family_e *family);

/**
 * @brief Generic handler for DER signature
 *
 * @param[in] data TLV data
 * @param[out] sig Pointer to signature pointer (will point to data in buffer)
 * @param[out] sig_size Pointer to signature size variable
 * @return true if successful
 */
bool address_book_handle_signature(const tlv_data_t *data, const uint8_t **sig, uint8_t *sig_size);

/**
 * @brief Generic handler for printable string extraction
 *
 * Extracts a string from TLV data and verifies it contains only printable characters.
 *
 * @param[in] data TLV data
 * @param[out] output_buffer Buffer to store the extracted string
 * @param[in] buffer_size Maximum size of the output buffer (including null terminator)
 * @return true if successful
 */
bool address_book_handle_printable_string(const tlv_data_t *data,
                                          char             *output_buffer,
                                          size_t            buffer_size);

#endif  // ADDRESS_BOOK_COMMON_H
