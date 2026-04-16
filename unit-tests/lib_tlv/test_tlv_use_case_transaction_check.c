/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef UNIT_TESTING
#undef UNIT_TESTING
#include <cmocka.h>
#define UNIT_TESTING
#else
#include <cmocka.h>
#endif

#include "os_pki.h"
#include "tlv_use_case_transaction_check.h"
#include "ledger_pki.h"
#include "buffer.h"

/* -------------------------------------------------------------------------- */
/* Mock definitions                                                           */
/* -------------------------------------------------------------------------- */

check_signature_with_pki_status_t check_signature_with_pki(const buffer_t    hash,
                                                           const uint8_t    *expected_key_usage,
                                                           const cx_curve_t *expected_curve,
                                                           const buffer_t    signature)
{
    (void) hash;
    (void) expected_key_usage;
    (void) expected_curve;
    (void) signature;
    return mock_type(check_signature_with_pki_status_t);
}

/* -------------------------------------------------------------------------- */
/* Helper functions to build TLV payloads                                     */
/* -------------------------------------------------------------------------- */

/**
 * Encode a value in DER format (short or long form) into buffer at *offset.
 */
static void der_encode(uint8_t *buffer, size_t *offset, uint32_t value)
{
    if (value < 0x80) {
        buffer[(*offset)++] = (uint8_t) value;
    }
    else if (value <= 0xFF) {
        buffer[(*offset)++] = 0x81;
        buffer[(*offset)++] = (uint8_t) value;
    }
    else if (value <= 0xFFFF) {
        buffer[(*offset)++] = 0x82;
        buffer[(*offset)++] = (uint8_t) (value >> 8);
        buffer[(*offset)++] = (uint8_t) value;
    }
    else {
        // Not needed for our tests, but for completeness
        buffer[(*offset)++] = 0x84;
        buffer[(*offset)++] = (uint8_t) (value >> 24);
        buffer[(*offset)++] = (uint8_t) (value >> 16);
        buffer[(*offset)++] = (uint8_t) (value >> 8);
        buffer[(*offset)++] = (uint8_t) value;
    }
}

static void append_tlv(uint8_t       *buffer,
                       size_t        *offset,
                       uint32_t       tag,
                       const uint8_t *value,
                       size_t         value_len)
{
    der_encode(buffer, offset, tag);
    der_encode(buffer, offset, (uint32_t) value_len);
    memcpy(&buffer[*offset], value, value_len);
    *offset += value_len;
}

static void append_tlv_uint8(uint8_t *buffer, size_t *offset, uint32_t tag, uint8_t value)
{
    append_tlv(buffer, offset, tag, &value, 1);
}

static void append_tlv_uint64(uint8_t *buffer, size_t *offset, uint32_t tag, uint64_t value)
{
    uint8_t bytes[8];
    bytes[0] = (value >> 56) & 0xFF;
    bytes[1] = (value >> 48) & 0xFF;
    bytes[2] = (value >> 40) & 0xFF;
    bytes[3] = (value >> 32) & 0xFF;
    bytes[4] = (value >> 24) & 0xFF;
    bytes[5] = (value >> 16) & 0xFF;
    bytes[6] = (value >> 8) & 0xFF;
    bytes[7] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, 8);
}

static void append_tlv_string(uint8_t *buffer, size_t *offset, uint32_t tag, const char *str)
{
    append_tlv(buffer, offset, tag, (const uint8_t *) str, strlen(str));
}

/* -------------------------------------------------------------------------- */
/* Helper: build a valid transaction-type payload into buffer, return offset  */
/* -------------------------------------------------------------------------- */

// Tag values from the implementation
#define TAG_STRUCTURE_TYPE_VAL    0x01
#define TAG_STRUCTURE_VERSION_VAL 0x02
#define TAG_ADDRESS_VAL           0x22
#define TAG_CHAIN_ID_VAL          0x23
#define TAG_TX_HASH_VAL           0x27
#define TAG_DOMAIN_HASH_VAL       0x28
#define TAG_NORMALIZED_RISK_VAL   0x80
#define TAG_NORMALIZED_CAT_VAL    0x81
#define TAG_PROVIDER_MSG_VAL      0x82
#define TAG_TINY_URL_VAL          0x83
#define TAG_SIMULATION_TYPE_VAL   0x84
#define TAG_ADDITIONAL_DATA_VAL   0x85
#define TAG_DER_SIGNATURE_VAL     0x15

