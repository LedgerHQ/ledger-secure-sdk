/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_ledger_account_ui_flows.c
 * @brief Unit tests for the two ledger-account address-book commands that launch NBGL UI.
 *
 * Functions under test:
 *   register_ledger_account()  ledger_account_register.c  TYPE 0x2f
 *   edit_ledger_account()      ledger_account_edit.c      TYPE 0x30
 *
 * display_register_ledger_account_review() and nbgl_useCaseReviewLight() are
 * stubbed to invoke their callbacks synchronously with g_mock_review_choice.
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

#include "nbgl_types.h"

typedef uint32_t nbgl_operationType_t;
typedef void (*nbgl_callback_t)(void);
typedef void (*nbgl_choiceCallback_t)(bool confirm);
typedef struct nbgl_contentTagValue_s     nbgl_contentTagValue_t;
typedef struct nbgl_contentTagValueList_s nbgl_contentTagValueList_t;

unsigned char G_io_rx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];

const nbgl_icon_details_t C_Address_Book_64px = {0};

bool g_mock_review_choice = true;

void nbgl_useCaseReviewLight(nbgl_operationType_t              operationType,
                             const nbgl_contentTagValueList_t *tagValueList,
                             const nbgl_icon_details_t        *icon,
                             const char                       *reviewTitle,
                             const char                       *reviewSubTitle,
                             const char                       *confirmText,
                             nbgl_choiceCallback_t             callback)
{
    (void) operationType;
    (void) tagValueList;
    (void) icon;
    (void) reviewTitle;
    (void) reviewSubTitle;
    (void) confirmText;
    callback(g_mock_review_choice);
}

void nbgl_useCaseStatus(const char *message, bool isSuccess, nbgl_callback_t quitCallback)
{
    (void) message;
    (void) isSuccess;
    (void) quitCallback;
}

#include "ledger_account.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Mock controls ───────────────────────────────────────────────────────── */

static bool g_mock_rla_result = true;
static bool g_mock_ela_result = true;

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
    g_mock_review_choice = true;
    g_mock_rla_result    = true;
    g_mock_ela_result    = true;
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

/* ── App-entrypoint stubs ────────────────────────────────────────────────── */

bool handle_check_register_ledger_account(ledger_account_t *params)
{
    (void) params;
    return g_mock_rla_result;
}

bool handle_check_edit_ledger_account(edit_ledger_account_t *params)
{
    (void) params;
    return g_mock_ela_result;
}

void display_register_ledger_account_review(nbgl_choiceCallback_t callback)
{
    callback(g_mock_review_choice);
}

void finalize_ui_ledger_account(void) {}

void on_edit_ledger_account_applied(const edit_ledger_account_t *e)
{
    (void) e;
}

/* ══════════════════════════════════════════════════════════════════════════
 * 5. register_ledger_account  (TYPE_REGISTER_LEDGER_ACCOUNT = 0x2f)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_register_ledger_account(uint8_t    *buf,
                                            size_t      buf_size,
                                            uint8_t     type,
                                            uint8_t     version,
                                            const char *name,
                                            bool        include_deriv,
                                            bool        include_family,
                                            uint8_t     family,
                                            bool        include_chain_id)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, type);
    tlv_u8(buf, &off, 0x02, version);
    if (name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) name, (uint8_t) strlen(name));
    }
    if (include_deriv) {
        tlv_append(buf, &off, 0x69, BIP32_ETH_PATH, sizeof(BIP32_ETH_PATH));
    }
    if (include_family) {
        tlv_u8(buf, &off, 0x51, family);
    }
    if (include_chain_id) {
        tlv_append(buf, &off, 0x23, ETH_CHAIN_ID_1, sizeof(ETH_CHAIN_ID_1));
    }
    return off;
}

static size_t build_valid_register_ledger_account(uint8_t *buf, size_t sz)
{
    return build_register_ledger_account(buf, sz, 0x2f, 0x01, "MyLedger", true, true, 0x00, false);
}

static void test_rla_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_register_ledger_account(
        buf, sizeof(buf), 0xFF, 0x01, "MyLedger", true, true, 0x00, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_ledger_account(buf, len));
}

static void test_rla_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_register_ledger_account(
        buf, sizeof(buf), 0x2f, 0x01, "MyLedger", false, true, 0x00, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_ledger_account(buf, len));
}

static void test_rla_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_register_ledger_account(
        buf, sizeof(buf), 0x2f, 0x01, "EthLedger", true, true, 0x01, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_ledger_account(buf, len));
}

static void test_rla_app_rejects(void)
{
    g_mock_rla_result = false;
    uint8_t buf[512];
    size_t  len = build_valid_register_ledger_account(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_WRONG_PARAMETER_VALUE, register_ledger_account(buf, len));
}

static void test_rla_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_register_ledger_account(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, register_ledger_account(buf, len));
}

static void test_rla_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_register_ledger_account(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, register_ledger_account(buf, len));
}

/* ══════════════════════════════════════════════════════════════════════════
 * 6. edit_ledger_account  (TYPE_EDIT_LEDGER_ACCOUNT = 0x30)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_edit_ledger_account(uint8_t    *buf,
                                        size_t      buf_size,
                                        uint8_t     type,
                                        uint8_t     version,
                                        const char *new_name,
                                        const char *prev_name,
                                        bool        include_deriv,
                                        bool        include_family,
                                        uint8_t     family,
                                        bool        include_chain_id,
                                        bool        include_hmac_proof)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, type);
    tlv_u8(buf, &off, 0x02, version);
    if (new_name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) new_name, (uint8_t) strlen(new_name));
    }
    if (prev_name) {
        tlv_append(buf, &off, 0xf3, (const uint8_t *) prev_name, (uint8_t) strlen(prev_name));
    }
    if (include_deriv) {
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

static size_t build_valid_edit_ledger_account(uint8_t *buf, size_t sz)
{
    return build_edit_ledger_account(
        buf, sz, 0x30, 0x01, "NewLedger", "OldLedger", true, true, 0x00, false, true);
}

static void test_ela_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_ledger_account(
        buf, sizeof(buf), 0xFF, 0x01, "New", "Old", true, true, 0x00, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_ledger_account(
        buf, sizeof(buf), 0x30, 0x01, "New", "Old", true, true, 0x00, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_ledger_account(
        buf, sizeof(buf), 0x30, 0x01, "New", "Old", true, true, 0x01, false, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_app_rejects(void)
{
    g_mock_ela_result = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_ledger_account(buf, sizeof(buf));
    /* edit_ledger_account returns SWO_INCORRECT_DATA when callback rejects */
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_edit_ledger_account(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_ledger_account(buf, len));
}

static void test_ela_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_ledger_account(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_ledger_account(buf, len));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* register_ledger_account */
    RUN_TEST(test_rla_wrong_struct_type);
    RUN_TEST(test_rla_missing_mandatory_field);
    RUN_TEST(test_rla_ethereum_missing_chain_id);
    RUN_TEST(test_rla_app_rejects);
    RUN_TEST(test_rla_success);
    RUN_TEST(test_rla_review_rejected);

    /* edit_ledger_account */
    RUN_TEST(test_ela_wrong_struct_type);
    RUN_TEST(test_ela_missing_mandatory_field);
    RUN_TEST(test_ela_ethereum_missing_chain_id);
    RUN_TEST(test_ela_app_rejects);
    RUN_TEST(test_ela_success);
    RUN_TEST(test_ela_review_rejected);

    return UNITY_END();
}
