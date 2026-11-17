/*****************************************************************************
 *   (c) 2026 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

/**
 * @file test_transaction_check.c
 * @brief Unit tests for transaction_check.c: APDU dispatch, descriptor reception and binding of
 *        the report to the operation being signed.
 *
 * The TLV parsing and signature verification (tlv_use_case_transaction_check) and the coin-app
 * entrypoints are replaced by controllable stubs.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"
#include "Mockio.h"
#include "Mocknbgl_use_case.h"

#include "apdu_chunks.h"
#include "status_words.h"
#include "transaction_check.h"
#include "transaction_check_entrypoints.h"
#include "transaction_check_internal.h"

volatile bool G_called_from_swap;

#define P1_DESCRIPTOR 0x00
#define P1_OPT_IN     0x01

/* ── TLV use case stub ───────────────────────────────────────────────────── */

static tlv_transaction_check_status_t g_uc_ret;
static tlv_transaction_check_out_t    g_uc_out;
static size_t                         g_uc_payload_size;
static int                            g_uc_calls;
static uint8_t                        g_uc_tx_hash[TRANSACTION_CHECK_HASH_SIZE];
static uint8_t                        g_uc_domain_hash[TRANSACTION_CHECK_HASH_SIZE];
static uint8_t                        g_uc_address[TRANSACTION_CHECK_MAX_ADDRESS_SIZE + 1];

tlv_transaction_check_status_t tlv_use_case_transaction_check(const buffer_t              *payload,
                                                              tlv_transaction_check_out_t *out)
{
    g_uc_calls++;
    g_uc_payload_size = payload->size;
    memset(out, 0, sizeof(*out));
    if (g_uc_ret != TLV_TRANSACTION_CHECK_SUCCESS) {
        return g_uc_ret;
    }
    *out = g_uc_out;
    return TLV_TRANSACTION_CHECK_SUCCESS;
}

/* ── Coin-app entrypoint stubs ───────────────────────────────────────────── */

static bool        g_setting_opt_in;
static bool        g_setting_enable;
static bool        g_app_idle;
static bool        g_check_descriptor_ret;
static bool        g_check_signer_ret;
static bool        g_check_chain_id_ret;
static uint8_t     g_signer_address[TRANSACTION_CHECK_MAX_ADDRESS_SIZE];
static size_t      g_signer_address_len;
static const void *g_signer_app_data;
static bool        g_opt_in_displayed;

bool handle_check_tx_report(const tlv_transaction_check_out_t *descriptor)
{
    (void) descriptor;
    return g_check_descriptor_ret;
}

bool handle_check_tx_report_signer(const uint8_t            *address,
                                   size_t                    address_len,
                                   const tx_check_context_t *ctx)
{
    memcpy(g_signer_address, address, address_len);
    g_signer_address_len = address_len;
    g_signer_app_data    = ctx->app_data;
    return g_check_signer_ret;
}

bool handle_check_tx_report_chain_id(bool                      chain_id_received,
                                     uint64_t                  chain_id,
                                     const tx_check_context_t *ctx)
{
    (void) chain_id_received;
    (void) chain_id;
    (void) ctx;
    return g_check_chain_id_ret;
}

bool handle_tx_check_get_setting(tx_check_setting_t setting)
{
    return (setting == TX_CHECK_SETTING_OPT_IN) ? g_setting_opt_in : g_setting_enable;
}

void handle_tx_check_set_setting(tx_check_setting_t setting, bool value)
{
    if (setting == TX_CHECK_SETTING_OPT_IN) {
        g_setting_opt_in = value;
    }
    else {
        g_setting_enable = value;
    }
}

bool handle_tx_check_is_idle(void)
{
    return g_app_idle;
}

void finalize_ui_tx_check_opt_in(bool response_expected)
{
    (void) response_expected;
}

void handle_tx_check_opt_in_response(bool enabled)
{
    (void) enabled;
}

/* Opt-in screen, tested in test_transaction_check_ui.c */
static tx_check_status_t g_opt_in_ret;

