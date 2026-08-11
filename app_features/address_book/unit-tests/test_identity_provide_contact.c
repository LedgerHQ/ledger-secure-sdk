/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_identity_provide_contact.c
 * @brief Unit tests for provide_contact() — identity_provide_contact.c.
 *
 * Tests (provide_contact, P1=0x20):
 *  - Wrong STRUCTURE_TYPE value       → SWO_INCORRECT_DATA
 *  - Wrong STRUCTURE_VERSION value    → SWO_INCORRECT_DATA
 *  - Missing CONTACT_NAME             → SWO_INCORRECT_DATA
 *  - Missing SCOPE                    → SWO_INCORRECT_DATA
 *  - Missing ACCOUNT_IDENTIFIER       → SWO_INCORRECT_DATA
 *  - Missing GROUP_HANDLE             → SWO_INCORRECT_DATA
 *  - GROUP_HANDLE wrong size          → SWO_INCORRECT_DATA
 *  - Missing BLOCKCHAIN_FAMILY        → SWO_INCORRECT_DATA
 *  - FAMILY_ETHEREUM without CHAIN_ID → SWO_INCORRECT_DATA
 *  - Missing HMAC_PROOF               → SWO_INCORRECT_DATA
 *  - Missing HMAC_REST                → SWO_INCORRECT_DATA
 *  - App callback rejects             → SWO_WRONG_PARAMETER_VALUE
 *  - Valid Bitcoin payload            → SWO_SUCCESS
 *  - Valid Ethereum payload           → SWO_SUCCESS
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"
#include "Mockio.h"
#include "Mockos_address_book.h"
#include "Mockos_utils.h"
#include "Mocklcx_rng.h"

unsigned char G_io_rx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];

#include "identity.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Global mock controls ────────────────────────────────────────────────── */

static bool g_mock_provide_identity_result = true;

void setUp(void)
{
    Mockio_Init();
    Mockos_address_book_Init();
    Mockos_utils_Init();
    Mocklcx_rng_Init();
    io_send_response_buffers_IgnoreAndReturn(0);
    sys_address_book_hmac_IgnoreAndReturn(true);
    sys_address_book_hmac_verify_IgnoreAndReturn(true);
    is_printable_string_IgnoreAndReturn(true);
    cx_rng_no_throw_Ignore();
    g_mock_provide_identity_result = true;
}

void tearDown(void)
{
    Mockio_Verify();
    Mockio_Destroy();
    Mockos_address_book_Verify();
    Mockos_address_book_Destroy();
    Mockos_utils_Verify();
    Mockos_utils_Destroy();
    Mocklcx_rng_Verify();
    Mocklcx_rng_Destroy();
}

bool handle_provide_identity(const identity_t *contact)
{
    (void) contact;
    return g_mock_provide_identity_result;
}

/* ── Payload builders ────────────────────────────────────────────────────── */

static size_t build_provide_contact(uint8_t    *buf,
                                    size_t      buf_size,
                                    uint8_t     struct_type,
                                    uint8_t     struct_version,
                                    const char *contact_name,
                                    const char *scope,
                                    bool        include_identifier,
                                    bool        include_group_handle,
                                    uint8_t     group_handle_len,
                                    bool        include_blockchain_family,
                                    uint8_t     family,
                                    bool        include_chain_id,
                                    bool        include_hmac_proof,
                                    bool        include_hmac_rest)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, struct_type);
    tlv_u8(buf, &off, 0x02, struct_version);

    if (contact_name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) contact_name, (uint8_t) strlen(contact_name));
    }
    if (scope) {
        tlv_append(buf, &off, 0xf1, (const uint8_t *) scope, (uint8_t) strlen(scope));
    }
    if (include_identifier) {
        tlv_append(buf, &off, 0xf2, DUMMY_IDENTIFIER, sizeof(DUMMY_IDENTIFIER));
    }
    if (include_group_handle) {
        tlv_append(buf, &off, 0xf6, ZERO_64, group_handle_len);
    }
    if (include_blockchain_family) {
        tlv_u8(buf, &off, 0x51, family);
    }
    if (include_chain_id) {
        tlv_append(buf, &off, 0x23, ETH_CHAIN_ID_1, sizeof(ETH_CHAIN_ID_1));
    }
    if (include_hmac_proof) {
        tlv_append(buf, &off, 0x29, ZERO_32, sizeof(ZERO_32));
    }
    if (include_hmac_rest) {
        tlv_append(buf, &off, 0xf7, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_provide_contact_btc(uint8_t *buf, size_t buf_size)
{
    return build_provide_contact(buf,
                                 buf_size,
                                 0x33,
                                 0x01,
                                 "Alice",
                                 "Bitcoin",
                                 true,
                                 true,
                                 64,
                                 true,
                                 0x00,
                                 false,
                                 true,
                                 true);
}

static size_t build_valid_provide_contact_eth(uint8_t *buf, size_t buf_size)
{
    return build_provide_contact(buf,
                                 buf_size,
                                 0x33,
                                 0x01,
                                 "Vitalik",
                                 "Ethereum",
                                 true,
                                 true,
                                 64,
                                 true,
                                 0x01,
                                 true,
                                 true,
                                 true);
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_pc_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0xFF,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_wrong_struct_version(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0xFF,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_contact_name(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(
        buf, sizeof(buf), 0x33, 0x01, NULL, "BTC", true, true, 64, true, 0x00, false, true, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_scope(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(
        buf, sizeof(buf), 0x33, 0x01, "Alice", NULL, true, true, 64, true, 0x00, false, true, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_identifier(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       false,
                                       true,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_group_handle(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       false,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_group_handle_wrong_size(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       32,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_blockchain_family(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       false,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Vitalik",
                                       "Ethereum",
                                       true,
                                       true,
                                       64,
                                       true,
                                       0x01,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_hmac_proof(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       false,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_hmac_rest(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_app_callback_rejects(void)
{
    uint8_t buf[512];
    size_t  len                    = build_valid_provide_contact_btc(buf, sizeof(buf));
    g_mock_provide_identity_result = false;
    TEST_ASSERT_EQUAL_INT(SWO_WRONG_PARAMETER_VALUE, provide_contact(buf, len));
}

static void test_pc_success_bitcoin(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_contact_btc(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, provide_contact(buf, len));
}

static void test_pc_success_ethereum(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_contact_eth(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, provide_contact(buf, len));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pc_wrong_struct_type);
    RUN_TEST(test_pc_wrong_struct_version);
    RUN_TEST(test_pc_missing_contact_name);
    RUN_TEST(test_pc_missing_scope);
    RUN_TEST(test_pc_missing_identifier);
    RUN_TEST(test_pc_missing_group_handle);
    RUN_TEST(test_pc_group_handle_wrong_size);
    RUN_TEST(test_pc_missing_blockchain_family);
    RUN_TEST(test_pc_ethereum_missing_chain_id);
    RUN_TEST(test_pc_missing_hmac_proof);
    RUN_TEST(test_pc_missing_hmac_rest);
    RUN_TEST(test_pc_app_callback_rejects);
    RUN_TEST(test_pc_success_bitcoin);
    RUN_TEST(test_pc_success_ethereum);
    return UNITY_END();
}
