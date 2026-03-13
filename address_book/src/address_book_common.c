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

/**
 * @file address_book_common.c
 * @brief Shared helpers for all Address Book sub-commands
 *
 * Provides reusable TLV field handlers used across every TLV-based flow:
 *
 *  - address_book_handle_derivation_path(): parses a BIP32 derivation path
 *  - address_book_handle_chain_id(): parses a uint64 chain ID
 *  - address_book_handle_blockchain_family(): parses a blockchain family enum
 *  - address_book_handle_printable_string(): extracts and validates a printable string
 */

#include <string.h>
#include "address_book_common.h"
#include "os_utils.h"
#include "os_print.h"
#include "buffer.h"
#include "tlv_library.h"
#include "bip32.h"
#include "address_book.h"

#ifdef HAVE_ADDRESS_BOOK

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
