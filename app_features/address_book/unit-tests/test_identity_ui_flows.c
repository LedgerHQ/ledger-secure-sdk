/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_identity_ui_flows.c
 * @brief Unit tests for the four identity address-book commands that launch NBGL UI.
 *
 * Functions under test:
 *   register_identity()    identity_register.c          TYPE 0x2d
 *   edit_contact_name()    identity_edit_contact_name.c TYPE 0x2e
 *   edit_identifier()      identity_edit_identifier.c   TYPE 0x31
 *   edit_scope()           identity_edit_scope.c        TYPE 0x32
 *
 * nbgl_useCaseReviewLight() is stubbed to invoke its callback synchronously
 * with g_mock_review_choice, making confirm/reject paths reachable in tests.
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

#include "identity.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Mock controls ───────────────────────────────────────────────────────── */

static bool g_mock_ri_result = true;
static bool g_mock_ei_result = true;

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
    g_mock_ri_result     = true;
    g_mock_ei_result     = true;
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

bool handle_check_register_identity(identity_t *params)
{
    (void) params;
    return g_mock_ri_result;
}

bool handle_check_edit_identifier(const edit_identifier_t *params)
{
    (void) params;
    return g_mock_ei_result;
}

void finalize_ui_register_identity(void) {}
void finalize_ui_edit_contact_name(void) {}
void finalize_ui_edit_identifier(void) {}
void finalize_ui_edit_scope(void) {}

void on_edit_contact_name_applied(const edit_contact_name_t *e)
{
    (void) e;
}
void on_edit_identifier_applied(const edit_identifier_t *e)
{
    (void) e;
}
void on_edit_scope_applied(const edit_scope_t *e)
{
    (void) e;
}

nbgl_contentTagValue_t *get_register_identity_tagValue(uint8_t idx)
{
    (void) idx;
    return NULL;
}

