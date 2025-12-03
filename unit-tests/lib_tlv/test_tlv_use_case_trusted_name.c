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

#include "tlv_use_case_trusted_name.h"
#include "ledger_pki.h"
#include "buffer.h"

/* -------------------------------------------------------------------------- */
/* Mock definitions                                                           */
/* -------------------------------------------------------------------------- */

static check_signature_with_pki_status_t mock_pki_result = CHECK_SIGNATURE_WITH_PKI_SUCCESS;

check_signature_with_pki_status_t check_signature_with_pki(const buffer_t    hash,
                                                           const uint8_t    *expected_key_usage,
                                                           const cx_curve_t *expected_curve,
                                                           const buffer_t    signature)
{
    (void) hash;
    (void) expected_key_usage;
    (void) expected_curve;
    (void) signature;
    return mock_pki_result;
}

/* -------------------------------------------------------------------------- */
/* Helper functions to build TLV payloads                                     */
/* -------------------------------------------------------------------------- */

static void append_tlv(uint8_t       *buffer,
                       size_t        *offset,
                       uint8_t        tag,
                       const uint8_t *value,
                       size_t         value_len)
{
    buffer[(*offset)++] = tag;
    buffer[(*offset)++] = (uint8_t) value_len;
    memcpy(&buffer[*offset], value, value_len);
    *offset += value_len;
}

static void append_tlv_uint8(uint8_t *buffer, size_t *offset, uint8_t tag, uint8_t value)
{
    append_tlv(buffer, offset, tag, &value, 1);
}

static void append_tlv_uint16(uint8_t *buffer, size_t *offset, uint8_t tag, uint16_t value)
{
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, 2);
}

static void append_tlv_uint32(uint8_t *buffer, size_t *offset, uint8_t tag, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, 4);
}

static void append_tlv_uint64(uint8_t *buffer, size_t *offset, uint8_t tag, uint64_t value)
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

static void append_tlv_string(uint8_t *buffer, size_t *offset, uint8_t tag, const char *str)
{
    append_tlv(buffer, offset, tag, (const uint8_t *) str, strlen(str));
}

/* -------------------------------------------------------------------------- */
/* Test: Valid complete trusted name v1                                       */
/* -------------------------------------------------------------------------- */

static void test_valid_trusted_name_v1(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    // Structure type: 0x03 (TYPE_TRUSTED_NAME)
    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    // Version: 1
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    // Trusted name type: EOA
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    // Trusted name source: CAL
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    // Trusted name: "Ledger"
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    // Chain ID: 1 (Ethereum mainnet)
    append_tlv_uint64(payload, &offset, 0x23, 1);
    // Address: sample Ethereum address
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    // Signer key ID: test key
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    // Signer algorithm: ECDSA_KECCAK_256
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    // DER Signature: dummy 64 bytes
    uint8_t signature[64] = {0};
    memset(signature, 0x42, sizeof(signature));
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_SUCCESS;

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_SUCCESS);
    assert_int_equal(out.version, 1);
    assert_int_equal(out.trusted_name_type, TLV_TRUSTED_NAME_TYPE_EOA);
    assert_int_equal(out.trusted_name_source, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    assert_int_equal(out.chain_id, 1);
    assert_int_equal(out.address.size, 42);
    assert_memory_equal(out.address.ptr, "0x1234567890abcdef1234567890abcdef12345678", 42);
    assert_int_equal(out.trusted_name.size, 6);
    assert_memory_equal(out.trusted_name.ptr, "Ledger", 6);
    assert_false(out.nft_id_received);
    assert_false(out.source_contract_received);
    assert_false(out.challenge_received);
    assert_false(out.not_valid_after_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Valid trusted name v2 with optional fields                           */
/* -------------------------------------------------------------------------- */

static void test_valid_trusted_name_v2_with_optionals(void **state)
{
    (void) state;

    uint8_t payload[512];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x02);  // Version 2
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_TOKEN);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_ENS);
    append_tlv_string(payload, &offset, 0x20, "MyToken");
    append_tlv_uint64(payload, &offset, 0x23, 137);  // Polygon
    append_tlv_string(payload, &offset, 0x22, "0xabcdef1234567890abcdef1234567890abcdef12");
    // Optional: NFT ID
    uint8_t nft_id[32] = {0};
    memset(nft_id, 0xAA, sizeof(nft_id));
    append_tlv(payload, &offset, 0x72, nft_id, sizeof(nft_id));
    // Optional: Source contract
    append_tlv_string(payload, &offset, 0x73, "0xfactory123456789012345678901234567890ab");
    // Optional: Challenge
    append_tlv_uint32(payload, &offset, 0x12, 0x12345678);
    // Optional: Not valid after (semver: major.minor.patch)
    uint8_t semver[4] = {0x01, 0x02, 0x03, 0x04};  // 1.2.772
    append_tlv(payload, &offset, 0x10, semver, sizeof(semver));
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_SHA256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_SUCCESS;

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_SUCCESS);
    assert_int_equal(out.version, 2);
    assert_true(out.nft_id_received);
    assert_true(out.source_contract_received);
    assert_true(out.challenge_received);
    assert_true(out.not_valid_after_received);
    assert_int_equal(out.challenge, 0x12345678);
    assert_int_equal(out.not_valid_after.major, 1);
    assert_int_equal(out.not_valid_after.minor, 2);
    assert_int_equal(out.not_valid_after.patch, 0x0304);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing structure type tag                                           */
