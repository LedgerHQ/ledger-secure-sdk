/*****************************************************************************
 *   (c) 2025 Ledger SAS.
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

#ifdef HAVE_TRANSACTION_CHECKS

#include "tlv_library.h"
#include "buffer.h"

/* TLV use-case API on top of the TLV library
 *
 * This SDK helper implements the Web3 Check (W3C) transaction simulation descriptor parsing.
 *
 * Please refer to the tlv_library.h file for documentation on how to write your own use-case if it
 * does not follow the above specification.
 *
 * This descriptor associates a transaction or typed-data hash with a risk assessment from a
 * security provider. The trusted information comes from the Ledger backend and is forwarded by
 * Ledger Live.
 */

// Maximum sizes from the spec
#define W3C_CHECK_HASH_SIZE        32
#define W3C_CHECK_MSG_SIZE         25
#define W3C_CHECK_URL_SIZE         30
#define W3C_CHECK_PARTNER_SIZE     20
// EVM address is 20 bytes, Solana base58 address is up to 44 bytes
#define W3C_CHECK_MAX_ADDRESS_SIZE 44

// clang-format off
typedef enum w3c_check_risk_e {
    W3C_CHECK_RISK_BENIGN    = 0x00,
    W3C_CHECK_RISK_WARNING   = 0x01,
    W3C_CHECK_RISK_MALICIOUS = 0x02,
    // Internal level for error handling, not an API entry point
    W3C_CHECK_RISK_UNKNOWN,
} w3c_check_risk_t;
// clang-format on

typedef enum w3c_check_type_e {
    W3C_CHECK_TYPE_TRANSACTION = 0x00,
    W3C_CHECK_TYPE_TYPED_DATA  = 0x01,
    W3C_CHECK_TYPE_COUNT,
} w3c_check_type_t;

typedef enum w3c_check_category_e {
    W3C_CHECK_CATEGORY_NA               = 0x00,
    W3C_CHECK_CATEGORY_OTHERS           = 0x01,
    W3C_CHECK_CATEGORY_ADDRESS          = 0x02,
    W3C_CHECK_CATEGORY_DAPP             = 0x03,
    W3C_CHECK_CATEGORY_LOSING_OPERATION = 0x04,
    W3C_CHECK_CATEGORY_COUNT,
} w3c_check_category_t;

// Output of the use-case TLV parser
typedef struct tlv_w3c_check_out_s {
    // Chain ID (required for transactions, optional for typed data)
    uint64_t chain_id;
    // Transaction or message hash (32 bytes)
    uint8_t tx_hash[W3C_CHECK_HASH_SIZE];
    // Domain hash for EIP-712 typed data (32 bytes)
    uint8_t domain_hash[W3C_CHECK_HASH_SIZE];
    // Provider message (UTF-8, null-terminated)
    char provider_msg[W3C_CHECK_MSG_SIZE + 1];
    // Report URL (null-terminated)
    char tiny_url[W3C_CHECK_URL_SIZE + 1];
    // Sender address (variable size: 20 for EVM, 43-44 for Solana)
    uint8_t address[W3C_CHECK_MAX_ADDRESS_SIZE];
    uint8_t address_len;
    // Risk level
    w3c_check_risk_t risk;
    // Simulation type
    w3c_check_type_t type;
    // Risk category
    w3c_check_category_t category;

    // Flag optional tags reception
    bool chain_id_received;
    bool domain_hash_received;
    bool provider_msg_received;
} tlv_w3c_check_out_t;

typedef enum tlv_w3c_check_status_e {
    TLV_W3C_CHECK_SUCCESS               = 0,
    TLV_W3C_CHECK_PARSING_ERROR         = 1,
    TLV_W3C_CHECK_MISSING_STRUCTURE_TAG = 2,
    TLV_W3C_CHECK_WRONG_TYPE            = 3,
    TLV_W3C_CHECK_MISSING_TAG           = 4,
    TLV_W3C_CHECK_UNKNOWN_VERSION       = 5,
    TLV_W3C_CHECK_UNSUPPORTED_TAG       = 6,
    TLV_W3C_CHECK_HASH_FAILED           = 7,
    // Combined as a mask with check_signature_with_pki_status_t
    TLV_W3C_CHECK_SIGNATURE_ERROR = 0x80,
} tlv_w3c_check_status_t;

/**
 * @brief Parse a W3C Check (transaction simulation) TLV payload.
 *
 * Parses the provided TLV payload, verifies the PKI signature, and extracts the simulation
 * result into the output structure.
 *
 * @param[in]  payload  Pointer to the buffer containing the TLV payload data.
 * @param[out] out      Pointer to the structure where the parsed W3C check result will be stored.
 *
 * @return Status code indicating the result of the operation.
 *         See `tlv_w3c_check_status_t` for possible values.
 */
tlv_w3c_check_status_t tlv_use_case_w3c_check(const buffer_t       *payload,
                                               tlv_w3c_check_out_t  *out);

#endif  // HAVE_TRANSACTION_CHECKS