nbgl_contentTagValue_t *get_edit_identifier_tagValue(uint8_t idx)
{
    (void) idx;
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 * 1. register_identity  (TYPE_REGISTER_IDENTITY = 0x2d)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_register_identity(uint8_t    *buf,
                                      size_t      buf_size,
                                      uint8_t     type,
                                      uint8_t     version,
                                      const char *name,
                                      const char *scope,
                                      bool        include_identifier,
                                      bool        include_family,
                                      uint8_t     family,
                                      bool        include_chain_id,
                                      bool        include_group_handle,
                                      bool        include_hmac_proof)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, type);
    tlv_u8(buf, &off, 0x02, version);
    if (name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) name, (uint8_t) strlen(name));
    }
    if (scope) {
        tlv_append(buf, &off, 0xf1, (const uint8_t *) scope, (uint8_t) strlen(scope));
    }
    if (include_identifier) {
        tlv_append(buf, &off, 0xf2, DUMMY_IDENTIFIER, sizeof(DUMMY_IDENTIFIER));
    }
    if (include_group_handle) {
        tlv_append(buf, &off, 0xf6, ZERO_64, GROUP_HANDLE_SIZE);
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

static size_t build_valid_register_identity(uint8_t *buf, size_t sz)
{
    return build_register_identity(
        buf, sz, 0x2d, 0x01, "Alice", "Bitcoin", true, true, 0x00, false, false, false);
}

static void test_ri_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_register_identity(
        buf, sizeof(buf), 0xFF, 0x01, "Alice", "BTC", true, true, 0x00, false, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_register_identity(
        buf, sizeof(buf), 0x2d, 0x01, "Alice", NULL, true, true, 0x00, false, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_group_handle_without_hmac_proof(void)
{
    uint8_t buf[512];
    size_t  len = build_register_identity(
        buf, sizeof(buf), 0x2d, 0x01, "Alice", "BTC", true, true, 0x00, false, true, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_register_identity(
        buf, sizeof(buf), 0x2d, 0x01, "Alice", "Ethereum", true, true, 0x01, false, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_app_rejects(void)
{
    g_mock_ri_result = false;
    uint8_t buf[512];
    size_t  len = build_valid_register_identity(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_WRONG_PARAMETER_VALUE, register_identity(buf, len));
}

static void test_ri_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_register_identity(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, register_identity(buf, len));
}

static void test_ri_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_register_identity(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, register_identity(buf, len));
}

/* ══════════════════════════════════════════════════════════════════════════
 * 2. edit_contact_name  (TYPE_EDIT_CONTACT_NAME = 0x2e)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_edit_contact_name(uint8_t    *buf,
                                      size_t      buf_size,
                                      uint8_t     type,
                                      uint8_t     version,
                                      const char *new_name,
                                      const char *prev_name,
                                      bool        include_group_handle,
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
    if (include_group_handle) {
        tlv_append(buf, &off, 0xf6, ZERO_64, GROUP_HANDLE_SIZE);
    }
    if (include_hmac_proof) {
        tlv_append(buf, &off, 0x29, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_edit_contact_name(uint8_t *buf, size_t sz)
{
    return build_edit_contact_name(buf, sz, 0x2e, 0x01, "Bob", "Alice", true, true);
}

static void test_ecn_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_contact_name(buf, sizeof(buf), 0xFF, 0x01, "Bob", "Alice", true, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_contact_name(buf, len));
}

static void test_ecn_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_contact_name(buf, sizeof(buf), 0x2e, 0x01, "Bob", NULL, true, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_contact_name(buf, len));
}

static void test_ecn_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_edit_contact_name(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_contact_name(buf, len));
}

static void test_ecn_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_contact_name(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_contact_name(buf, len));
}

/* ══════════════════════════════════════════════════════════════════════════
 * 3. edit_identifier  (TYPE_EDIT_IDENTIFIER = 0x31)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_edit_identifier(uint8_t    *buf,
                                    size_t      buf_size,
                                    uint8_t     type,
                                    uint8_t     version,
                                    const char *name,
                                    const char *scope,
                                    bool        include_new_identifier,
                                    bool        include_prev_identifier,
                                    bool        include_group_handle,
                                    bool        include_family,
                                    uint8_t     family,
                                    bool        include_chain_id,
                                    bool        include_hmac_proof,
                                    bool        include_hmac_rest)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, type);
    tlv_u8(buf, &off, 0x02, version);
    if (name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) name, (uint8_t) strlen(name));
    }
    if (scope) {
        tlv_append(buf, &off, 0xf1, (const uint8_t *) scope, (uint8_t) strlen(scope));
    }
    if (include_new_identifier) {
        tlv_append(buf, &off, 0xf2, DUMMY_IDENTIFIER, sizeof(DUMMY_IDENTIFIER));
    }
    if (include_prev_identifier) {
        tlv_append(buf, &off, 0xf4, DUMMY_IDENTIFIER, sizeof(DUMMY_IDENTIFIER));
    }
    if (include_group_handle) {
        tlv_append(buf, &off, 0xf6, ZERO_64, GROUP_HANDLE_SIZE);
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
    if (include_hmac_rest) {
        tlv_append(buf, &off, 0xf7, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_edit_identifier(uint8_t *buf, size_t sz)
{
    return build_edit_identifier(
        buf, sz, 0x31, 0x01, "Alice", "Bitcoin", true, true, true, true, 0x00, false, true, true);
}

static void test_ei_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_identifier(buf,
                                       sizeof(buf),
                                       0xFF,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_identifier(buf, len));
}

static void test_ei_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_identifier(buf,
                                       sizeof(buf),
                                       0x31,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_identifier(buf, len));
}

static void test_ei_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_identifier(buf,
                                       sizeof(buf),
                                       0x31,
                                       0x01,
                                       "Alice",
                                       "Ethereum",
                                       true,
                                       true,
                                       true,
                                       true,
                                       0x01,
                                       false,
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_identifier(buf, len));
}

static void test_ei_app_rejects(void)
{
    g_mock_ei_result = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_identifier(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_WRONG_PARAMETER_VALUE, edit_identifier(buf, len));
}

static void test_ei_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_edit_identifier(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_identifier(buf, len));
}

static void test_ei_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_identifier(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_identifier(buf, len));
}

/* ══════════════════════════════════════════════════════════════════════════
 * 4. edit_scope  (TYPE_EDIT_SCOPE = 0x32)
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_edit_scope(uint8_t    *buf,
                               size_t      buf_size,
                               uint8_t     type,
                               uint8_t     version,
                               const char *name,
                               const char *new_scope,
                               const char *prev_scope,
                               bool        include_identifier,
                               bool        include_group_handle,
                               bool        include_family,
                               uint8_t     family,
                               bool        include_chain_id,
                               bool        include_hmac_proof,
                               bool        include_hmac_rest)
{
    size_t off = 0;
    (void) buf_size;

    tlv_u8(buf, &off, 0x01, type);
    tlv_u8(buf, &off, 0x02, version);
    if (name) {
        tlv_append(buf, &off, 0xf0, (const uint8_t *) name, (uint8_t) strlen(name));
    }
    if (new_scope) {
        tlv_append(buf, &off, 0xf1, (const uint8_t *) new_scope, (uint8_t) strlen(new_scope));
    }
    if (include_identifier) {
        tlv_append(buf, &off, 0xf2, DUMMY_IDENTIFIER, sizeof(DUMMY_IDENTIFIER));
    }
    if (prev_scope) {
        tlv_append(buf, &off, 0xf5, (const uint8_t *) prev_scope, (uint8_t) strlen(prev_scope));
    }
    if (include_group_handle) {
        tlv_append(buf, &off, 0xf6, ZERO_64, GROUP_HANDLE_SIZE);
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
    if (include_hmac_rest) {
        tlv_append(buf, &off, 0xf7, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_edit_scope(uint8_t *buf, size_t sz)
{
    return build_edit_scope(buf,
                            sz,
                            0x32,
                            0x01,
                            "Alice",
                            "Bitcoin Wallet",
                            "Personal Bitcoin",
                            true,
                            true,
                            true,
                            0x00,
                            false,
                            true,
                            true);
}

static void test_es_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_scope(buf,
                                  sizeof(buf),
                                  0xFF,
                                  0x01,
                                  "Alice",
                                  "New",
                                  "Old",
                                  true,
                                  true,
                                  true,
                                  0x00,
                                  false,
                                  true,
                                  true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_scope(buf, len));
}

static void test_es_missing_mandatory_field(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_scope(buf,
                                  sizeof(buf),
                                  0x32,
                                  0x01,
                                  "Alice",
                                  "New",
                                  "Old",
                                  true,
                                  true,
                                  true,
                                  0x00,
                                  false,
                                  true,
                                  false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_scope(buf, len));
}

static void test_es_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_scope(buf,
                                  sizeof(buf),
                                  0x32,
                                  0x01,
                                  "Alice",
                                  "Ethereum Wallet",
                                  "Personal Ethereum",
                                  true,
                                  true,
                                  true,
                                  0x01,
                                  false,
                                  true,
                                  true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_scope(buf, len));
}

static void test_es_success(void)
{
    uint8_t buf[512];
    size_t  len = build_valid_edit_scope(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_scope(buf, len));
}

static void test_es_review_rejected(void)
{
    g_mock_review_choice = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_scope(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_NO_RESPONSE, edit_scope(buf, len));
}

/* ── Test runner ─────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    /* register_identity */
    RUN_TEST(test_ri_wrong_struct_type);
    RUN_TEST(test_ri_missing_mandatory_field);
    RUN_TEST(test_ri_ethereum_missing_chain_id);
    RUN_TEST(test_ri_group_handle_without_hmac_proof);
    RUN_TEST(test_ri_app_rejects);
    RUN_TEST(test_ri_success);
    RUN_TEST(test_ri_review_rejected);

    /* edit_contact_name */
    RUN_TEST(test_ecn_wrong_struct_type);
    RUN_TEST(test_ecn_missing_mandatory_field);
    RUN_TEST(test_ecn_success);
    RUN_TEST(test_ecn_review_rejected);

    /* edit_identifier */
    RUN_TEST(test_ei_wrong_struct_type);
    RUN_TEST(test_ei_missing_mandatory_field);
    RUN_TEST(test_ei_ethereum_missing_chain_id);
    RUN_TEST(test_ei_app_rejects);
    RUN_TEST(test_ei_success);
    RUN_TEST(test_ei_review_rejected);

    /* edit_scope */
    RUN_TEST(test_es_wrong_struct_type);
    RUN_TEST(test_es_missing_mandatory_field);
    RUN_TEST(test_es_ethereum_missing_chain_id);
    RUN_TEST(test_es_success);
    RUN_TEST(test_es_review_rejected);

    return UNITY_END();
}