/* -------------------------------------------------------------------------- */

static void test_missing_structure_type(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    // Missing structure type tag (0x01)
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

static void test_wrong_structure_type(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x99);  // Wrong type
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

static void test_missing_version_tag(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    // Missing version tag (0x02)
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_MISSING_TAG);
}

static void test_missing_signature_tag(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    // Missing signature tag (0x15)

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

static void test_version_zero(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x00);  // Version 0
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_UNKNOWN_VERSION);
}

static void test_version_too_high(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, CURRENT_TRUSTED_NAME_SPEC_VERSION + 1);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Source contract in v1 (unsupported)                                  */
/* -------------------------------------------------------------------------- */

static void test_source_contract_in_v1(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);  // Version 1
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_string(
        payload, &offset, 0x73, "0xfactory123456789012345678901234567890ab");  // Not allowed in v1
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_UNSUPPORTED_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong key ID                                                         */
/* -------------------------------------------------------------------------- */

static void test_wrong_key_id(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, 0x99);  // Wrong key ID
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_WRONG_KEY_ID);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

static void test_signature_verification_failure(void **state)
{
    (void) state;

    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, TLV_STRUCTURE_TYPE_TRUSTED_NAME);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint8(payload, &offset, 0x70, TLV_TRUSTED_NAME_TYPE_EOA);
    append_tlv_uint8(payload, &offset, 0x71, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    append_tlv_string(payload, &offset, 0x20, "Ledger");
    append_tlv_uint64(payload, &offset, 0x23, 1);
    append_tlv_string(payload, &offset, 0x22, "0x1234567890abcdef1234567890abcdef12345678");
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_KECCAK_256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE;

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_true((result & TLV_TRUSTED_NAME_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

static void test_invalid_tlv_format(void **state)
{
    (void) state;

    uint8_t                payload[10] = {0x01, 0xFF, 0x80};  // Length exceeds buffer
    buffer_t               buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_trusted_name_out_t out         = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

static void test_empty_payload(void **state)
{
    (void) state;

    uint8_t                payload[1] = {0};
    buffer_t               buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_trusted_name_out_t out        = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    assert_int_equal(result, TLV_TRUSTED_NAME_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_trusted_name_v1),
        cmocka_unit_test(test_valid_trusted_name_v2_with_optionals),
        cmocka_unit_test(test_missing_structure_type),
        cmocka_unit_test(test_wrong_structure_type),
        cmocka_unit_test(test_missing_version_tag),
        cmocka_unit_test(test_missing_signature_tag),
        cmocka_unit_test(test_version_zero),
        cmocka_unit_test(test_version_too_high),
        cmocka_unit_test(test_source_contract_in_v1),
        cmocka_unit_test(test_wrong_key_id),
        cmocka_unit_test(test_signature_verification_failure),
        cmocka_unit_test(test_invalid_tlv_format),
        cmocka_unit_test(test_empty_payload),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