#define TYPE_TRANSACTION_CHECK 0x09
#define VERSION_1              0x01

static const uint8_t DUMMY_HASH[32] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
};

// 20-byte address (EVM minimum)
static const uint8_t DUMMY_ADDRESS[20] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33,
    0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD,
};

static size_t build_valid_transaction_payload(uint8_t *payload)
{
    size_t offset = 0;

    // Structure type: TRANSACTION_CHECK
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    // Version: 1
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    // TX hash (32 bytes)
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    // Address (20 bytes)
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    // Chain ID: 1 (Ethereum mainnet)
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    // Risk: BENIGN
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    // Category: NA
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    // Tiny URL
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    // Simulation type: TRANSACTION
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    // DER Signature (dummy 10 bytes, within [8..72])
    uint8_t signature[10] = {0};
    memset(signature, 0x42, sizeof(signature));
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    return offset;
}

/* -------------------------------------------------------------------------- */
/* Test: Valid complete transaction check (transaction type)                   */
/* -------------------------------------------------------------------------- */

static void test_valid_transaction_check(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = build_valid_transaction_payload(payload);

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    will_return(check_signature_with_pki, CHECK_SIGNATURE_WITH_PKI_SUCCESS);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_SUCCESS);
    assert_int_equal(out.risk, TRANSACTION_CHECK_RISK_BENIGN);
    assert_int_equal(out.category, TRANSACTION_CHECK_CATEGORY_NA);
    assert_int_equal(out.type, TRANSACTION_CHECK_TYPE_TRANSACTION);
    assert_int_equal(out.chain_id, 1);
    assert_true(out.chain_id_received);
    assert_int_equal(out.tx_hash.size, 32);
    assert_memory_equal(out.tx_hash.ptr, DUMMY_HASH, 32);
    assert_int_equal(out.address.size, 20);
    assert_memory_equal(out.address.ptr, DUMMY_ADDRESS, 20);
    assert_string_equal(out.tiny_url, "https://l.example/abc");
    assert_false(out.provider_msg_received);
    assert_false(out.domain_hash_received);
    assert_false(out.additional_data_received);
    assert_string_equal(out.partner, "TestPartner");
}

/* -------------------------------------------------------------------------- */
/* Test: Valid typed-data payload with optional fields                         */
/* -------------------------------------------------------------------------- */

