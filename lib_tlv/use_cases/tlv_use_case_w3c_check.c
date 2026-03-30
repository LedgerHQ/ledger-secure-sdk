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

#ifdef HAVE_TRANSACTION_CHECKS

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "os_utils.h"
#include "ox_ec.h"
#include "cx.h"
#include "os_pki.h"
#include "ledger_pki.h"
#include "tlv_use_case_w3c_check.h"

#define TYPE_W3C_CHECK    0x09
#define W3C_CHECK_VERSION 0x01

#define DER_SIGNATURE_MIN_SIZE 8   // Minimum ECDSA DER signature size
#define DER_SIGNATURE_MAX_SIZE 72  // Maximum ECDSA DER signature size

// Parsed TLV data (internal context)
typedef struct tlv_extracted_s {
    // Received tags set by the parser
    TLV_reception_t received_tags;

    // W3C check output data
    tlv_w3c_check_out_t *output;

    // Tags handled by the use case API and not the caller
    uint8_t structure_type;
    uint8_t structure_version;

    buffer_t input_sig;

    // Progressive hash of the received TLVs (except the signature tag)
    cx_sha256_t hash_ctx;
} tlv_extracted_t;

// ─────────────────────────────────────────────────────────────────────────────
// Tag handlers
// ─────────────────────────────────────────────────────────────────────────────

static bool handle_structure_type(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    return get_uint8_t_from_tlv_data(data, &tlv_extracted->structure_type);
}

static bool handle_structure_version(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    return get_uint8_t_from_tlv_data(data, &tlv_extracted->structure_version);
}

static bool handle_tx_hash(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    buffer_t hash = {0};
    if (!get_buffer_from_tlv_data(
            data, &hash, W3C_CHECK_HASH_SIZE, W3C_CHECK_HASH_SIZE)) {
        PRINTF("TX_HASH: failed to extract\n");
        return false;
    }
    memmove(tlv_extracted->output->tx_hash, hash.ptr, hash.size);
    return true;
}

static bool handle_domain_hash(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    buffer_t hash = {0};
    if (!get_buffer_from_tlv_data(
            data, &hash, W3C_CHECK_HASH_SIZE, W3C_CHECK_HASH_SIZE)) {
        PRINTF("DOMAIN_HASH: failed to extract\n");
        return false;
    }
    memmove(tlv_extracted->output->domain_hash, hash.ptr, hash.size);
    return true;
}

static bool handle_address(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    buffer_t address = {0};
    // Accept variable address sizes: 20 (EVM) or 43-44 (Solana base58)
    if (!get_buffer_from_tlv_data(data, &address, 1, W3C_CHECK_MAX_ADDRESS_SIZE)) {
        PRINTF("ADDRESS: failed to extract\n");
        return false;
    }
    memmove(tlv_extracted->output->address, address.ptr, address.size);
    tlv_extracted->output->address_len = address.size;
    return true;
}

static bool handle_chain_id(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    return get_uint64_t_from_tlv_data(data, &tlv_extracted->output->chain_id);
}

static bool handle_risk(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("W3C_NORMALIZED_RISK: failed to extract\n");
        return false;
    }
    // UNKNOWN level is an internal fallback, not an API level — reject it as input
    if (value > W3C_CHECK_RISK_MALICIOUS) {
        PRINTF("W3C_NORMALIZED_RISK: value %d out of range\n", value);
        return false;
    }
    tlv_extracted->output->risk = (w3c_check_risk_t) value;
    return true;
}

static bool handle_category(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("W3C_NORMALIZED_CATEGORY: failed to extract\n");
        return false;
    }
    if (value >= W3C_CHECK_CATEGORY_COUNT) {
        PRINTF("W3C_NORMALIZED_CATEGORY: value %d out of range\n", value);
        return false;
    }
    tlv_extracted->output->category = (w3c_check_category_t) value;
    return true;
}

static bool handle_type(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    uint8_t value = 0;
    if (!get_uint8_t_from_tlv_data(data, &value)) {
        PRINTF("W3C_SIMULATION_TYPE: failed to extract\n");
        return false;
    }
    if (value >= W3C_CHECK_TYPE_COUNT) {
        PRINTF("W3C_SIMULATION_TYPE: value %d out of range\n", value);
        return false;
    }
    tlv_extracted->output->type = (w3c_check_type_t) value;
    return true;
}

/**
 * Check that a string is printable ASCII.
 */
static bool is_printable_string(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!isprint((unsigned char) str[i])) {
            return false;
        }
    }
    return true;
}

