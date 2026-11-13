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
 * @file test_transaction_check_ui.c
 * @brief Unit tests for transaction_check_ui.c: opt-in screen, user choice, settings update and
 *        deferred APDU reply, sent by the SDK or by the app.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"
#include "Mockio.h"
#include "Mocknbgl_use_case.h"

#include "status_words.h"
#include "transaction_check.h"
#include "transaction_check_entrypoints.h"
#include "transaction_check_internal.h"

volatile bool G_called_from_swap;

/* ── Glyphs ──────────────────────────────────────────────────────────────── */

const nbgl_icon_details_t C_Privacy_40px;
const nbgl_icon_details_t C_QRCode_40px;
const nbgl_icon_details_t C_Warning_40px;
const nbgl_icon_details_t C_Question_Mark_Circle_40px;

/* ── Coin-app entrypoint stubs ───────────────────────────────────────────── */

static bool g_setting_opt_in;
static bool g_setting_enable;
static bool g_app_idle;
static int  g_finalize_calls;
static bool g_finalize_response_expected;
static int  g_app_reply_calls;
static bool g_app_reply_enabled;
static bool g_app_reply_before_status;

static const char *g_status_msg;

bool handle_check_tx_report(const tlv_transaction_check_out_t *descriptor)
{
    (void) descriptor;
    return true;
}

bool handle_check_tx_report_signer(const uint8_t            *address,
                                   size_t                    address_len,
                                   const tx_check_context_t *ctx)
{
    (void) address;
    (void) address_len;
    (void) ctx;
    return true;
}

bool handle_check_tx_report_chain_id(bool                      chain_id_received,
                                     uint64_t                  chain_id,
                                     const tx_check_context_t *ctx)
{
    (void) chain_id_received;
    (void) chain_id;
    (void) ctx;
    return true;
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

void handle_tx_check_opt_in_response(bool enabled)
{
    g_app_reply_before_status = (g_status_msg == NULL);
    g_app_reply_calls++;
    g_app_reply_enabled = enabled;
}

void finalize_ui_tx_check_opt_in(bool response_expected)
{
    g_finalize_calls++;
    g_finalize_response_expected = response_expected;
}

/* ── NBGL / IO captures ──────────────────────────────────────────────────── */

static nbgl_choiceCallback_t        g_choice_cb;
static int                          g_choice_count;
static const char                  *g_message;
static const nbgl_genericDetails_t *g_details;
static nbgl_callback_t              g_status_cb;
static int                          g_reply_calls;
static uint8_t                      g_reply_byte;
static uint16_t                     g_reply_sw;

static void choice_cb(const nbgl_icon_details_t *centerIcon,
                      const nbgl_icon_details_t *headerIcon,
                      const char                *title,
                      const char                *message,
                      const char                *subMessage,
                      const char                *confirmText,
                      const char                *cancelText,
                      nbgl_warningDetails_t     *details,
                      nbgl_choiceCallback_t      callback,
                      int                        num_calls)
{
    (void) centerIcon;
    (void) headerIcon;
    (void) title;
    (void) subMessage;
    (void) confirmText;
    (void) cancelText;
    (void) num_calls;
    g_choice_cb = callback;
    g_message   = message;
    g_details   = details;
    g_choice_count++;
}

static void status_cb(const char     *message,
                      bool            isSuccess,
                      nbgl_callback_t quitCallback,
                      int             num_calls)
{
    (void) isSuccess;
    (void) num_calls;
    g_status_msg = message;
    g_status_cb  = quitCallback;
}

static int send_response_cb(const buffer_t *rdatalist, size_t count, uint16_t sw, int num_calls)
{
    (void) num_calls;
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(1, rdatalist[0].size);
    g_reply_byte = rdatalist[0].ptr[0];
    g_reply_sw   = sw;
    g_reply_calls++;
    return 0;
}

void setUp(void)
{
    Mockio_Init();
    Mocknbgl_use_case_Init();
    g_setting_opt_in             = false;
    g_setting_enable             = false;
    g_app_idle                   = true;
    G_called_from_swap           = false;
    g_app_reply_calls            = 0;
    g_app_reply_enabled          = false;
    g_app_reply_before_status    = false;
    g_finalize_calls             = 0;
    g_finalize_response_expected = false;
    g_choice_cb                  = NULL;
    g_choice_count               = 0;
    g_message                    = NULL;
    g_details                    = NULL;
    g_status_cb                  = NULL;
    g_status_msg                 = NULL;
    g_reply_calls                = 0;
    nbgl_useCaseAdvancedChoiceWithDetails_StubWithCallback(choice_cb);
    nbgl_useCaseStatus_StubWithCallback(status_cb);
    io_send_response_buffers_StubWithCallback(send_response_cb);
}

void tearDown(void)
{
    Mockio_Verify();
    Mockio_Destroy();
    Mocknbgl_use_case_Verify();
    Mocknbgl_use_case_Destroy();
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_opt_in_already_done(void)
{
    g_setting_opt_in = true;
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, tx_check_opt_in(true));
    TEST_ASSERT_EQUAL(TX_CHECK_SUCCESS, tx_check_opt_in_sdk_apdu());
    TEST_ASSERT_EQUAL(0, g_choice_count);
}

static void test_opt_in_screen(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in(false));
    TEST_ASSERT_EQUAL(1, g_choice_count);
    TEST_ASSERT_EQUAL_STRING("Real-time warnings powered\nby 3rd service providers.", g_message);
    TEST_ASSERT_NOT_NULL(g_details);
    TEST_ASSERT_EQUAL(LEFT_CONTENT_WARNING, g_details->type);
    TEST_ASSERT_EQUAL_STRING("How it works", g_details->leftContent.title);
    TEST_ASSERT_EQUAL(3, g_details->leftContent.nbRows);
    TEST_ASSERT_FALSE(g_setting_opt_in);
}

