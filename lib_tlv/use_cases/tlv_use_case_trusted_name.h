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
 * This SDK helper implements the following cross-application specification:
 * https://ledgerhq.atlassian.net/wiki/spaces/TrustServices/pages/3736863735/LNS+Arch+Nano+Trusted+Names+Descriptor+Format+APIs
 *
 * Please refer to the tlv_library.h file for documentation on how to write your own use-case if it
 * does not follow the above specification.
 *
 * This descriptor associates a Blockchain address to a trusted domain name.
 * The trusted information comes from the Ledger CAL and is forwarded by the Ledger Live.
 */

#define TRUSTED_NAME_STRINGS_MAX_SIZE 64
#define NFT_ID_SIZE                   32

// source_contract_received field exists from version 3
#define SOURCE_CONTRACT_RECEIVED_MIN_VERSION 3
#define CURRENT_TRUSTED_NAME_SPEC_VERSION    3

#define SEMVER_SIZE 3

#define DER_SIGNATURE_MIN_SIZE 64  // Ed25519 size
#define DER_SIGNATURE_MAX_SIZE 72  // ECDSA max size

typedef enum tlv_structure_type_e {
    TLV_STRUCTURE_TYPE_TRUSTED_NAME = 0x03,
} tlv_structure_type_t;

typedef enum tlv_trusted_name_type_e {
    TLV_TRUSTED_NAME_TYPE_EOA             = 0x01,
    TLV_TRUSTED_NAME_TYPE_SMART_CONTRACT  = 0x02,
    TLV_TRUSTED_NAME_TYPE_COLLECTION      = 0x03,
    TLV_TRUSTED_NAME_TYPE_TOKEN           = 0x04,
    TLV_TRUSTED_NAME_TYPE_WALLET          = 0x05,
    TLV_TRUSTED_NAME_TYPE_CONTEXT_ADDRESS = 0x06,
} tlv_trusted_name_type_t;

typedef enum tlv_trusted_name_source_e {
    TLV_TRUSTED_NAME_SOURCE_LOCAL_ADDRESS_BOOK  = 0x00,
    TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST   = 0x01,
    TLV_TRUSTED_NAME_SOURCE_ENS                 = 0x02,
    TLV_TRUSTED_NAME_SOURCE_UNSTOPPABLE_DOMAINS = 0x03,
    TLV_TRUSTED_NAME_SOURCE_FREENAME            = 0x04,
    TLV_TRUSTED_NAME_SOURCE_DNS                 = 0x05,
    TLV_TRUSTED_NAME_SOURCE_DYNAMIC_RESOLVER    = 0x06,
} tlv_trusted_name_source_t;

typedef enum tlv_trusted_name_signer_key_id_e {
    TLV_TRUSTED_NAME_SIGNER_KEY_ID_TEST = 0x00,
    TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD = 0x07,
} tlv_trusted_name_signer_key_id_t;

typedef enum tlv_trusted_name_signer_algorithm_e {
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_SHA256     = 0x01,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_SHA3_256   = 0x02,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256 = 0x03,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_RIPEMD160  = 0x04,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_SHA512     = 0x16,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_EDDSA_KECCAK_256 = 0x17,
    TLV_TRUSTED_NAME_SIGNER_ALGORITHM_EDDSA_SHA3_256   = 0x18,
} tlv_trusted_name_signer_algorithm_t;

typedef struct semver_s {
    uint8_t  major;
    uint8_t  minor;
    uint16_t patch;
} semver_t;

// Output of the use-case TLV parser
typedef struct tlv_trusted_name_out_s {
    // Version number of the serialization format
    uint8_t version;
    // Encodes what type of trusted name this descriptor describes. tlv_trusted_name_type_t
    uint8_t trusted_name_type;
    // Describes the namespace in which the trusted name is defined. tlv_trusted_name_source_t
    uint8_t trusted_name_source;
    // The trusted name (UTF-8 string)
    buffer_t trusted_name;
    // The chain id of the network for which the trusted name was set.
    uint64_t chain_id;
    // The address resolved by the trusted name
    buffer_t address;

    // Optional fields, set only if the individual flags below are true
    // The token id of the NFT)
    buffer_t nft_id;
    // The source contract which created this address)
    buffer_t source_contract;
    // A challenge to ensure the freshness of the trusted name.
    uint32_t challenge;
    // A version date to revoke the trusted name validity.
    semver_t not_valid_after;

    // Flag optional tags reception
    bool nft_id_received;
    bool source_contract_received;
    bool challenge_received;
    bool not_valid_after_received;
} tlv_trusted_name_out_t;

typedef enum tlv_trusted_name_status_e {
    TLV_TRUSTED_NAME_SUCCESS               = 0,
    TLV_TRUSTED_NAME_PARSING_ERROR         = 1,
    TLV_TRUSTED_NAME_MISSING_STRUCTURE_TAG = 2,
    TLV_TRUSTED_NAME_WRONG_TYPE            = 3,
    TLV_TRUSTED_NAME_MISSING_TAG           = 4,
    TLV_TRUSTED_NAME_UNKNOWN_VERSION       = 5,
    TLV_TRUSTED_NAME_UNSUPPORTED_TAG       = 6,
    TLV_TRUSTED_NAME_WRONG_KEY_ID          = 7,
    TLV_TRUSTED_NAME_HASH_FAILED           = 8,
    // Combined as a mask with check_signature_with_pki_status_t
    TLV_TRUSTED_NAME_SIGNATURE_ERROR = 0x80,
} tlv_trusted_name_status_t;

/**
 * @brief Processes a TLV dynamic descriptor use case.
 *
 * This function parses the provided payload buffer and extracts dynamic descriptor information,
 * storing the results in the output structure.
 *
 * @param[in]  payload  Pointer to the buffer containing the TLV payload data.
 * @param[out] out      Pointer to the structure where the parsed dynamic descriptor will be stored.
 *
 * @return Status code indicating the result of the operation.
 *         See `tlv_dynamic_descriptor_status_t` for possible values.
 */
tlv_trusted_name_status_t tlv_use_case_trusted_name(const buffer_t         *payload,
                                                    tlv_trusted_name_out_t *out);
