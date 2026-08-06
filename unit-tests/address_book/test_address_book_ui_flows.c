/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *
 *****************************************************************************/

/**
 * @file test_address_book_ui_flows.c
 * @brief Unit tests for the six address-book commands that launch a NBGL
 *        confirmation UI before sending a response.
 *
 * nbgl_useCaseReviewLight() and display_register_ledger_account_review() are
 * stubbed to invoke their callback synchronously with g_mock_review_choice
 * (true = confirm, false = reject), making the full confirm/reject paths
 * reachable in unit tests.  All error paths (bad TLV, missing field, HMAC
 * failure, app-callback rejection) return early before reaching the UI.
 *
 * Functions under test:
 *   register_identity()         identity_register.c          TYPE 0x2d
 *   edit_contact_name()         identity_edit_contact_name.c TYPE 0x2e
 *   edit_identifier()           identity_edit_identifier.c   TYPE 0x31
 *   edit_scope()                identity_edit_scope.c        TYPE 0x32
 *   register_ledger_account()   ledger_account_register.c    TYPE 0x2f
 *   edit_ledger_account()       ledger_account_edit.c        TYPE 0x30
 *
 * Stubs provided here:
 *   handle_check_register_identity()       — configurable via g_mock_ri_result
 *   handle_check_edit_identifier()         — configurable via g_mock_ei_result
 *   handle_check_register_ledger_account() — configurable via g_mock_rla_result
 *   handle_check_edit_ledger_account()     — configurable via g_mock_ela_result
 *   display_register_ledger_account_review() — calls callback(g_mock_review_choice)
 *   LARGE_ADDRESS_BOOK_ICON                — zero-initialised icon
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

#include "nbgl_types.h" /* nbgl_icon_details_t */

/* Types from nbgl_use_case.h / nbgl_content.h — forward-declared to avoid
 * pulling in the full nbgl_use_case.h header. */
typedef uint32_t nbgl_operationType_t;
typedef void (*nbgl_callback_t)(void);
typedef void (*nbgl_choiceCallback_t)(bool confirm);
typedef struct nbgl_contentTagValue_s     nbgl_contentTagValue_t;
typedef struct nbgl_contentTagValueList_s nbgl_contentTagValueList_t;

/* IO buffers referenced by address_book_crypto.c. */
unsigned char G_io_rx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];

/* Icon stub: LARGE_ADDRESS_BOOK_ICON expands to C_Address_Book_64px on Flex/Stax. */
const nbgl_icon_details_t C_Address_Book_64px = {0};

/* Controlled by tests: true = user confirms, false = user rejects. */
bool g_mock_review_choice = true;

/* NBGL stubs — nbgl_use_case.h is NOT in MOCK_HEADERS so we provide custom
 * implementations that invoke the callback synchronously, allowing the full
 * confirm/reject code paths to be exercised in unit tests. */

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
#include "ledger_account.h"
#include "status_words.h"
#include "tlv_test_helpers.h"

/* ── Mock controls ───────────────────────────────────────────────────────── */
/* Local app-entrypoint stubs */
static bool g_mock_ri_result  = true; /* handle_check_register_identity      */
static bool g_mock_ei_result  = true; /* handle_check_edit_identifier         */
static bool g_mock_rla_result = true; /* handle_check_register_ledger_account */
static bool g_mock_ela_result = true; /* handle_check_edit_ledger_account     */

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

/* Coin-app review entrypoint for Ledger Account flows. */
void display_register_ledger_account_review(nbgl_choiceCallback_t callback)
{
    callback(g_mock_review_choice);
}

/* ── Finalise / apply callbacks (called from review_choice) ──────────────── */
void finalize_ui_register_identity(void) {}
void finalize_ui_edit_contact_name(void) {}
void finalize_ui_edit_identifier(void) {}
void finalize_ui_edit_scope(void) {}
void finalize_ui_ledger_account(void) {}

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
void on_edit_ledger_account_applied(const edit_ledger_account_t *e)
{
    (void) e;
}