tx_check_status_t tx_check_opt_in_sdk_apdu(void)
{
    g_opt_in_displayed = true;
    return g_opt_in_ret;
}

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static uint8_t        g_hash[TRANSACTION_CHECK_HASH_SIZE];
static uint8_t        g_domain[TRANSACTION_CHECK_HASH_SIZE];
static nbgl_warning_t g_warning;

/* Descriptor content returned by the TLV stub: a benign transaction check. */
static void set_descriptor(transaction_check_risk_t     risk,
                           transaction_check_category_t category,
                           transaction_check_type_t     type)
{
    memset(&g_uc_out, 0, sizeof(g_uc_out));
    g_uc_out.risk              = risk;
    g_uc_out.category          = category;
    g_uc_out.type              = type;
    g_uc_out.chain_id          = 1;
    g_uc_out.chain_id_received = true;
    g_uc_out.tx_hash           = (buffer_t){.ptr = g_uc_tx_hash, .size = sizeof(g_uc_tx_hash)};
    g_uc_out.address           = (buffer_t){.ptr = g_uc_address, .size = 20};
    strcpy(g_uc_out.tiny_url, "ledger.com/r/1");
    strcpy(g_uc_out.partner, "Provider");
    if (type == TRANSACTION_CHECK_TYPE_TYPED_DATA) {
        g_uc_out.domain_hash
            = (buffer_t){.ptr = g_uc_domain_hash, .size = sizeof(g_uc_domain_hash)};
        g_uc_out.domain_hash_received = true;
    }
}

static tx_check_status_t provide(void)
{
    uint8_t dummy = 0;
    return tx_check_provide_descriptor(&(const buffer_t){.ptr = &dummy, .size = 1});
}

static tx_check_context_t tx_context(void)
{
    return (tx_check_context_t){.type = TRANSACTION_CHECK_TYPE_TRANSACTION, .tx_hash = g_hash};
}

static uint32_t apply(const tx_check_context_t *ctx)
{
    memset(&g_warning, 0, sizeof(g_warning));
    tx_check_apply_report(&g_warning, ctx);
    return g_warning.predefinedSet;
}

static bolos_err_t send_apdu(uint8_t        p1,
                             uint8_t        p2,
                             const uint8_t *data,
                             size_t         len,
                             uint8_t       *out,
                             size_t        *out_len)
{
    uint8_t dummy_out[16];
    size_t  dummy_len = sizeof(dummy_out);
    if (out == NULL) {
        out     = dummy_out;
        out_len = &dummy_len;
    }
    return tx_check_handle_apdu(data, len, p1, p2, out, out_len);
}

void setUp(void)
{
    Mockio_Init();
    Mocknbgl_use_case_Init();
    tx_check_clear();
    g_uc_ret               = TLV_TRANSACTION_CHECK_SUCCESS;
    g_uc_calls             = 0;
    g_uc_payload_size      = 0;
    g_setting_opt_in       = true;
    g_setting_enable       = true;
    g_app_idle             = true;
    g_check_descriptor_ret = true;
    g_check_signer_ret     = true;
    g_check_chain_id_ret   = true;
    g_signer_address_len   = 0;
    g_signer_app_data      = NULL;
    g_opt_in_displayed     = false;
    g_opt_in_ret           = TX_CHECK_SUCCESS;
    G_called_from_swap     = false;
    memset(g_uc_tx_hash, 0xBB, sizeof(g_uc_tx_hash));
    memset(g_uc_domain_hash, 0xCC, sizeof(g_uc_domain_hash));
    memset(g_uc_address, 0xAA, sizeof(g_uc_address));
    memcpy(g_hash, g_uc_tx_hash, sizeof(g_hash));
    memcpy(g_domain, g_uc_domain_hash, sizeof(g_domain));
    set_descriptor(TRANSACTION_CHECK_RISK_BENIGN,
                   TRANSACTION_CHECK_CATEGORY_NA,
                   TRANSACTION_CHECK_TYPE_TRANSACTION);
}

void tearDown(void)
{
    Mockio_Verify();
    Mockio_Destroy();
    Mocknbgl_use_case_Verify();
    Mocknbgl_use_case_Destroy();
}

/* ── APDU dispatch ───────────────────────────────────────────────────────── */

