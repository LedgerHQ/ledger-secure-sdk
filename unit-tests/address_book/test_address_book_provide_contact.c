/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_address_book_provide_contact.c
 * @brief Unit tests for provide_contact() and provide_ledger_account_contact().
 *
 * Both functions are NBGL-free (fully synchronous) and can be tested end-to-end
 * with mocked OS crypto syscalls.
 *
 * Tests for provide_contact() (P1=0x20):
 *  - Wrong STRUCTURE_TYPE value       → SWO_INCORRECT_DATA
 *  - Wrong STRUCTURE_VERSION value    → SWO_INCORRECT_DATA
 *  - Missing CONTACT_NAME             → SWO_INCORRECT_DATA
 *  - Missing SCOPE                    → SWO_INCORRECT_DATA
 *  - Missing ACCOUNT_IDENTIFIER       → SWO_INCORRECT_DATA
 *  - Missing GROUP_HANDLE             → SWO_INCORRECT_DATA
 *  - GROUP_HANDLE wrong size          → SWO_INCORRECT_DATA
 *  - Missing DERIVATION_PATH          → SWO_SUCCESS (optional, silently ignored)
 *  - Missing BLOCKCHAIN_FAMILY        → SWO_INCORRECT_DATA
 *  - FAMILY_ETHEREUM without CHAIN_ID → SWO_INCORRECT_DATA
 *  - Missing HMAC_PROOF               → SWO_INCORRECT_DATA
 *  - Missing HMAC_REST                → SWO_INCORRECT_DATA
 *  - HMAC verify failure              → SWO_SECURITY_CONDITION_NOT_SATISFIED
 *  - App callback rejects             → SWO_WRONG_PARAMETER_VALUE
 *  - Valid Bitcoin payload            → SWO_SUCCESS
 *  - Valid Ethereum payload           → SWO_SUCCESS
 *
 * Tests for provide_ledger_account_contact() (P1=0x21):
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

#include "identity.h"
#include "ledger_account.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Global mock controls ────────────────────────────────────────────────── */

/* Local app entrypoint stubs — configurable return values */
static bool g_mock_provide_identity_result = true;
static bool g_mock_provide_la_result       = true;

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
    g_mock_provide_la_result       = true;
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

bool handle_provide_ledger_account(const ledger_account_t *account)
{
    (void) account;
    return g_mock_provide_la_result;
}

/* ── Payload builders ────────────────────────────────────────────────────── */

/* Build a full valid provide_contact TLV payload.
 * Pass family=FAMILY_BITCOIN (0x00) for non-Ethereum (no chain_id).
 * Pass family=FAMILY_ETHEREUM (0x01) and chain_id!=NULL to include chain_id.
 * Returns total payload length. */
static size_t build_provide_contact(uint8_t    *buf,
                                    size_t      buf_size,
                                    uint8_t     struct_type,
                                    uint8_t     struct_version,
                                    const char *contact_name,
                                    const char *scope,
                                    bool        include_identifier,
                                    bool        include_group_handle,
                                    uint8_t     group_handle_len,
                                    bool        include_deriv_path,
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
    if (include_deriv_path) {
        tlv_append(buf, &off, 0x69, BIP32_ETH_PATH, sizeof(BIP32_ETH_PATH));
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

/* Build a complete valid Bitcoin provide_contact payload into buf, return length. */
static size_t build_valid_provide_contact_btc(uint8_t *buf, size_t buf_size)
{
    return build_provide_contact(buf,
                                 buf_size,
                                 0x33, /* TYPE_PROVIDE_CONTACT */
                                 0x01, /* STRUCT_VERSION */
                                 "Alice",
                                 "Bitcoin",
                                 true,  /* identifier */
                                 true,  /* group_handle */
                                 64,    /* GROUP_HANDLE_SIZE */
                                 true,  /* deriv_path */
                                 true,  /* blockchain_family */
                                 0x00,  /* FAMILY_BITCOIN */
                                 false, /* no chain_id */
                                 true,  /* hmac_proof */
                                 true); /* hmac_rest */
}

/* Build a complete valid Ethereum provide_contact payload. */
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
                                 true,
                                 0x01, /* FAMILY_ETHEREUM */
                                 true, /* include chain_id */
                                 true,
                                 true);
}

/* Build a valid provide_ledger_account_contact payload. */
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

/* ── Tests: provide_contact ──────────────────────────────────────────────── */

