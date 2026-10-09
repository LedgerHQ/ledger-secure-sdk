/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unity.h"
#include "Mockledger_assert_internals.h"
#include "Mocklcx_sha256.h"
#include "Mocklcx_hash.h"
#include "Mockos_utils.h"
#include "Mockos_pki.h"
#include "Mockledger_pki.h"

#include "tlv_use_case_transaction_check.h"
#include "buffer.h"
#include "test_utils.h"

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

void test_valid_transaction_check(void)
{
    uint8_t payload[512];
    size_t  offset = build_valid_transaction_payload(payload);

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_SUCCESS);
    TEST_ASSERT_EQUAL_INT(out.risk, TRANSACTION_CHECK_RISK_BENIGN);
    TEST_ASSERT_EQUAL_INT(out.category, TRANSACTION_CHECK_CATEGORY_NA);
    TEST_ASSERT_EQUAL_INT(out.type, TRANSACTION_CHECK_TYPE_TRANSACTION);
    TEST_ASSERT_EQUAL_INT(out.chain_id, 1);
    TEST_ASSERT_TRUE(out.chain_id_received);
    TEST_ASSERT_EQUAL_INT(out.tx_hash.size, 32);
    TEST_ASSERT_EQUAL_MEMORY(out.tx_hash.ptr, DUMMY_HASH, 32);
    TEST_ASSERT_EQUAL_INT(out.address.size, 20);
    TEST_ASSERT_EQUAL_MEMORY(out.address.ptr, DUMMY_ADDRESS, 20);
    TEST_ASSERT_EQUAL_STRING(out.tiny_url, "https://l.example/abc");
    TEST_ASSERT_FALSE(out.provider_msg_received);
    TEST_ASSERT_FALSE(out.domain_hash_received);
    TEST_ASSERT_FALSE(out.additional_data_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Valid typed-data payload with optional fields                         */
/* -------------------------------------------------------------------------- */

void test_valid_typed_data_with_optionals(void)
{
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

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_SUCCESS);
    TEST_ASSERT_EQUAL_INT(out.type, TRANSACTION_CHECK_TYPE_TYPED_DATA);
    TEST_ASSERT_EQUAL_INT(out.risk, TRANSACTION_CHECK_RISK_WARNING);
    TEST_ASSERT_EQUAL_INT(out.category, TRANSACTION_CHECK_CATEGORY_DAPP);
    TEST_ASSERT_TRUE(out.domain_hash_received);
    TEST_ASSERT_EQUAL_INT(out.domain_hash.size, 32);
    TEST_ASSERT_TRUE(out.provider_msg_received);
    TEST_ASSERT_EQUAL_STRING(out.provider_msg, "Risk detected");
    TEST_ASSERT_TRUE(out.additional_data_received);
    // chain_id is optional for typed data and was not provided
    TEST_ASSERT_FALSE(out.chain_id_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing structure type tag                                           */
/* -------------------------------------------------------------------------- */

void test_missing_structure_type(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

void test_wrong_structure_type(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

void test_missing_version_tag(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_MISSING_TAG);
}

void test_missing_signature_tag(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Optional chain_id absent for TRANSACTION type                        */
/* -------------------------------------------------------------------------- */

void test_missing_chain_id_for_transaction(void)
{
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

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_SUCCESS);
    TEST_ASSERT_FALSE(out.chain_id_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Optional domain_hash absent for TYPED_DATA type                      */
/* -------------------------------------------------------------------------- */

void test_missing_domain_hash_for_typed_data(void)
{
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

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_SUCCESS);
    TEST_ASSERT_FALSE(out.domain_hash_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

void test_version_zero(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_UNKNOWN_VERSION);
}

void test_version_too_high(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

void test_signature_verification_failure(void)
{
    uint8_t payload[512];
    size_t  offset = build_valid_transaction_payload(payload);

    buffer_t                    buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_transaction_check_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE);

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_TRUE((result & TLV_TRANSACTION_CHECK_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Non-printable provider message rejected                              */
/* -------------------------------------------------------------------------- */

void test_non_printable_provider_msg(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Risk value out of range                                              */
/* -------------------------------------------------------------------------- */

void test_risk_out_of_range(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

void test_invalid_tlv_format(void)
{
    uint8_t                     payload[10] = {0x01, 0xFF, 0x09};  // Length exceeds buffer
    buffer_t                    buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_transaction_check_out_t out         = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

void test_empty_payload(void)
{
    uint8_t                     payload[1] = {0};
    buffer_t                    buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_transaction_check_out_t out        = {0};

    tlv_transaction_check_status_t result = tlv_use_case_transaction_check(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRANSACTION_CHECK_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

static bool is_printable_string_stub(const char *str, size_t len, int n)
{
    (void) n;
    for (size_t i = 0; i < len; i++) {
        if (!isprint((unsigned char) str[i])) {
            return false;
        }
    }
    return true;
}

void setUp(void)
{
    Mockledger_assert_internals_Init();
    Mocklcx_sha256_Init();
    Mocklcx_hash_Init();
    Mockos_utils_Init();
    Mockos_pki_Init();
    Mockledger_pki_Init();

    assert_exit_Ignore();
    assert_display_exit_Ignore();
    cx_sha256_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_hash_update_IgnoreAndReturn(CX_OK);
    cx_hash_final_IgnoreAndReturn(CX_OK);
    is_printable_string_Stub(is_printable_string_stub);
    os_pki_get_info_IgnoreAndReturn(0);
}

void tearDown(void)
{
    Mockledger_assert_internals_Verify();
    Mockledger_assert_internals_Destroy();
    Mocklcx_sha256_Verify();
    Mocklcx_sha256_Destroy();
    Mocklcx_hash_Verify();
    Mocklcx_hash_Destroy();
    Mockos_utils_Verify();
    Mockos_utils_Destroy();
    Mockos_pki_Verify();
    Mockos_pki_Destroy();
    Mockledger_pki_Verify();
    Mockledger_pki_Destroy();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_transaction_check);
    RUN_TEST(test_valid_typed_data_with_optionals);
    RUN_TEST(test_missing_structure_type);
    RUN_TEST(test_wrong_structure_type);
    RUN_TEST(test_missing_version_tag);
    RUN_TEST(test_missing_signature_tag);
    RUN_TEST(test_missing_chain_id_for_transaction);
    RUN_TEST(test_missing_domain_hash_for_typed_data);
    RUN_TEST(test_version_zero);
    RUN_TEST(test_version_too_high);
    RUN_TEST(test_signature_verification_failure);
    RUN_TEST(test_non_printable_provider_msg);
    RUN_TEST(test_risk_out_of_range);
    RUN_TEST(test_invalid_tlv_format);
    RUN_TEST(test_empty_payload);
    return UNITY_END();
}