static void test_opt_in_not_allowed_when_not_idle(void)
{
    g_app_idle = false;
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, tx_check_opt_in(true));
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, tx_check_opt_in_sdk_apdu());
    TEST_ASSERT_EQUAL(0, g_choice_count);
    // the settings switch does not wait for idle
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in(false));
}

static void test_opt_in_not_allowed_in_swap(void)
{
    G_called_from_swap = true;
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, tx_check_opt_in(true));
    TEST_ASSERT_EQUAL(TX_CHECK_NOT_ALLOWED, tx_check_opt_in_sdk_apdu());
    TEST_ASSERT_EQUAL(0, g_choice_count);
}

static void test_opt_in_accepted_from_sdk_apdu(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in_sdk_apdu());
    g_choice_cb(true);
    TEST_ASSERT_TRUE(g_setting_opt_in);
    TEST_ASSERT_TRUE(g_setting_enable);
    TEST_ASSERT_EQUAL(1, g_reply_calls);
    TEST_ASSERT_EQUAL(1, g_reply_byte);
    TEST_ASSERT_EQUAL_HEX16(SWO_SUCCESS, g_reply_sw);
    TEST_ASSERT_EQUAL(0, g_app_reply_calls);
    TEST_ASSERT_EQUAL_STRING("Transaction Check enabled", g_status_msg);
    TEST_ASSERT_EQUAL(0, g_finalize_calls);
    g_status_cb();
    TEST_ASSERT_EQUAL(1, g_finalize_calls);
    TEST_ASSERT_TRUE(g_finalize_response_expected);
}

static void test_opt_in_declined_from_sdk_apdu(void)
{
    g_setting_enable = true;
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in_sdk_apdu());
    g_choice_cb(false);
    TEST_ASSERT_TRUE(g_setting_opt_in);
    TEST_ASSERT_FALSE(g_setting_enable);
    TEST_ASSERT_EQUAL(1, g_reply_calls);
    TEST_ASSERT_EQUAL(0, g_reply_byte);
    TEST_ASSERT_NULL(g_status_msg);
    TEST_ASSERT_EQUAL(1, g_finalize_calls);
}

// On the app APDU, the app sends the reply, before the status screen
static void test_opt_in_accepted_from_app_apdu(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in(true));
    g_choice_cb(true);
    TEST_ASSERT_EQUAL(0, g_reply_calls);
    TEST_ASSERT_EQUAL(1, g_app_reply_calls);
    TEST_ASSERT_TRUE(g_app_reply_enabled);
    TEST_ASSERT_TRUE(g_app_reply_before_status);
    TEST_ASSERT_EQUAL_STRING("Transaction Check enabled", g_status_msg);
    g_status_cb();
    TEST_ASSERT_EQUAL(1, g_finalize_calls);
    TEST_ASSERT_TRUE(g_finalize_response_expected);
}

static void test_opt_in_declined_from_app_apdu(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in(true));
    g_choice_cb(false);
    TEST_ASSERT_EQUAL(0, g_reply_calls);
    TEST_ASSERT_EQUAL(1, g_app_reply_calls);
    TEST_ASSERT_FALSE(g_app_reply_enabled);
    TEST_ASSERT_EQUAL(1, g_finalize_calls);
}

static void test_opt_in_from_settings_sends_no_reply(void)
{
    TEST_ASSERT_EQUAL(TX_CHECK_PENDING, tx_check_opt_in(false));
    g_choice_cb(true);
    TEST_ASSERT_TRUE(g_setting_enable);
    TEST_ASSERT_EQUAL(0, g_reply_calls);
    TEST_ASSERT_EQUAL(0, g_app_reply_calls);
    g_status_cb();
    TEST_ASSERT_EQUAL(1, g_finalize_calls);
    TEST_ASSERT_FALSE(g_finalize_response_expected);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_opt_in_already_done);
    RUN_TEST(test_opt_in_screen);
    RUN_TEST(test_opt_in_not_allowed_when_not_idle);
    RUN_TEST(test_opt_in_not_allowed_in_swap);
    RUN_TEST(test_opt_in_accepted_from_sdk_apdu);
    RUN_TEST(test_opt_in_declined_from_sdk_apdu);
    RUN_TEST(test_opt_in_accepted_from_app_apdu);
    RUN_TEST(test_opt_in_declined_from_app_apdu);
    RUN_TEST(test_opt_in_from_settings_sends_no_reply);
    return UNITY_END();
}