static void test_apdu_unknown_p1(void)
{
    TEST_ASSERT_EQUAL_HEX16(SWO_WRONG_P1_P2, send_apdu(0x02, 0x00, NULL, 0, NULL, NULL));
}

static void test_apdu_descriptor_single_chunk(void)
{
    const uint8_t apdu[] = {0x00, 0x03, 0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SUCCESS,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
    TEST_ASSERT_EQUAL(1, g_uc_calls);
    TEST_ASSERT_EQUAL(3, g_uc_payload_size);
}

static void test_apdu_descriptor_two_chunks(void)
{
    const uint8_t first[] = {0x00, 0x04, 0x01, 0x02};
    const uint8_t next[]  = {0x03, 0x04};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SUCCESS,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, first, sizeof(first), NULL, NULL));
    TEST_ASSERT_EQUAL(0, g_uc_calls);
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SUCCESS, send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_NEXT, next, sizeof(next), NULL, NULL));
    TEST_ASSERT_EQUAL(1, g_uc_calls);
    TEST_ASSERT_EQUAL(4, g_uc_payload_size);
}

static void test_apdu_descriptor_first_chunk_too_short(void)
{
    const uint8_t apdu[] = {0x00};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
}

static void test_apdu_descriptor_too_large(void)
{
    const uint8_t apdu[] = {(TRANSACTION_CHECK_MAX_PAYLOAD_SIZE + 1) >> 8,
                            (TRANSACTION_CHECK_MAX_PAYLOAD_SIZE + 1) & 0xFF};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
}

static void test_apdu_descriptor_invalid_p2_drops_payload(void)
{
    const uint8_t first[] = {0x00, 0x04, 0x01, 0x02};
    const uint8_t next[]  = {0x03, 0x04};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SUCCESS,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, first, sizeof(first), NULL, NULL));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_DATA,
                            send_apdu(P1_DESCRIPTOR, 0x01, next, sizeof(next), NULL, NULL));
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_NEXT, next, sizeof(next), NULL, NULL));
    TEST_ASSERT_EQUAL(0, g_uc_calls);
}

static void test_apdu_descriptor_invalid_first_chunk_drops_payload(void)
{
    const uint8_t first[]       = {0x00, 0x04, 0x01, 0x02};
    const uint8_t short_first[] = {0x00};
    const uint8_t next[]        = {0x03, 0x04};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SUCCESS,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, first, sizeof(first), NULL, NULL));
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(
            P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, short_first, sizeof(short_first), NULL, NULL));
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_NEXT, next, sizeof(next), NULL, NULL));
    TEST_ASSERT_EQUAL(0, g_uc_calls);
}

static void test_apdu_descriptor_continuation_without_first(void)
{
    const uint8_t apdu[] = {0x01};
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_NEXT, apdu, sizeof(apdu), NULL, NULL));
}

static void test_apdu_descriptor_continuation_of_another_feature(void)
{
    const uint8_t first[] = {0x00, 0x04, 0x01, 0x02};
    const uint8_t next[]  = {0x03, 0x04};
    uint8_t      *buf     = NULL;
    size_t        len     = 0;
    TEST_ASSERT_EQUAL(APDU_CHUNKS_PENDING,
                      apdu_chunks_reassemble(APDU_CHUNKS_OWNER_ADDRESS_BOOK,
                                             first,
                                             sizeof(first),
                                             APDU_CHUNKS_P2_FIRST,
                                             &buf,
                                             &len));
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_NEXT, next, sizeof(next), NULL, NULL));
    TEST_ASSERT_EQUAL(0, g_uc_calls);
}

static void test_apdu_opt_in_already_opted_in(void)
{
    uint8_t out[4]   = {0};
    size_t  out_len  = sizeof(out);
    g_setting_enable = false;
    g_opt_in_ret     = TX_CHECK_SUCCESS;
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, send_apdu(P1_OPT_IN, 0x00, NULL, 0, out, &out_len));
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL(0, out[0]);
}

static void test_apdu_opt_in_displays_screen(void)
{
    size_t  out_len = 4;
    uint8_t out[4];
    g_opt_in_ret = TX_CHECK_PENDING;
    TEST_ASSERT_EQUAL_HEX16(SWO_NO_RESPONSE, send_apdu(P1_OPT_IN, 0x00, NULL, 0, out, &out_len));
    TEST_ASSERT_EQUAL(0, out_len);
    TEST_ASSERT_TRUE(g_opt_in_displayed);
}

