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
#include "tokens.h"

/* TLV use-case API on top of the TLV library
 *
 * This SDK helper implements the following cross-application specification:
 * https://ledgerhq.atlassian.net/wiki/spaces/TA/pages/5603262535/Token+Dynamic+Descriptor
 *
 * Please refer to the tlv_library.h file for documentation on how to write your own use-case if it
 * does not follow the above specification.
 *
 * The goal of this TLV use case is to parse dynamic information about a token for clear signing
 * purposes.
 * The trusted information comes from the Ledger CAL and is forwarded by the Ledger Live.
 */

#define DER_SIGNATURE_MIN_SIZE 64  // Ed25519 size
#define DER_SIGNATURE_MAX_SIZE 72  // ECDSA max size

// Output of the use-case TLV parser
typedef struct tlv_dynamic_descriptor_out_s {
    // Version of the serialization format. Forwarded to the use-case caller as it may be needed
    // for TUID parsing.
    uint8_t version;
    // Coin Type as defined in SLIP-44
    uint32_t coin_type;
    // Magnitude of the token (number of decimals)
    uint8_t magnitude;
    // The TUID contains a sub-TLV payload containing application specific data of this token.
    buffer_t TUID;
    // 0 copy is inconvenient for strings because they are not '\0' terminated in the TLV reception
    // format
    char ticker[MAX_TICKER_SIZE + 1];
} tlv_dynamic_descriptor_out_t;

typedef enum tlv_dynamic_descriptor_status_e {
    TLV_DYNAMIC_DESCRIPTOR_SUCCESS                = 0,
    TLV_DYNAMIC_DESCRIPTOR_PARSING_ERROR          = 1,
    TLV_DYNAMIC_DESCRIPTOR_MISSING_STRUCTURE_TAG  = 2,
    TLV_DYNAMIC_DESCRIPTOR_WRONG_TYPE             = 3,
    TLV_DYNAMIC_DESCRIPTOR_MISSING_TAG            = 4,
    TLV_DYNAMIC_DESCRIPTOR_WRONG_APPLICATION_NAME = 5,
    TLV_DYNAMIC_DESCRIPTOR_UNKNOWN_VERSION        = 6,
    // Combined as a mask  with check_signature_with_pki_status_t
    TLV_DYNAMIC_DESCRIPTOR_SIGNATURE_ERROR = 0x80,
} tlv_dynamic_descriptor_status_t;

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
tlv_dynamic_descriptor_status_t tlv_use_case_dynamic_descriptor(const buffer_t *payload,
                                                                tlv_dynamic_descriptor_out_t *out);