static void test_valid_typed_data_with_optionals(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    // Risk: WARNING
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_WARNING);
    // Category: DAPP
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_DAPP);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/def");
    // Simulation type: TYPED_DATA
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TYPED_DATA);
    // Domain hash (required for typed data)
    append_tlv(payload, &offset, TAG_DOMAIN_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    // Optional: provider message
    append_tlv_string(payload, &offset, TAG_PROVIDER_MSG_VAL, "Risk detected");
    // Optional: additional data (no handler, just flagged)
    uint8_t extra[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, TAG_ADDITIONAL_DATA_VAL, extra, sizeof(extra));
    // DER Signature
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    will_return(check_signature_with_pki, CHECK_SIGNATURE_WITH_PKI_SUCCESS);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_SUCCESS);
    assert_int_equal(out.type, TRANSACTION_CHECK_TYPE_TYPED_DATA);
    assert_int_equal(out.risk, TRANSACTION_CHECK_RISK_WARNING);
    assert_int_equal(out.category, TRANSACTION_CHECK_CATEGORY_DAPP);
    assert_true(out.domain_hash_received);
    assert_int_equal(out.domain_hash.size, 32);
    assert_true(out.provider_msg_received);
    assert_string_equal(out.provider_msg, "Risk detected");
    assert_true(out.additional_data_received);
    // chain_id is optional for typed data and was not provided
    assert_false(out.chain_id_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing structure type tag                                           */
/* -------------------------------------------------------------------------- */

static void test_missing_structure_type(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    // Missing structure type tag (0x01)
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

static void test_wrong_structure_type(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, 0xFF);  // Wrong type
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

static void test_missing_version_tag(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    // Missing version tag
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_MISSING_TAG);
}

static void test_missing_signature_tag(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    // Missing signature tag

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Optional chain_id absent for TRANSACTION type                        */
/* -------------------------------------------------------------------------- */

static void test_missing_chain_id_for_transaction(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    // No chain_id
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    will_return(check_signature_with_pki, CHECK_SIGNATURE_WITH_PKI_SUCCESS);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_SUCCESS);
    assert_false(out.chain_id_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Optional domain_hash absent for TYPED_DATA type                      */
/* -------------------------------------------------------------------------- */

static void test_missing_domain_hash_for_typed_data(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    // Typed data without domain hash
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TYPED_DATA);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    will_return(check_signature_with_pki, CHECK_SIGNATURE_WITH_PKI_SUCCESS);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_SUCCESS);
    assert_false(out.domain_hash_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

static void test_version_zero(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, 0x00);  // Version 0
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_UNKNOWN_VERSION);
}

static void test_version_too_high(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, 0x02);  // Version 2
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

static void test_signature_verification_failure(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = build_valid_transaction_payload(payload);

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    will_return(check_signature_with_pki, CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_true((result & TLV_TRANSACTION_CHECK_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Non-printable provider message rejected                              */
/* -------------------------------------------------------------------------- */

static void test_non_printable_provider_msg(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, TRANSACTION_CHECK_RISK_BENIGN);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    // Provider message with non-printable byte
    uint8_t bad_msg[] = {'H', 'e', 'l', 'l', 'o', 0x01, '!'};
    append_tlv(payload, &offset, TAG_PROVIDER_MSG_VAL, bad_msg, sizeof(bad_msg));
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Risk value out of range                                              */
/* -------------------------------------------------------------------------- */

static void test_risk_out_of_range(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_TYPE_VAL, TYPE_TRANSACTION_CHECK);
    append_tlv_uint8(payload, &offset, TAG_STRUCTURE_VERSION_VAL, VERSION_1);
    append_tlv(payload, &offset, TAG_TX_HASH_VAL, DUMMY_HASH, sizeof(DUMMY_HASH));
    append_tlv(payload, &offset, TAG_ADDRESS_VAL, DUMMY_ADDRESS, sizeof(DUMMY_ADDRESS));
    append_tlv_uint64(payload, &offset, TAG_CHAIN_ID_VAL, 1);
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_RISK_VAL, 0xFF);  // Out of range
    append_tlv_uint8(payload, &offset, TAG_NORMALIZED_CAT_VAL, TRANSACTION_CHECK_CATEGORY_NA);
    append_tlv_string(payload, &offset, TAG_TINY_URL_VAL, "https://l.example/abc");
    append_tlv_uint8(payload, &offset, TAG_SIMULATION_TYPE_VAL, TRANSACTION_CHECK_TYPE_TRANSACTION);
    uint8_t signature[10] = {0};
    append_tlv(payload, &offset, TAG_DER_SIGNATURE_VAL, signature, sizeof(signature));

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

static void test_invalid_tlv_format(void **state)
{
    (void) state;

    uint8_t                     payload[10] = {0x01, 0xFF, 0x09};  // Length exceeds buffer
    buffer_t                    buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_transaction_check_out_t out         = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

static void test_empty_payload(void **state)
{
    (void) state;

    uint8_t                     payload[1] = {0};
    buffer_t                    buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_transaction_check_out_t out        = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    assert_int_equal(result, TLV_TRANSACTION_CHECK_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_transaction_check),
        cmocka_unit_test(test_valid_typed_data_with_optionals),
        cmocka_unit_test(test_missing_structure_type),
        cmocka_unit_test(test_wrong_structure_type),
        cmocka_unit_test(test_missing_version_tag),
        cmocka_unit_test(test_missing_signature_tag),
        cmocka_unit_test(test_missing_chain_id_for_transaction),
        cmocka_unit_test(test_missing_domain_hash_for_typed_data),
        cmocka_unit_test(test_version_zero),
        cmocka_unit_test(test_version_too_high),
        cmocka_unit_test(test_signature_verification_failure),
        cmocka_unit_test(test_non_printable_provider_msg),
        cmocka_unit_test(test_risk_out_of_range),
        cmocka_unit_test(test_invalid_tlv_format),
        cmocka_unit_test(test_empty_payload),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
