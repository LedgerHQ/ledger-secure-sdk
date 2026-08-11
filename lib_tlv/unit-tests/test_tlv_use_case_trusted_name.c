/*****************************************************************************
 *   (c) 2025 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unity.h"
#include "Mockledger_assert_internals.h"
#include "Mocklcx_sha256.h"
#include "Mocklcx_sha3.h"
#include "Mocklcx_ripemd160.h"
#include "Mocklcx_sha512.h"
#include "Mocklcx_hash.h"
#include "Mockledger_pki.h"

#include "tlv_use_case_trusted_name.h"
#include "buffer.h"
#include "test_utils.h"

/* -------------------------------------------------------------------------- */
/* Test: Valid complete trusted name v1                                       */
/* -------------------------------------------------------------------------- */

void test_valid_trusted_name_v1(void)
{
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

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_SUCCESS);
    TEST_ASSERT_EQUAL_INT(out.version, 1);
    TEST_ASSERT_EQUAL_INT(out.trusted_name_type, TLV_TRUSTED_NAME_TYPE_EOA);
    TEST_ASSERT_EQUAL_INT(out.trusted_name_source, TLV_TRUSTED_NAME_SOURCE_CRYPTO_ASSET_LIST);
    TEST_ASSERT_EQUAL_INT(out.chain_id, 1);
    TEST_ASSERT_EQUAL_INT(out.address.size, 42);
    TEST_ASSERT_EQUAL_MEMORY(out.address.ptr, "0x1234567890abcdef1234567890abcdef12345678", 42);
    TEST_ASSERT_EQUAL_INT(out.trusted_name.size, 6);
    TEST_ASSERT_EQUAL_MEMORY(out.trusted_name.ptr, "Ledger", 6);
    TEST_ASSERT_FALSE(out.nft_id_received);
    TEST_ASSERT_FALSE(out.source_contract_received);
    TEST_ASSERT_FALSE(out.challenge_received);
    TEST_ASSERT_FALSE(out.not_valid_after_received);
    TEST_ASSERT_FALSE(out.blockchain_family_received);
}

/* -------------------------------------------------------------------------- */
/* Test: Valid trusted name v2 with optional fields                           */
/* -------------------------------------------------------------------------- */

void test_valid_trusted_name_v2_with_optionals(void)
{
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
    // Optional: Blockchain family
    append_tlv_uint8(payload, &offset, 0x51, TLV_TRUSTED_NAME_BLOCKCHAIN_FAMILY_ETHEREUM);
    append_tlv_uint16(payload, &offset, 0x13, TLV_TRUSTED_NAME_SIGNER_KEY_ID_PROD);
    append_tlv_uint8(payload, &offset, 0x14, TLV_TRUSTED_NAME_SIGNER_ALGORITHM_ECDSA_SHA256);
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x15, signature, sizeof(signature));

    buffer_t               buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_trusted_name_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_SUCCESS);
    TEST_ASSERT_EQUAL_INT(out.version, 2);
    TEST_ASSERT_TRUE(out.nft_id_received);
    TEST_ASSERT_TRUE(out.source_contract_received);
    TEST_ASSERT_TRUE(out.challenge_received);
    TEST_ASSERT_TRUE(out.not_valid_after_received);
    TEST_ASSERT_EQUAL_INT(out.challenge, 0x12345678);
    TEST_ASSERT_EQUAL_INT(out.not_valid_after.major, 1);
    TEST_ASSERT_EQUAL_INT(out.not_valid_after.minor, 2);
    TEST_ASSERT_EQUAL_INT(out.not_valid_after.patch, 0x0304);
    TEST_ASSERT_TRUE(out.blockchain_family_received);
    TEST_ASSERT_EQUAL_INT(out.blockchain_family, TLV_TRUSTED_NAME_BLOCKCHAIN_FAMILY_ETHEREUM);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing structure type tag                                           */
/* -------------------------------------------------------------------------- */

void test_missing_structure_type(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

void test_wrong_structure_type(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

void test_missing_version_tag(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_MISSING_TAG);
}

void test_missing_signature_tag(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

void test_version_zero(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_UNKNOWN_VERSION);
}

void test_version_too_high(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Source contract in v1 (unsupported)                                  */
/* -------------------------------------------------------------------------- */

void test_source_contract_in_v1(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_UNSUPPORTED_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong key ID                                                         */
/* -------------------------------------------------------------------------- */

void test_wrong_key_id(void)
{
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

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_WRONG_KEY_ID);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

void test_signature_verification_failure(void)
{
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

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE);

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    TEST_ASSERT_TRUE((result & TLV_TRUSTED_NAME_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

void test_invalid_tlv_format(void)
{
    uint8_t                payload[10] = {0x01, 0xFF, 0x80};  // Length exceeds buffer
    buffer_t               buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_trusted_name_out_t out         = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

void test_empty_payload(void)
{
    uint8_t                payload[1] = {0};
    buffer_t               buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_trusted_name_out_t out        = {0};

    tlv_trusted_name_status_t result = tlv_use_case_trusted_name(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_TRUSTED_NAME_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

void setUp(void)
{
    Mockledger_assert_internals_Init();
    Mocklcx_sha256_Init();
    Mocklcx_sha3_Init();
    Mocklcx_ripemd160_Init();
    Mocklcx_sha512_Init();
    Mocklcx_hash_Init();
    Mockledger_pki_Init();

    assert_exit_Ignore();
    assert_display_exit_Ignore();
    cx_sha256_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_sha3_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_keccak_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_ripemd160_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_sha512_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_hash_update_IgnoreAndReturn(CX_OK);
    cx_hash_final_IgnoreAndReturn(CX_OK);
}

void tearDown(void)
{
    Mockledger_assert_internals_Verify();
    Mockledger_assert_internals_Destroy();
    Mocklcx_sha256_Verify();
    Mocklcx_sha256_Destroy();
    Mocklcx_sha3_Verify();
    Mocklcx_sha3_Destroy();
    Mocklcx_ripemd160_Verify();
    Mocklcx_ripemd160_Destroy();
    Mocklcx_sha512_Verify();
    Mocklcx_sha512_Destroy();
    Mocklcx_hash_Verify();
    Mocklcx_hash_Destroy();
    Mockledger_pki_Verify();
    Mockledger_pki_Destroy();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_trusted_name_v1);
    RUN_TEST(test_valid_trusted_name_v2_with_optionals);
    RUN_TEST(test_missing_structure_type);
    RUN_TEST(test_wrong_structure_type);
    RUN_TEST(test_missing_version_tag);
    RUN_TEST(test_missing_signature_tag);
    RUN_TEST(test_version_zero);
    RUN_TEST(test_version_too_high);
    RUN_TEST(test_source_contract_in_v1);
    RUN_TEST(test_wrong_key_id);
    RUN_TEST(test_signature_verification_failure);
    RUN_TEST(test_invalid_tlv_format);
    RUN_TEST(test_empty_payload);
    return UNITY_END();
}
