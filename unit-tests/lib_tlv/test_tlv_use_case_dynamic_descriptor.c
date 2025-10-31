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

#include "tlv_use_case_dynamic_descriptor.h"
#include "ledger_pki.h"
#include "buffer.h"

/* -------------------------------------------------------------------------- */
/* Mock definitions                                                           */
/* -------------------------------------------------------------------------- */

/* Mock PKI verification result */
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

static void append_tlv_uint32(uint8_t *buffer, size_t *offset, uint8_t tag, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    append_tlv(buffer, offset, tag, bytes, 4);
}

static void append_tlv_string(uint8_t *buffer, size_t *offset, uint8_t tag, const char *str)
{
    append_tlv(buffer, offset, tag, (const uint8_t *) str, strlen(str));
}

/* -------------------------------------------------------------------------- */
/* Test: Valid complete dynamic descriptor                                    */
/* -------------------------------------------------------------------------- */

static void test_valid_dynamic_descriptor(void **state)
{
    (void) state;

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

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_SUCCESS;

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_SUCCESS);
    assert_int_equal(out.version, 1);
    assert_int_equal(out.coin_type, 0x8000003C);
    assert_string_equal(out.ticker, "BTC");
    assert_int_equal(out.magnitude, 8);
    assert_int_equal(out.TUID.size, 4);
    assert_memory_equal(out.TUID.ptr, tuid, 4);
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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong structure type                                                 */
/* -------------------------------------------------------------------------- */

static void test_wrong_structure_type(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_WRONG_TYPE);
}

/* -------------------------------------------------------------------------- */
/* Test: Missing required fields                                              */
/* -------------------------------------------------------------------------- */

static void test_missing_version_tag(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_TAG);
}

static void test_missing_signature_tag(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Wrong application name                                               */
/* -------------------------------------------------------------------------- */

static void test_wrong_application_name(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_WRONG_APPLICATION_NAME);
}

/* -------------------------------------------------------------------------- */
/* Test: Unsupported version                                                  */
/* -------------------------------------------------------------------------- */

static void test_version_zero(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_UNKNOWN_VERSION);
}

static void test_version_too_high(void **state)
{
    (void) state;

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

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_UNKNOWN_VERSION);
}

/* -------------------------------------------------------------------------- */
/* Test: Signature verification failure                                       */
/* -------------------------------------------------------------------------- */

static void test_signature_verification_failure(void **state)
{
    (void) state;

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

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_WRONG_SIGNATURE;

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    assert_true((result & TLV_DYNAMIC_DESCRIPTOR_SIGNATURE_ERROR) != 0);
}

/* -------------------------------------------------------------------------- */
/* Test: Malformed TLV parsing                                                */
/* -------------------------------------------------------------------------- */

static void test_invalid_tlv_format(void **state)
{
    (void) state;

    uint8_t                      payload[10] = {0x01, 0xFF, 0x90};  // Length exceeds buffer
    buffer_t                     buf         = {.ptr = payload, .size = 3, .offset = 0};
    tlv_dynamic_descriptor_out_t out         = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_PARSING_ERROR);
}

/* -------------------------------------------------------------------------- */
/* Test: Empty payload                                                        */
/* -------------------------------------------------------------------------- */

static void test_empty_payload(void **state)
{
    (void) state;

    uint8_t                      payload[1] = {0};
    buffer_t                     buf        = {.ptr = payload, .size = 0, .offset = 0};
    tlv_dynamic_descriptor_out_t out        = {0};

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_MISSING_STRUCTURE_TAG);
}

/* -------------------------------------------------------------------------- */
/* Test: Ticker and coin type validation                                      */
/* -------------------------------------------------------------------------- */

static void test_various_tickers(void **state)
{
    (void) state;

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

    mock_pki_result = CHECK_SIGNATURE_WITH_PKI_SUCCESS;

    tlv_dynamic_descriptor_status_t result = tlv_use_case_dynamic_descriptor(&buf, &out);

    assert_int_equal(result, TLV_DYNAMIC_DESCRIPTOR_SUCCESS);
    assert_string_equal(out.ticker, "LONGNAME");
    assert_int_equal(out.magnitude, 0x12);
}

/* -------------------------------------------------------------------------- */
/* Test suite entry point                                                     */
/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_dynamic_descriptor),
        cmocka_unit_test(test_missing_structure_type),
        cmocka_unit_test(test_wrong_structure_type),
        cmocka_unit_test(test_missing_version_tag),
        cmocka_unit_test(test_missing_signature_tag),
        cmocka_unit_test(test_wrong_application_name),
        cmocka_unit_test(test_version_zero),
        cmocka_unit_test(test_version_too_high),
        cmocka_unit_test(test_signature_verification_failure),
        cmocka_unit_test(test_invalid_tlv_format),
        cmocka_unit_test(test_empty_payload),
        cmocka_unit_test(test_various_tickers),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
