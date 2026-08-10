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
#include "Mocklcx_hash.h"
#include "Mockledger_pki.h"

#include "tlv_use_case_dynamic_descriptor.h"
#include "buffer.h"
#include "test_utils.h"

/* -------------------------------------------------------------------------- */
/* Test: Valid complete dynamic descriptor                                    */
/* -------------------------------------------------------------------------- */

void test_valid_dynamic_descriptor(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    // Structure type: 0x90 (TYPE_DYNAMIC_TOKEN)
    append_tlv_uint8(payload, &offset, 0x01, 0x90);

    // Version: 1
    append_tlv_uint8(payload, &offset, 0x02, 0x01);

    // Coin type: 0x8000003C (Bitcoin testnet)
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);

    // Application name: "TestApp"
    append_tlv_string(payload, &offset, 0x04, APPNAME);

    // Ticker: "BTC"
    append_tlv_string(payload, &offset, 0x05, "BTC");

    // Magnitude: 8
    append_tlv_uint8(payload, &offset, 0x06, 0x08);

    // TUID: 4 bytes
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));

    // Signature: dummy 64 bytes
    uint8_t signature[64] = {0};
    memset(signature, 0x42, sizeof(signature));
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_SUCCESS);
    TEST_ASSERT_EQUAL_INT(out.version, 1);
    TEST_ASSERT_EQUAL_INT(out.coin_type, 0x8000003C);
    TEST_ASSERT_EQUAL_STRING(out.ticker, "BTC");
    TEST_ASSERT_EQUAL_INT(out.magnitude, 8);
    TEST_ASSERT_EQUAL_INT(out.TUID.size, 4);
    TEST_ASSERT_EQUAL_MEMORY(out.TUID.ptr, tuid, 4);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing structure type tag                                           */
/* -------------------------------------------------------------------------- */

void test_missing_structure_type(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    // Missing structure type tag (0x01)
    append_tlv_uint8(payload, &offset, 0x02, 0x01);         // Version
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);  // Coin type
    append_tlv_string(payload, &offset, 0x04, APPNAME);     // App name
    append_tlv_string(payload, &offset, 0x05, "BTC");       // Ticker
    append_tlv_uint8(payload, &offset, 0x06, 0x08);         // Magnitude
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));  // TUID
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));  // Signature

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

void test_wrong_structure_type(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x99);  // Wrong type (not 0x90)
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

void test_missing_version_tag(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    // Missing version tag (0x02)
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_TAG);
}

void test_missing_signature_tag(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    // Missing signature tag (0x08)

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong application name                                               */
/* -------------------------------------------------------------------------- */

void test_wrong_application_name(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, "WrongApp");  // Wrong app name
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_WRONG_APPLICATION_NAME);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

void test_version_zero(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x00);  // Version 0
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_UNKNOWN_VERSION);
}

void test_version_too_high(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x02);  // Version 2 (unsupported)
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

void test_signature_verification_failure(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "BTC");
    append_tlv_uint8(payload, &offset, 0x06, 0x08);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE);

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_TRUE((result & TLV_DYNAMIC_DESCRIPTOR_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

void test_invalid_tlv_format(void)
{
    uint8_t                      payload[10] = {0x01, 0xFF, 0x90};  // Length exceeds buffer
    buffer_t                     buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_dynamic_descriptor_out_t out         = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

void test_empty_payload(void)
{
    uint8_t                      payload[1] = {0};
    buffer_t                     buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_dynamic_descriptor_out_t out        = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Ticker and coin type validation                                      */
/* -------------------------------------------------------------------------- */

void test_various_tickers(void)
{
    uint8_t payload[256];
    size_t  offset = 0;

    append_tlv_uint8(payload, &offset, 0x01, 0x90);
    append_tlv_uint8(payload, &offset, 0x02, 0x01);
    append_tlv_uint32(payload, &offset, 0x03, 0x8000003C);
    append_tlv_string(payload, &offset, 0x04, APPNAME);
    append_tlv_string(payload, &offset, 0x05, "LONGNAME");  // Longer ticker
    append_tlv_uint8(payload, &offset, 0x06, 0x12);
    uint8_t tuid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    append_tlv(payload, &offset, 0x07, tuid, sizeof(tuid));
    uint8_t signature[64] = {0};
    memset(signature, 0x42, sizeof(signature));
    append_tlv(payload, &offset, 0x08, signature, sizeof(signature));

    buffer_t                     buf = {.ptr = payload, .size = offset, .offset = 0};
    tlv_dynamic_descriptor_out_t out = {0};

    check_signature_with_pki_IgnoreAndReturn(CHECK_SIGNATURE_WITH_PKI_SUCCESS);
    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    TEST_ASSERT_EQUAL_INT(result, TLV_DYNAMIC_DESCRIPTOR_SUCCESS);
    TEST_ASSERT_EQUAL_STRING(out.ticker, "LONGNAME");
    TEST_ASSERT_EQUAL_INT(out.magnitude, 0x12);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

void setUp(void)
{
    Mockledger_assert_internals_Init();
    Mocklcx_sha256_Init();
    Mocklcx_hash_Init();
    Mockledger_pki_Init();

    assert_exit_Ignore();
    assert_display_exit_Ignore();
    cx_sha256_init_no_throw_IgnoreAndReturn(CX_OK);
    cx_hash_update_IgnoreAndReturn(CX_OK);
    cx_hash_final_IgnoreAndReturn(CX_OK);
}

void tearDown(void)
{
    Mockledger_assert_internals_Verify();
    Mockledger_assert_internals_Destroy();
    Mocklcx_sha256_Verify();
    Mocklcx_sha256_Destroy();
    Mocklcx_hash_Verify();
    Mocklcx_hash_Destroy();
    Mockledger_pki_Verify();
    Mockledger_pki_Destroy();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_dynamic_descriptor);
    RUN_TEST(test_missing_structure_type);
    RUN_TEST(test_wrong_structure_type);
    RUN_TEST(test_missing_version_tag);
    RUN_TEST(test_missing_signature_tag);
    RUN_TEST(test_wrong_application_name);
    RUN_TEST(test_version_zero);
    RUN_TEST(test_version_too_high);
    RUN_TEST(test_signature_verification_failure);
    RUN_TEST(test_invalid_tlv_format);
    RUN_TEST(test_empty_payload);
    RUN_TEST(test_various_tickers);
    return UNITY_END();
}
