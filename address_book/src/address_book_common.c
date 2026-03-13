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

#include "address_book_common.h"
#include "lcx_sha256.h"
#include "lcx_ecdsa.h"
#include "ledger_pki.h"
#include "os_pki.h"
#include "os_utils.h"
#include "os_print.h"
#include "buffer.h"
#include "tlv_library.h"
#include "bip32.h"
#include "address_book.h"

#ifdef HAVE_ADDRESS_BOOK

/* Private constants ---------------------------------------------------------*/
// DER signature tag value (0x15) - defined here to avoid conflicts with TLV macros
#define DER_SIGNATURE_TAG_VALUE 0x15

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
bool address_book_verify_signature(cx_sha256_t *hash_ctx, const uint8_t *sig, uint8_t sig_size)
{
    uint8_t          hash[CX_SHA256_SIZE] = {0};
    const buffer_t   buffer               = {.ptr = hash, .size = sizeof(hash)};
    const buffer_t   signature            = {.ptr = (uint8_t *) sig, .size = sig_size};
    const cx_curve_t curve                = CX_CURVE_SECP256R1;
    const uint8_t    key_usage            = CERTIFICATE_PUBLIC_KEY_USAGE_ADDRESS_BOOK;

    if (cx_hash_no_throw((cx_hash_t *) hash_ctx, CX_LAST, NULL, 0, hash, sizeof(hash)) != CX_OK) {
        PRINTF("Could not finalize struct hash!\n");
        return false;
    }

    if (check_signature_with_pki(buffer, &key_usage, &curve, signature)
        != CHECK_SIGNATURE_WITH_PKI_SUCCESS) {
        return false;
    }

    return true;
}

/**
 * @brief Common TLV handler for hashing (excludes signature tag)
 *
 * Generic handler that hashes all TLV tags except the signature tag.
 * To be used with DEFINE_TLV_PARSER macro.
 *
 * @param[in] data TLV data
 * @param[in] hash_ctx_ptr Hash context (passed as void*, should be cx_sha256_t*)
 * @return true on success
 */
bool address_book_hash_handler(const tlv_data_t *data, void *hash_ctx_ptr)
{
    // Hash all tags except the signature tag (0x15 = TAG_DER_SIGNATURE)
    if (data->tag != DER_SIGNATURE_TAG_VALUE) {
        CX_ASSERT(cx_hash_no_throw(
            (cx_hash_t *) hash_ctx_ptr, 0, data->raw.ptr, data->raw.size, NULL, 0));
    }
    return true;
}

/**
 * @brief Generic handler for BIP32 derivation path
 *
 * @param[in] data TLV data
 * @param[out] bip32_path Pointer to BIP32 path structure to fill
 * @return true if successful
 */
bool address_book_handle_derivation_path(const tlv_data_t *data, path_bip32_t *bip32_path)
{
    buffer_t path_buffer = {0};
    uint8_t  data_len    = 0;

    // Get the raw buffer
    if (!get_buffer_from_tlv_data(data, &path_buffer, 1, MAX_BIP32_PATH * 4 + 1)) {
        PRINTF("DERIVATION_PATH: failed to extract\n");
        return false;
    }

    // Parse BIP32 path using the standard parser
    data_len = (uint8_t) path_buffer.size;
    if (bip32_path_parse(path_buffer.ptr, &data_len, bip32_path) == NULL) {
        PRINTF("DERIVATION_PATH: failed to parse BIP32 path\n");
        return false;
    }

    return true;
}

/**
 * @brief Generic handler for chain ID
 *
 * @param[in] data TLV data
 * @param[out] chain_id Pointer to chain ID variable to fill
 * @return true if successful
 */
bool address_book_handle_chain_id(const tlv_data_t *data, uint64_t *chain_id)
{
    if (!get_uint64_t_from_tlv_data(data, chain_id)) {
        PRINTF("CHAIN_ID: failed to extract\n");
        return false;
    }
    return true;
}

/**
 * @brief Generic handler for blockchain family
 *
 * @param[in] data TLV data
 * @param[out] family Pointer to blockchain family enum to fill
 * @return true if successful
 */
bool address_book_handle_blockchain_family(const tlv_data_t *data, blockchain_family_e *family)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("UINT8: failed to extract\n");
        return false;
    }
    if (value >= FAMILY_MAX) {
        PRINTF("BLOCKCHAIN_FAMILY: Value out of range (%d)!\n", FAMILY_MAX);
        return false;
    }
    *family = (blockchain_family_e) value;
    return true;
}

/**
 * @brief Generic handler for DER signature
 *
 * @param[in] data TLV data
 * @param[out] sig Pointer to signature pointer (will point to data in buffer)
 * @param[out] sig_size Pointer to signature size variable
 * @return true if successful
 */
bool address_book_handle_signature(const tlv_data_t *data, const uint8_t **sig, uint8_t *sig_size)
{
    buffer_t sig_buffer = {0};
    if (!get_buffer_from_tlv_data(data,
                                  &sig_buffer,
                                  CX_ECDSA_SHA256_SIG_MIN_ASN1_LENGTH,
                                  CX_ECDSA_SHA256_SIG_MAX_ASN1_LENGTH)) {
        PRINTF("DER_SIGNATURE: failed to extract\n");
        return false;
    }
    *sig_size = sig_buffer.size;
    *sig      = sig_buffer.ptr;
    return true;
}

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
                                          size_t            buffer_size)
{
    // Extract the string (with null terminator added by get_string_from_tlv_data)
    if (!get_string_from_tlv_data(data, output_buffer, 1, buffer_size - 1)) {
        PRINTF("String extraction failed\n");
        return false;
    }
    if (!is_printable_string(output_buffer, strlen(output_buffer))) {
        PRINTF("String contains non-printable characters\n");
        return false;
    }
    return true;
}

#endif  // HAVE_ADDRESS_BOOK
