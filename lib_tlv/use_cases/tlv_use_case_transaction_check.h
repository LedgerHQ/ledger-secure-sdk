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

#include "tlv_library.h"
#include "buffer.h"

/* TLV use-case API on top of the TLV library
 *
 * This SDK helper implements the Transaction Check descriptor parsing.
 *
 * Please refer to the tlv_library.h file for documentation on how to write your own use-case if it
 * does not follow the above specification.
 *
 * This descriptor associates a transaction or typed-data hash with a risk assessment from a
 * security provider. The trusted information comes from the Ledger backend and is forwarded by
 * Ledger Live.
 */

// Maximum sizes from the spec
#define TRANSACTION_CHECK_HASH_SIZE        CX_SHA3_256_SIZE
#define TRANSACTION_CHECK_MSG_SIZE         25
#define TRANSACTION_CHECK_URL_SIZE         30
// EVM address is 20 bytes, Solana base58 address is up to 44 bytes
#define TRANSACTION_CHECK_MIN_ADDRESS_SIZE 20
#define TRANSACTION_CHECK_MAX_ADDRESS_SIZE 44

#define TRANSACTION_CHECK_PARTNER_SIZE 20

typedef enum transaction_check_risk_e {
    TRANSACTION_CHECK_RISK_BENIGN    = 0x00,
    TRANSACTION_CHECK_RISK_WARNING   = 0x01,
    TRANSACTION_CHECK_RISK_MALICIOUS = 0x02,
    TRANSACTION_CHECK_RISK_COUNT,
} transaction_check_risk_t;

typedef enum transaction_check_type_e {
    TRANSACTION_CHECK_TYPE_TRANSACTION = 0x00,
    TRANSACTION_CHECK_TYPE_TYPED_DATA  = 0x01,
    TRANSACTION_CHECK_TYPE_COUNT,
} transaction_check_type_t;

typedef enum transaction_check_category_e {
    TRANSACTION_CHECK_CATEGORY_NA               = 0x00,
    TRANSACTION_CHECK_CATEGORY_OTHERS           = 0x01,
    TRANSACTION_CHECK_CATEGORY_ADDRESS          = 0x02,
    TRANSACTION_CHECK_CATEGORY_DAPP             = 0x03,
    TRANSACTION_CHECK_CATEGORY_LOSING_OPERATION = 0x04,
    TRANSACTION_CHECK_CATEGORY_COUNT,
} transaction_check_category_t;

// Output of the use-case TLV parser
typedef struct tlv_transaction_check_out_s {
    // Chain ID (required for transactions, optional for typed data)
    uint64_t chain_id;
    // Transaction or message hash (32 bytes, zero-copy)
    buffer_t tx_hash;
    // Domain hash for EIP-712 typed data (32 bytes, zero-copy)
    buffer_t domain_hash;
    // Provider message (ASCII printable, null-terminated)
    char provider_msg[TRANSACTION_CHECK_MSG_SIZE + 1];
    // Report URL (null-terminated)
    char tiny_url[TRANSACTION_CHECK_URL_SIZE + 1];
    // Sender address (variable size, zero-copy)
    buffer_t address;
    // Partner name (from PKI certificate, null-terminated)
    char partner[TRANSACTION_CHECK_PARTNER_SIZE + 1];
    // Risk level
    transaction_check_risk_t risk;
    // Simulation type
    transaction_check_type_t type;
    // Risk category
    transaction_check_category_t category;

    // Flag optional tags reception
    bool chain_id_received;
    bool domain_hash_received;
    bool provider_msg_received;
    bool additional_data_received;
} tlv_transaction_check_out_t;

typedef enum tlv_transaction_check_status_e {
    TLV_TRANSACTION_CHECK_SUCCESS               = 0,
    TLV_TRANSACTION_CHECK_PARSING_ERROR         = 1,
    TLV_TRANSACTION_CHECK_MISSING_STRUCTURE_TAG = 2,
    TLV_TRANSACTION_CHECK_WRONG_TYPE            = 3,
    TLV_TRANSACTION_CHECK_MISSING_TAG           = 4,
    TLV_TRANSACTION_CHECK_UNKNOWN_VERSION       = 5,
    // Combined as a mask with check_signature_with_pki_status_t
    TLV_TRANSACTION_CHECK_SIGNATURE_ERROR = 0x80,
} tlv_transaction_check_status_t;

/**
 * @brief Parse a Transaction Check TLV payload.
 *
 * Parses the provided TLV payload, verifies the PKI signature, and extracts the simulation
 * result into the output structure.
 *
 * @param[in]  payload  Pointer to the buffer containing the TLV payload data.
 * @param[out] out      Pointer to the structure where the parsed Transaction Check result will be
 * stored.
 *
 * @return Status code indicating the result of the operation.
 *         See `tlv_transaction_check_status_t` for possible values.
 */
tlv_transaction_check_status_t tlv_use_case_transaction_check(const buffer_t              *payload,
                                                              tlv_transaction_check_out_t *out);