/* Dynamic tag-value callbacks for paginated NBGL review — unused in unit tests. */
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
 *
 * Tags: 0x01 type, 0x02 version, 0xf0 name, 0xf1 scope, 0xf2 identifier,
 *       0x69 deriv_path, 0x51 family, [0x23 chain_id],
 *       [0xf6 group_handle + 0x29 hmac_proof]  ← optional pair
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_register_identity(uint8_t    *buf,
                                      size_t      buf_size,
                                      uint8_t     type,
                                      uint8_t     version,
                                      const char *name,
                                      const char *scope,
                                      bool        include_identifier,
                                      bool        include_deriv,
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

/* Valid BTC registration without the optional group_handle/hmac_proof pair */
static size_t build_valid_register_identity(uint8_t *buf, size_t sz)
{
    return build_register_identity(buf,
                                   sz,
                                   0x2d,
                                   0x01,
                                   "Alice",
                                   "Bitcoin",
                                   true,  /* identifier   */
                                   true,  /* deriv_path   */
                                   true,  /* family       */
                                   0x00,  /* FAMILY_BITCOIN */
                                   false, /* no chain_id  */
                                   false, /* no group_handle */
                                   false  /* no hmac_proof */
    );
}

static void test_ri_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len = build_register_identity(
        buf, sizeof(buf), 0xFF, 0x01, "Alice", "BTC", true, true, true, 0x00, false, false, false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_missing_mandatory_field(void)
{
    uint8_t buf[512];
    /* omit scope → verify_fields fails */
    size_t len = build_register_identity(buf,
                                         sizeof(buf),
                                         0x2d,
                                         0x01,
                                         "Alice",
                                         NULL, /* no scope */
                                         true,
                                         true,
                                         true,
                                         0x00,
                                         false,
                                         false,
                                         false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_group_handle_without_hmac_proof(void)
{
    uint8_t buf[512];
    /* group_handle present but hmac_proof absent → verify_fields rejects pair */
    size_t len = build_register_identity(buf,
                                         sizeof(buf),
                                         0x2d,
                                         0x01,
                                         "Alice",
                                         "BTC",
                                         true,
                                         true,
                                         true,
                                         0x00,
                                         false,
                                         true, /* group_handle */
                                         false /* no hmac_proof */
    );
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    /* FAMILY_ETHEREUM but chain_id omitted → verify_fields rejects */
    size_t len = build_register_identity(buf,
                                         sizeof(buf),
                                         0x2d,
                                         0x01,
                                         "Alice",
                                         "Ethereum",
                                         true,
                                         true,
                                         true,
                                         0x01,  /* FAMILY_ETHEREUM */
                                         false, /* no chain_id */
                                         false,
                                         false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_identity(buf, len));
}

static void test_ri_hmac_fails(void)
{
    sys_address_book_hmac_verify_IgnoreAndReturn(false);
    uint8_t buf[512];
    /* include group_handle + hmac_proof so the HMAC check runs */
    size_t len = build_register_identity(
        buf, sizeof(buf), 0x2d, 0x01, "Alice", "BTC", true, true, true, 0x00, false, true, true);
    TEST_ASSERT_EQUAL_INT(SWO_SECURITY_CONDITION_NOT_SATISFIED, register_identity(buf, len));
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
 *
 * Tags: 0x01, 0x02, 0xf0 new_name, 0xf3 prev_name,
 *       0xf6 group_handle, 0x69 deriv_path, 0x29 hmac_proof
 * All mandatory; no family/chain_id; no app callback.
 * ══════════════════════════════════════════════════════════════════════════ */

static size_t build_edit_contact_name(uint8_t    *buf,
                                      size_t      buf_size,
                                      uint8_t     type,
                                      uint8_t     version,
                                      const char *new_name,
                                      const char *prev_name,
                                      bool        include_group_handle,
                                      bool        include_deriv,
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
    if (include_deriv) {
        tlv_append(buf, &off, 0x69, BIP32_ETH_PATH, sizeof(BIP32_ETH_PATH));
    }
    if (include_hmac_proof) {
        tlv_append(buf, &off, 0x29, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_edit_contact_name(uint8_t *buf, size_t sz)
{
    return build_edit_contact_name(buf, sz, 0x2e, 0x01, "Bob", "Alice", true, true, true);
}

static void test_ecn_wrong_struct_type(void)
{
    uint8_t buf[512];
    size_t  len
        = build_edit_contact_name(buf, sizeof(buf), 0xFF, 0x01, "Bob", "Alice", true, true, true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_contact_name(buf, len));
}

static void test_ecn_missing_mandatory_field(void)
{
    uint8_t buf[512];
    /* omit prev_name → verify_fields fails */
    size_t len = build_edit_contact_name(buf,
                                         sizeof(buf),
                                         0x2e,
                                         0x01,
                                         "Bob",
                                         NULL, /* no prev_name */
                                         true,
                                         true,
                                         true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_contact_name(buf, len));
}

static void test_ecn_hmac_fails(void)
{
    sys_address_book_hmac_verify_IgnoreAndReturn(false);
    uint8_t buf[512];
    size_t  len = build_valid_edit_contact_name(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SECURITY_CONDITION_NOT_SATISFIED, edit_contact_name(buf, len));
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
 *
 * Tags: 0x01, 0x02, 0xf0 name, 0xf1 scope, 0xf2 new_identifier,
 *       0xf4 prev_identifier, 0xf6 group_handle, 0x69 deriv_path,
 *       0x51 family, [0x23 chain_id], 0x29 hmac_proof, 0xf7 hmac_rest
 * All non-bracketed mandatory; chain_id only for Ethereum.
 * App callback: handle_check_edit_identifier → SWO_WRONG_PARAMETER_VALUE.
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
                                    bool        include_deriv,
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
    if (include_hmac_rest) {
        tlv_append(buf, &off, 0xf7, ZERO_32, sizeof(ZERO_32));
    }
    return off;
}

static size_t build_valid_edit_identifier(uint8_t *buf, size_t sz)
{
    return build_edit_identifier(buf,
                                 sz,
                                 0x31,
                                 0x01,
                                 "Alice",
                                 "Bitcoin",
                                 true,
                                 true,
                                 true,
                                 true,
                                 true,
                                 0x00, /* FAMILY_BITCOIN, no chain_id */
                                 false,
                                 true,
                                 true);
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
    /* omit hmac_rest → verify_fields fails */
    size_t len = build_edit_identifier(buf,
                                       sizeof(buf),
                                       0x31,
                                       0x01,
                                       "Alice",
                                       "BTC",
                                       true,
                                       true,
                                       true,
                                       true,
                                       true,
                                       0x00,
                                       false,
                                       true,
                                       false /* no hmac_rest */);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_identifier(buf, len));
}

static void test_ei_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    /* FAMILY_ETHEREUM but chain_id omitted → verify_fields rejects */
    size_t len = build_edit_identifier(buf,
                                       sizeof(buf),
                                       0x31,
                                       0x01,
                                       "Alice",
                                       "Ethereum",
                                       true,
                                       true,
                                       true,
                                       true,
                                       true,
                                       0x01,  /* FAMILY_ETHEREUM */
                                       false, /* no chain_id */
                                       true,
                                       true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_identifier(buf, len));
}

static void test_ei_hmac_fails(void)
{
    sys_address_book_hmac_verify_IgnoreAndReturn(false);
    uint8_t buf[512];
    size_t  len = build_valid_edit_identifier(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SECURITY_CONDITION_NOT_SATISFIED, edit_identifier(buf, len));
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
 *
 * Tags: 0x01, 0x02, 0xf0 name, 0xf1 new_scope, 0xf2 identifier,
 *       0xf5 prev_scope, 0xf6 group_handle, 0x69 deriv_path,
 *       0x51 family, [0x23 chain_id], 0x29 hmac_proof, 0xf7 hmac_rest
 * No app callback — goes straight to UI after HMAC verification.
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
                               bool        include_deriv,
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
                            true,
                            0x00, /* FAMILY_BITCOIN */
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
    /* omit hmac_rest → verify_fields fails */
    size_t len = build_edit_scope(buf,
                                  sizeof(buf),
                                  0x32,
                                  0x01,
                                  "Alice",
                                  "New",
                                  "Old",
                                  true,
                                  true,
                                  true,
                                  true,
                                  0x00,
                                  false,
                                  true,
                                  false /* no hmac_rest */);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_scope(buf, len));
}

static void test_es_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    /* FAMILY_ETHEREUM but chain_id omitted → verify_fields rejects */
    size_t len = build_edit_scope(buf,
                                  sizeof(buf),
                                  0x32,
                                  0x01,
                                  "Alice",
                                  "Ethereum Wallet",
                                  "Personal Ethereum",
                                  true,
                                  true,
                                  true,
                                  true,
                                  0x01,  /* FAMILY_ETHEREUM */
                                  false, /* no chain_id */
                                  true,
                                  true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_scope(buf, len));
}

static void test_es_hmac_fails(void)
{
    sys_address_book_hmac_verify_IgnoreAndReturn(false);
    uint8_t buf[512];
    size_t  len = build_valid_edit_scope(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(SWO_SECURITY_CONDITION_NOT_SATISFIED, edit_scope(buf, len));
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

/* ══════════════════════════════════════════════════════════════════════════
 * 5. register_ledger_account  (TYPE_REGISTER_LEDGER_ACCOUNT = 0x2f)
 *
 * Tags: 0x01, 0x02, 0xf0 name, 0x69 deriv_path, 0x51 family,
 *       [0x23 chain_id]
 * No HMAC; app callback: handle_check_register_ledger_account → SWO_WRONG_PARAMETER_VALUE.
 * On success: display_register_ledger_account_review() then SWO_NO_RESPONSE.
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
    /* omit derivation_path → verify_fields fails */
    size_t len = build_register_ledger_account(buf,
                                               sizeof(buf),
                                               0x2f,
                                               0x01,
                                               "MyLedger",
                                               false, /* no deriv */
                                               true,
                                               0x00,
                                               false);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, register_ledger_account(buf, len));
}

static void test_rla_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_register_ledger_account(buf,
                                               sizeof(buf),
                                               0x2f,
                                               0x01,
                                               "EthLedger",
                                               true,
                                               true,
                                               0x01, /* FAMILY_ETHEREUM */
                                               false /* no chain_id */);
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
 *
 * Tags: 0x01, 0x02, 0xf0 new_name, 0xf3 prev_name,
 *       0x69 deriv_path, 0x51 family, [0x23 chain_id], 0x29 hmac_proof
 * HMAC: address_book_verify_hmac_proof_ledger_account (mocked via sys_address_book_hmac_verify).
 * App callback: handle_check_edit_ledger_account → SWO_INCORRECT_DATA on rejection.
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
    /* omit hmac_proof → verify_fields fails */
    size_t len = build_edit_ledger_account(buf,
                                           sizeof(buf),
                                           0x30,
                                           0x01,
                                           "New",
                                           "Old",
                                           true,
                                           true,
                                           0x00,
                                           false,
                                           false /* no hmac_proof */);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_ethereum_missing_chain_id(void)
{
    uint8_t buf[512];
    size_t  len = build_edit_ledger_account(buf,
                                           sizeof(buf),
                                           0x30,
                                           0x01,
                                           "New",
                                           "Old",
                                           true,
                                           true,
                                           0x01,  /* FAMILY_ETHEREUM */
                                           false, /* no chain_id */
                                           true);
    TEST_ASSERT_EQUAL_INT(SWO_INCORRECT_DATA, edit_ledger_account(buf, len));
}

static void test_ela_app_rejects(void)
{
    g_mock_ela_result = false;
    uint8_t buf[512];
    size_t  len = build_valid_edit_ledger_account(buf, sizeof(buf));
    /* edit_ledger_account returns SWO_INCORRECT_DATA (not SWO_WRONG_PARAMETER_VALUE)
     * when the coin-app callback rejects — see ledger_account_edit.c. */
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

    /* register_identity */
    RUN_TEST(test_ri_wrong_struct_type);
    RUN_TEST(test_ri_missing_mandatory_field);
    RUN_TEST(test_ri_ethereum_missing_chain_id);
    RUN_TEST(test_ri_group_handle_without_hmac_proof);
    RUN_TEST(test_ri_hmac_fails);
    RUN_TEST(test_ri_app_rejects);
    RUN_TEST(test_ri_success);
    RUN_TEST(test_ri_review_rejected);

    /* edit_contact_name */
    RUN_TEST(test_ecn_wrong_struct_type);
    RUN_TEST(test_ecn_missing_mandatory_field);
    RUN_TEST(test_ecn_hmac_fails);
    RUN_TEST(test_ecn_success);
    RUN_TEST(test_ecn_review_rejected);

    /* edit_identifier */
    RUN_TEST(test_ei_wrong_struct_type);
    RUN_TEST(test_ei_missing_mandatory_field);
    RUN_TEST(test_ei_ethereum_missing_chain_id);
    RUN_TEST(test_ei_hmac_fails);
    RUN_TEST(test_ei_app_rejects);
    RUN_TEST(test_ei_success);
    RUN_TEST(test_ei_review_rejected);

    /* edit_scope */
    RUN_TEST(test_es_wrong_struct_type);
    RUN_TEST(test_es_missing_mandatory_field);
    RUN_TEST(test_es_ethereum_missing_chain_id);
    RUN_TEST(test_es_hmac_fails);
    RUN_TEST(test_es_success);
    RUN_TEST(test_es_review_rejected);

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