static bool handle_provider_msg(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    if (!get_string_from_tlv_data(data,
                                  tlv_extracted->output->provider_msg,
                                  0,
                                  sizeof(tlv_extracted->output->provider_msg))) {
        PRINTF("W3C_PROVIDER_MESSAGE: failed to extract\n");
        return false;
    }
    if (!is_printable_string(tlv_extracted->output->provider_msg,
                             strlen(tlv_extracted->output->provider_msg))) {
        PRINTF("W3C_PROVIDER_MESSAGE: not printable\n");
        return false;
    }
    return true;
}

static bool handle_tiny_url(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    if (!get_string_from_tlv_data(data,
                                  tlv_extracted->output->tiny_url,
                                  0,
                                  sizeof(tlv_extracted->output->tiny_url))) {
        PRINTF("W3C_TINY_URL: failed to extract\n");
        return false;
    }
    if (!is_printable_string(tlv_extracted->output->tiny_url,
                             strlen(tlv_extracted->output->tiny_url))) {
        PRINTF("W3C_TINY_URL: not printable\n");
        return false;
    }
    return true;
}

static bool handle_signature(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    return get_buffer_from_tlv_data(
        data, &tlv_extracted->input_sig, DER_SIGNATURE_MIN_SIZE, DER_SIGNATURE_MAX_SIZE);
}

// ─────────────────────────────────────────────────────────────────────────────
// TLV parser macro
// ─────────────────────────────────────────────────────────────────────────────

static bool handle_common(const tlv_data_t *data, tlv_extracted_t *tlv_extracted);

// clang-format off
#define W3C_CHECK_TLV_TAGS(X)                                                                      \
    X(0x01, TAG_STRUCTURE_TYPE,              handle_structure_type,    ENFORCE_UNIQUE_TAG)           \
    X(0x02, TAG_STRUCTURE_VERSION,           handle_structure_version, ENFORCE_UNIQUE_TAG)           \
    X(0x22, TAG_ADDRESS,                     handle_address,           ENFORCE_UNIQUE_TAG)           \
    X(0x23, TAG_CHAIN_ID,                    handle_chain_id,          ENFORCE_UNIQUE_TAG)           \
    X(0x27, TAG_TX_HASH,                     handle_tx_hash,           ENFORCE_UNIQUE_TAG)           \
    X(0x28, TAG_DOMAIN_HASH,                 handle_domain_hash,       ENFORCE_UNIQUE_TAG)           \
    X(0x80, TAG_W3C_NORMALIZED_RISK,         handle_risk,              ENFORCE_UNIQUE_TAG)           \
    X(0x81, TAG_W3C_NORMALIZED_CATEGORY,     handle_category,          ENFORCE_UNIQUE_TAG)           \
    X(0x82, TAG_W3C_PROVIDER_MSG,            handle_provider_msg,      ENFORCE_UNIQUE_TAG)           \
    X(0x83, TAG_W3C_TINY_URL,               handle_tiny_url,          ENFORCE_UNIQUE_TAG)           \
    X(0x84, TAG_W3C_SIMULATION_TYPE,         handle_type,              ENFORCE_UNIQUE_TAG)           \
    X(0x85, TAG_W3C_ADDITIONAL_DATA,         NULL,                     ENFORCE_UNIQUE_TAG)           \
    X(0x15, TAG_DER_SIGNATURE,               handle_signature,         ENFORCE_UNIQUE_TAG)
// clang-format on

DEFINE_TLV_PARSER(W3C_CHECK_TLV_TAGS, &handle_common, parse_w3c_check_tlv)

/**
 * @brief Common handler called for all tags to progressively hash them (except signature).
 */