static void test_pc_wrong_struct_type(void)
{
    uint8_t buf[512];
    /* struct_type = 0xFF instead of 0x33 */
    size_t len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0xFF,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
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
                                       0xFF, /* bad version */
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
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
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       NULL, /* no name */
                                       "BTC",
                                       true,
                                       true,
                                       64,
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_scope(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       NULL, /* no scope */
                                       true,
                                       true,
                                       64,
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
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
                                       false, /* no identifier */
                                       true,
                                       64,
                                       true,
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
                                       false, /* no group_handle */
                                       64,
                                       true,
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
    /* GROUP_HANDLE_SIZE=64 bytes expected; send 32 */
    size_t len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       32, /* wrong size */
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_missing_derivation_path(void)
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
                                       false, /* no deriv_path — optional, ignored */
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_SUCCESS, provide_contact(buf, len));
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
                                       true,
                                       false, /* no family */
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    /* FAMILY_ETHEREUM but chain_id not included */
    size_t len = build_provide_contact(buf,
                                       sizeof(buf),
                                       0x33,
                                       0x01,
                                       "Vitalik",
                                       "Ethereum",
                                       true,
                                       true,
                                       64,
                                       true,
                                       true,
                                       0x01,  /* FAMILY_ETHEREUM */
                                       false, /* missing chain_id */
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
                                       true,
                                       0x00,
                                       false,
                                       false, /* no hmac_proof */
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
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       false); /* no hmac_rest */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_contact(buf, len));
}

static void test_pc_hmac_verify_fails(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_contact_btc(buf, sizeof(buf));

    sys_address_book_hmac_verify_IgnoreAndReturn(false);
    TEST_ASSERT_EQUAL_INT(SWO_SECURITY_CONDITION_NOT_SATISFIED, provide_contact(buf, len));
}

static void test_pc_app_callback_rejects(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_contact_btc(buf, sizeof(buf));

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

/* ── Tests: provide_ledger_account_contact ───────────────────────────────── */

static void test_la_wrong_struct_type(void)
{
    uint8_t buf[512];
    /* struct_type = 0x33 (identity) instead of 0x34 (ledger account) */
    size_t len = build_provide_la_contact(
        buf, sizeof(buf), 0x33, 0x01, "MyLedger", true, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_wrong_struct_version(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(buf,
                                          sizeof(buf),
                                          0x34,
                                          0xFF, /* bad version */
                                          "MyLedger",
                                          true,
                                          true,
                                          0x00,
                                          false,
                                          true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_contact_name(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(buf,
                                          sizeof(buf),
                                          0x34,
                                          0x01,
                                          NULL, /* no name */
                                          true,
                                          true,
                                          0x00,
                                          false,
                                          true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_derivation_path(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(buf,
                                          sizeof(buf),
                                          0x34,
                                          0x01,
                                          "MyLedger",
                                          false, /* no deriv_path */
                                          true,
                                          0x00,
                                          false,
                                          true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_blockchain_family(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(buf,
                                          sizeof(buf),
                                          0x34,
                                          0x01,
                                          "MyLedger",
                                          true,
                                          false, /* no family */
                                          0x00,
                                          false,
                                          true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(buf,
                                          sizeof(buf),
                                          0x34,
                                          0x01,
                                          "EthLedger",
                                          true,
                                          true,
                                          0x01,  /* FAMILY_ETHEREUM */
                                          false, /* missing chain_id */
                                          true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_missing_hmac_proof(void)
{
    uint8_t buf[512];
    size_t  len = build_provide_la_contact(
        buf, sizeof(buf), 0x34, 0x01, "MyLedger", true, true, 0x00, false, false); /* no hmac_proof
                                                                                     */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, provide_ledger_account_contact(buf, len));
}

static void test_la_app_callback_rejects(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_provide_la_btc(buf, sizeof(buf));

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

    /* provide_contact (P1=0x20) */
    RUN_TEST(test_pc_wrong_struct_type);
    RUN_TEST(test_pc_wrong_struct_version);
    RUN_TEST(test_pc_missing_contact_name);
    RUN_TEST(test_pc_missing_scope);
    RUN_TEST(test_pc_missing_identifier);
    RUN_TEST(test_pc_missing_group_handle);
    RUN_TEST(test_pc_group_handle_wrong_size);
    RUN_TEST(test_pc_missing_derivation_path);
    RUN_TEST(test_pc_missing_blockchain_family);
    RUN_TEST(test_pc_ethereum_missing_chain_id);
    RUN_TEST(test_pc_missing_hmac_proof);
    RUN_TEST(test_pc_missing_hmac_rest);
    RUN_TEST(test_pc_hmac_verify_fails);
    RUN_TEST(test_pc_app_callback_rejects);
    RUN_TEST(test_pc_success_bitcoin);
    RUN_TEST(test_pc_success_ethereum);

    /* provide_ledger_account_contact (P1=0x21) */
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