static void test_apdu_opt_in_not_allowed(void)
{
    size_t  out_len = 4;
    uint8_t out[4];
    g_opt_in_ret = TX_CHECK_NOT_ALLOWED;
    TEST_ASSERT_EQUAL_HEX16(SWO_COMMAND_NOT_ALLOWED,
                            send_apdu(P1_OPT_IN, 0x00, NULL, 0, out, &out_len));
    TEST_ASSERT_EQUAL(0, out_len);
}

// Each descriptor status keeps its status word on the SDK APDU
static void test_apdu_descriptor_status_words(void)
{
    const uint8_t apdu[] = {0x00, 0x01, 0x01};

    g_uc_ret = TLV_TRANSACTION_CHECK_SIGNATURE_ERROR | 0x01;
    TEST_ASSERT_EQUAL_HEX16(
        SWO_SECURITY_CONDITION_NOT_SATISFIED,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
    g_uc_ret = TLV_TRANSACTION_CHECK_MISSING_TAG;
    TEST_ASSERT_EQUAL_HEX16(
        SWO_INCORRECT_DATA,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
    g_uc_ret         = TLV_TRANSACTION_CHECK_SUCCESS;
    g_setting_enable = false;
    TEST_ASSERT_EQUAL_HEX16(
        SWO_COMMAND_CODE_NOT_SUPPORTED,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
    g_setting_enable   = true;
    G_called_from_swap = true;
    TEST_ASSERT_EQUAL_HEX16(
        SWO_COMMAND_NOT_ALLOWED,
        send_apdu(P1_DESCRIPTOR, APDU_CHUNKS_P2_FIRST, apdu, sizeof(apdu), NULL, NULL));
}

static void test_status_to_sw(void)
{
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, tx_check_status_to_sw(TX_CHECK_SUCCESS));
    TEST_ASSERT_EQUAL_HEX16(SWO_NO_RESPONSE, tx_check_status_to_sw(TX_CHECK_PENDING));
    TEST_ASSERT_EQUAL_HEX16(SWO_COMMAND_CODE_NOT_SUPPORTED,
                            tx_check_status_to_sw(TX_CHECK_DISABLED));
    TEST_ASSERT_EQUAL_HEX16(SWO_COMMAND_NOT_ALLOWED, tx_check_status_to_sw(TX_CHECK_NOT_ALLOWED));
    TEST_ASSERT_EQUAL_HEX16(SWO_SECURITY_CONDITION_NOT_SATISFIED,
                            tx_check_status_to_sw(TX_CHECK_INVALID_SIGNATURE));
    TEST_ASSERT_EQUAL_HEX16(SWO_INCORRECT_DATA, tx_check_status_to_sw(TX_CHECK_INVALID_DESCRIPTOR));
}

static void test_apdu_opt_in_rejects_p2_and_data(void)
{
    const uint8_t data[] = {0x00};
    TEST_ASSERT_EQUAL_HEX16(SWO_WRONG_P1_P2, send_apdu(P1_OPT_IN, 0x80, NULL, 0, NULL, NULL));
    TEST_ASSERT_EQUAL_HEX16(SWO_WRONG_P1_P2,
                            send_apdu(P1_OPT_IN, 0x00, data, sizeof(data), NULL, NULL));
}

/* ── Descriptor reception ────────────────────────────────────────────────── */

static void test_provide_disabled(void)
{
    g_setting_enable = false;
    TEST_ASSERT_EQUAL(TX_CHECK_DISABLED, provide());
    TEST_ASSERT_EQUAL(0, g_uc_calls);
}

static void test_provide_replaces_report(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    set_descriptor(TRANSACTION_CHECK_RISK_MALICIOUS,
                   TRANSACTION_CHECK_CATEGORY_OTHERS,
                   TRANSACTION_CHECK_TYPE_TRANSACTION);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_THREAT_DETECTED_WARN, apply(&ctx));
}

static void test_provide_invalid_drops_report(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    g_uc_ret = TLV_TRANSACTION_CHECK_MISSING_TAG;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_DESCRIPTOR, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_provide_rejected_while_bound_to_review(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_NO_THREAT_WARN, apply(&ctx));
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, provide());
    // the review still shows the bound report
    TEST_ASSERT_EQUAL_STRING("Provider", g_warning.reportProvider);
    tx_check_clear();
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
}

