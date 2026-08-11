/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_ledger_account_provide_contact.c
 * @brief Unit tests for provide_ledger_account_contact() — ledger_account_provide_contact.c.
 *
 * Tests (provide_ledger_account_contact, P1=0x21):
 *  - Wrong STRUCTURE_TYPE             → SWO_INCORRECT_DATA
 *  - Wrong STRUCTURE_VERSION          → SWO_INCORRECT_DATA
 *  - Missing CONTACT_NAME             → SWO_INCORRECT_DATA
 *  - Missing DERIVATION_PATH          → SWO_INCORRECT_DATA
 *  - Missing BLOCKCHAIN_FAMILY        → SWO_INCORRECT_DATA
 *  - FAMILY_ETHEREUM without CHAIN_ID → SWO_INCORRECT_DATA
 *  - Missing HMAC_PROOF               → SWO_INCORRECT_DATA
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

#include "ledger_account.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Global mock controls ────────────────────────────────────────────────── */

static bool g_mock_provide_la_result = true;

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
    g_mock_provide_la_result = true;
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

bool handle_provide_ledger_account(const ledger_account_t *account)
{
    (void) account;
    return g_mock_provide_la_result;
}

/* ── Payload builders ────────────────────────────────────────────────────── */

static size_t build_provide_la_contact(uint8_t    *buf,
                                       size_t      buf_size,
                                       uint8_t     struct_type,
                                       uint8_t     struct_version,
                                       const char *name,
                                       bool        include_deriv_path,
                                       bool        include_family,
                                       uint8_t     family,
                                       bool        include_chain_id,
                                       bool        include_hmac_proof)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, struct_type);
    tlv_u8(buf, &off, 0x02, struct_version);
    if (name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) name, (uint8_t) strlen(name));
    }
    if (include_deriv_path) {
        tlv_append(buf, &off, 0x69, BIP32_ETH_PATH, sizeof(BIP32_ETH_PATH));
    }
    if (include_family) {
        tlv_u8(buf, &off, 0x51, family);
    }
    if (include_chain_id) {
        tlv_append(buf, &off, 0x23, ETH_CHAIN_ID_1, sizeof(ETH_CHAIN_ID_1));
    }
    if (include_hmac_proof) {
        tlv_append(buf, &off, 0x29, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_provide_la_btc(uint8_t *buf, size_t buf_size)
{
    return build_provide_la_contact(
        buf, buf_size, 0x34, 0x01, "MyLedger", true, true, 0x00, false, true);
}

static size_t build_valid_provide_la_eth(uint8_t *buf, size_t buf_size)
{
    return build_provide_la_contact(
        buf, buf_size, 0x34, 0x01, "EthLedger", true, true, 0x01, true, true);
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_la_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x33, 0x01, "MyLedger", true, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_wrong_struct_version(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0xFF, "MyLedger", true, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_contact_name(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, NULL, true, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_derivation_path(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, "MyLedger", false, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_blockchain_family(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, "MyLedger", true, false, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, "EthLedger", true, true, 0x01, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_hmac_proof(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, "MyLedger", true, true, 0x00, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_app_callback_rejects(void)
{
    uint8_t buf[512];
    size_t  len              = build_valid_provide_la_btc(buf, sizeof(buf));
    g_mock_provide_la_result = false;
    TEST_ASSERT_EQUAL_INT(SWO_WRONG_PARAMETER_VALUE, provide_ledger_account_contact(buf, len));
}

static void test_la_success_bitcoin(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_la_btc(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, provide_ledger_account_contact(buf, len));
}

static void test_la_success_ethereum(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_la_eth(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, provide_ledger_account_contact(buf, len));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_la_wrong_struct_type);
    RUN_TEST(test_la_wrong_struct_version);
    RUN_TEST(test_la_missing_contact_name);
    RUN_TEST(test_la_missing_derivation_path);
    RUN_TEST(test_la_missing_blockchain_family);
    RUN_TEST(test_la_ethereum_missing_chain_id);
    RUN_TEST(test_la_missing_hmac_proof);
    RUN_TEST(test_la_app_callback_rejects);
    RUN_TEST(test_la_success_bitcoin);
    RUN_TEST(test_la_success_ethereum);
    return UNITY_END();
}