static bool handle_common(const tlv_data_t *data, tlv_extracted_t *tlv_extracted)
{
    if (data->tag != TAG_DER_SIGNATURE) {
        CX_ASSERT(cx_hash_update(
            (cx_hash_t *) &tlv_extracted->hash_ctx, data->raw.ptr, data->raw.size));
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Verification
// ─────────────────────────────────────────────────────────────────────────────

static tlv_w3c_check_status_t verify_struct(const tlv_extracted_t *tlv_extracted)
{
    // Check structure type tag present
    if (!TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_STRUCTURE_TYPE)) {
        PRINTF("Error: no struct type specified!\n");
        return TLV_W3C_CHECK_MISSING_STRUCTURE_TAG;
    }

    if (tlv_extracted->structure_type != TYPE_W3C_CHECK) {
        PRINTF("Error: unexpected struct type %d\n", tlv_extracted->structure_type);
        return TLV_W3C_CHECK_WRONG_TYPE;
    }

    // Check mandatory fields
    if (!TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags,
                                 TAG_STRUCTURE_VERSION,
                                 TAG_TX_HASH,
                                 TAG_ADDRESS,
                                 TAG_W3C_NORMALIZED_RISK,
                                 TAG_W3C_NORMALIZED_CATEGORY,
                                 TAG_W3C_TINY_URL,
                                 TAG_W3C_SIMULATION_TYPE,
                                 TAG_DER_SIGNATURE)) {
        PRINTF("Error: missing mandatory tags in W3C check TLV\n");
        return TLV_W3C_CHECK_MISSING_TAG;
    }

    // Reject the additional data tag for now
    if (TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_W3C_ADDITIONAL_DATA)) {
        PRINTF("Error: unexpected W3C_ADDITIONAL_DATA tag received\n");
        return TLV_W3C_CHECK_UNSUPPORTED_TAG;
    }

    // Version check
    if (tlv_extracted->structure_version != W3C_CHECK_VERSION) {
        PRINTF("Error: unsupported W3C check version %d\n", tlv_extracted->structure_version);
        return TLV_W3C_CHECK_UNKNOWN_VERSION;
    }

    // Type-specific required fields
    if (tlv_extracted->output->type == W3C_CHECK_TYPE_TRANSACTION) {
        if (!TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_CHAIN_ID)) {
            PRINTF("Error: missing TAG_CHAIN_ID for W3C_CHECK_TYPE_TRANSACTION\n");
            return TLV_W3C_CHECK_MISSING_TAG;
        }
    }
    if (tlv_extracted->output->type == W3C_CHECK_TYPE_TYPED_DATA) {
        if (!TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_DOMAIN_HASH)) {
            PRINTF("Error: missing TAG_DOMAIN_HASH for W3C_CHECK_TYPE_TYPED_DATA\n");
            return TLV_W3C_CHECK_MISSING_TAG;
        }
    }

    // Forward optional field reception flags
    tlv_extracted->output->chain_id_received
        = TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_CHAIN_ID);
    tlv_extracted->output->domain_hash_received
        = TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_DOMAIN_HASH);
    tlv_extracted->output->provider_msg_received
        = TLV_CHECK_RECEIVED_TAGS(tlv_extracted->received_tags, TAG_W3C_PROVIDER_MSG);

    return TLV_W3C_CHECK_SUCCESS;
}

static tlv_w3c_check_status_t verify_signature(tlv_extracted_t *tlv_extracted)
{
    // Finalize the SHA-256 hash
    uint8_t  tlv_hash[CX_SHA256_SIZE] = {0};
    CX_ASSERT(cx_hash_final((cx_hash_t *) &tlv_extracted->hash_ctx, tlv_hash));
    buffer_t hash = {.ptr = tlv_hash, .size = sizeof(tlv_hash)};

    // Verify with PKI
    uint8_t    expected_key_usage = CERTIFICATE_PUBLIC_KEY_USAGE_TX_SIMU_SIGNER;
    cx_curve_t expected_curve     = CX_CURVE_SECP256K1;

    check_signature_with_pki_status_t err;
    err = check_signature_with_pki(hash, &expected_key_usage, &expected_curve,
                                   tlv_extracted->input_sig);
    if (err != CHECK_SIGNATURE_WITH_PKI_SUCCESS) {
        PRINTF("Failed to verify W3C check signature\n");
        return TLV_W3C_CHECK_SIGNATURE_ERROR | err;
    }

    return TLV_W3C_CHECK_SUCCESS;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

tlv_w3c_check_status_t tlv_use_case_w3c_check(const buffer_t      *payload,
                                               tlv_w3c_check_out_t *out)
{
    // Initialize the parsing context
    tlv_extracted_t tlv_extracted = {0};
    tlv_extracted.output          = out;

    // Reset output
    memset(out, 0, sizeof(*out));

    // Initialize the progressive SHA-256 hash
    CX_ASSERT(cx_sha256_init_no_throw(&tlv_extracted.hash_ctx));

    // Parse the TLV payload
    if (!parse_w3c_check_tlv(payload, &tlv_extracted, &tlv_extracted.received_tags)) {
        PRINTF("Failed to parse W3C check TLV payload\n");
        return TLV_W3C_CHECK_PARSING_ERROR;
    }

    // Verify structural integrity
    tlv_w3c_check_status_t err = verify_struct(&tlv_extracted);
    if (err != TLV_W3C_CHECK_SUCCESS) {
        PRINTF("Failed to verify W3C check TLV structure\n");
        return err;
    }

    // Verify PKI signature
    err = verify_signature(&tlv_extracted);
    if (err != TLV_W3C_CHECK_SUCCESS) {
        PRINTF("Failed to verify W3C check signature\n");
        return err;
    }

    return TLV_W3C_CHECK_SUCCESS;
}

#endif  // HAVE_TRANSACTION_CHECKS