static void test_provide_replaces_unbound_report(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    // a report not matching the operation is not bound to the review
    g_hash[0] ^= 0xFF;
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
}

static void test_provide_rejected_in_swap(void)
{
    G_called_from_swap = true;
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, provide());
    TEST_ASSERT_EQUAL(0, g_uc_calls);
}

static void test_provide_parsing_error(void)
{
    g_uc_ret = TLV_TRANSACTION_CHECK_MISSING_TAG;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_DESCRIPTOR, provide());
}

static void test_provide_signature_error(void)
{
    // PKI status combined with the signature error mask
    g_uc_ret = TLV_TRANSACTION_CHECK_SIGNATURE_ERROR | 0x01;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_SIGNATURE, provide());
}

static void test_max_payload_size(void)
{
    TEST_ASSERT_EQUAL(277, TRANSACTION_CHECK_MAX_PAYLOAD_SIZE);
}

static void test_provide_additional_data_rejected(void)
{
    g_uc_out.additional_data_received = true;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_DESCRIPTOR, provide());
}

static void test_provide_address_too_long(void)
{
    g_uc_out.address.size = TRANSACTION_CHECK_MAX_ADDRESS_SIZE + 1;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_DESCRIPTOR, provide());
}

static void test_provide_rejected_by_app_is_not_stored(void)
{
    g_check_descriptor_ret = false;
    TEST_ASSERT_EQUAL(TX_CHECK_INVALID_DESCRIPTOR, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
    g_check_descriptor_ret = true;
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
}

/* ── Binding to the operation ────────────────────────────────────────────── */

static void test_apply_disabled_leaves_warning(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    g_setting_enable       = false;
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(0, apply(&ctx));
    TEST_ASSERT_NULL(g_warning.reportProvider);
}

static void test_apply_without_report(void)
{
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_without_operation(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(NULL));
    tx_check_context_t ctx = tx_context();
    ctx.tx_hash            = NULL;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_type_mismatch(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    ctx.type               = TRANSACTION_CHECK_TYPE_TYPED_DATA;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_chain_id_rejected(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    g_check_chain_id_ret   = false;
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_signer_rejected(void)
{
    static const int app_data = 0;
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    g_check_signer_ret     = false;
    tx_check_context_t ctx = tx_context();
    ctx.app_data           = &app_data;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
    TEST_ASSERT_EQUAL(20, g_signer_address_len);
    TEST_ASSERT_EQUAL_MEMORY(g_uc_address, g_signer_address, 20);
    TEST_ASSERT_EQUAL_PTR(&app_data, g_signer_app_data);
}

static void test_apply_hash_mismatch(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    g_hash[0] ^= 0xFF;
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_domain_hash(void)
{
    set_descriptor(TRANSACTION_CHECK_RISK_BENIGN,
                   TRANSACTION_CHECK_CATEGORY_NA,
                   TRANSACTION_CHECK_TYPE_TYPED_DATA);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    ctx.type               = TRANSACTION_CHECK_TYPE_TYPED_DATA;
    ctx.domain_hash        = g_domain;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_NO_THREAT_WARN, apply(&ctx));
    g_domain[0] ^= 0xFF;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_domain_hash_not_received(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    ctx.domain_hash        = g_domain;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

static void test_apply_benign(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_NO_THREAT_WARN, apply(&ctx));
    TEST_ASSERT_EQUAL_STRING("Provider", g_warning.reportProvider);
    TEST_ASSERT_EQUAL_STRING("ledger.com/r/1", g_warning.reportUrl);
    TEST_ASSERT_NULL(g_warning.providerMessage);
}

static void test_apply_report_domain_without_context_domain(void)
{
    set_descriptor(TRANSACTION_CHECK_RISK_BENIGN,
                   TRANSACTION_CHECK_CATEGORY_NA,
                   TRANSACTION_CHECK_TYPE_TYPED_DATA);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    ctx.type               = TRANSACTION_CHECK_TYPE_TYPED_DATA;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
}

// Without domain on either side, typed data is bound by its hash only
static void test_apply_typed_data_without_domain(void)
{
    set_descriptor(TRANSACTION_CHECK_RISK_BENIGN,
                   TRANSACTION_CHECK_CATEGORY_NA,
                   TRANSACTION_CHECK_TYPE_TYPED_DATA);
    g_uc_out.domain_hash_received = false;
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    ctx.type               = TRANSACTION_CHECK_TYPE_TYPED_DATA;
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_NO_THREAT_WARN, apply(&ctx));
}

static void test_apply_reused_warning(void)
{
    tx_check_context_t ctx = tx_context();

    set_descriptor(TRANSACTION_CHECK_RISK_WARNING,
                   TRANSACTION_CHECK_CATEGORY_DAPP,
                   TRANSACTION_CHECK_TYPE_TRANSACTION);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    memset(&g_warning, 0, sizeof(g_warning));
    g_warning.predefinedSet = 1 << BLIND_SIGNING_WARN;
    tx_check_apply_report(&g_warning, &ctx);

    // Same warning, not zeroed, for a malicious report
    tx_check_clear();
    set_descriptor(TRANSACTION_CHECK_RISK_MALICIOUS,
                   TRANSACTION_CHECK_CATEGORY_DAPP,
                   TRANSACTION_CHECK_TYPE_TRANSACTION);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_apply_report(&g_warning, &ctx);
    TEST_ASSERT_EQUAL_HEX32((1 << BLIND_SIGNING_WARN) | (1 << W3C_THREAT_DETECTED_WARN),
                            g_warning.predefinedSet);
    TEST_ASSERT_EQUAL_STRING(
        "This dApp is linked to a scammer. "
        "Your assets will most likely be stolen.",
        g_warning.providerMessage);

    // Same warning, Transaction Check disabled: no stale verdict is left
    g_setting_enable = false;
    tx_check_apply_report(&g_warning, &ctx);
    TEST_ASSERT_EQUAL_HEX32(1 << BLIND_SIGNING_WARN, g_warning.predefinedSet);
    TEST_ASSERT_NULL(g_warning.reportProvider);
    TEST_ASSERT_NULL(g_warning.providerMessage);
    TEST_ASSERT_NULL(g_warning.reportUrl);
}

static void test_apply_keeps_other_warnings(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    memset(&g_warning, 0, sizeof(g_warning));
    g_warning.predefinedSet = 1 << BLIND_SIGNING_WARN;
    tx_check_apply_report(&g_warning, &ctx);
    TEST_ASSERT_EQUAL_HEX32((1 << BLIND_SIGNING_WARN) | (1 << W3C_NO_THREAT_WARN),
                            g_warning.predefinedSet);
}

static void check_risk(transaction_check_risk_t     risk,
                       transaction_check_category_t category,
                       uint32_t                     expected_set,
                       const char                  *expected_text)
{
    tx_check_clear();
    set_descriptor(risk, category, TRANSACTION_CHECK_TYPE_TRANSACTION);
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(expected_set, apply(&ctx));
    TEST_ASSERT_EQUAL_STRING(expected_text, g_warning.providerMessage);
}

static void test_apply_warning_texts(void)
{
    check_risk(TRANSACTION_CHECK_RISK_WARNING,
               TRANSACTION_CHECK_CATEGORY_ADDRESS,
               1 << W3C_RISK_DETECTED_WARN,
               "This transaction involves a suspicious address. "
               "It might not be safe to continue.");
    check_risk(TRANSACTION_CHECK_RISK_WARNING,
               TRANSACTION_CHECK_CATEGORY_DAPP,
               1 << W3C_RISK_DETECTED_WARN,
               "This transaction involves a suspicious dApp. "
               "It might not be safe to continue.");
    check_risk(TRANSACTION_CHECK_RISK_WARNING,
               TRANSACTION_CHECK_CATEGORY_LOSING_OPERATION,
               1 << W3C_RISK_DETECTED_WARN,
               "This transaction could end in a loss. "
               "Check transaction details carefully before signing.");
    check_risk(TRANSACTION_CHECK_RISK_WARNING,
               TRANSACTION_CHECK_CATEGORY_OTHERS,
               1 << W3C_RISK_DETECTED_WARN,
               "This transaction might be malicious. It might not be safe to continue. "
               "Tap the QR code icon for more details.");
}

static void test_apply_threat_texts(void)
{
    check_risk(TRANSACTION_CHECK_RISK_MALICIOUS,
               TRANSACTION_CHECK_CATEGORY_ADDRESS,
               1 << W3C_THREAT_DETECTED_WARN,
               "This transaction involves a malicious address. "
               "Your assets will most likely be stolen.");
    check_risk(TRANSACTION_CHECK_RISK_MALICIOUS,
               TRANSACTION_CHECK_CATEGORY_DAPP,
               1 << W3C_THREAT_DETECTED_WARN,
               "This dApp is linked to a scammer. "
               "Your assets will most likely be stolen.");
    check_risk(TRANSACTION_CHECK_RISK_MALICIOUS,
               TRANSACTION_CHECK_CATEGORY_OTHERS,
               1 << W3C_THREAT_DETECTED_WARN,
               "This request is malicious. Your assets will most likely be stolen. "
               "View full report for details.");
}

static void test_clear_drops_report(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
    tx_check_clear();
    tx_check_context_t ctx = tx_context();
    TEST_ASSERT_EQUAL_HEX32(1 << W3C_ISSUE_WARN, apply(&ctx));
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, provide());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_apdu_unknown_p1);
    RUN_TEST(test_apdu_descriptor_single_chunk);
    RUN_TEST(test_apdu_descriptor_two_chunks);
    RUN_TEST(test_apdu_descriptor_first_chunk_too_short);
    RUN_TEST(test_apdu_descriptor_too_large);
    RUN_TEST(test_apdu_descriptor_invalid_p2_drops_payload);
    RUN_TEST(test_apdu_descriptor_invalid_first_chunk_drops_payload);
    RUN_TEST(test_apdu_descriptor_continuation_without_first);
    RUN_TEST(test_apdu_descriptor_continuation_of_another_feature);
    RUN_TEST(test_apdu_opt_in_already_opted_in);
    RUN_TEST(test_apdu_opt_in_displays_screen);
    RUN_TEST(test_apdu_opt_in_not_allowed);
    RUN_TEST(test_apdu_descriptor_status_words);
    RUN_TEST(test_status_to_sw);
    RUN_TEST(test_apdu_opt_in_rejects_p2_and_data);
    RUN_TEST(test_provide_disabled);
    RUN_TEST(test_provide_replaces_report);
    RUN_TEST(test_provide_invalid_drops_report);
    RUN_TEST(test_provide_rejected_while_bound_to_review);
    RUN_TEST(test_provide_replaces_unbound_report);
    RUN_TEST(test_provide_rejected_in_swap);
    RUN_TEST(test_provide_parsing_error);
    RUN_TEST(test_provide_signature_error);
    RUN_TEST(test_max_payload_size);
    RUN_TEST(test_provide_additional_data_rejected);
    RUN_TEST(test_provide_address_too_long);
    RUN_TEST(test_provide_rejected_by_app_is_not_stored);
    RUN_TEST(test_apply_disabled_leaves_warning);
    RUN_TEST(test_apply_without_report);
    RUN_TEST(test_apply_without_operation);
    RUN_TEST(test_apply_type_mismatch);
    RUN_TEST(test_apply_chain_id_rejected);
    RUN_TEST(test_apply_signer_rejected);
    RUN_TEST(test_apply_hash_mismatch);
    RUN_TEST(test_apply_domain_hash);
    RUN_TEST(test_apply_domain_hash_not_received);
    RUN_TEST(test_apply_benign);
    RUN_TEST(test_apply_report_domain_without_context_domain);
    RUN_TEST(test_apply_typed_data_without_domain);
    RUN_TEST(test_apply_reused_warning);
    RUN_TEST(test_apply_keeps_other_warnings);
    RUN_TEST(test_apply_warning_texts);
    RUN_TEST(test_apply_threat_texts);
    RUN_TEST(test_clear_drops_report);
    return UNITY_END();
}
