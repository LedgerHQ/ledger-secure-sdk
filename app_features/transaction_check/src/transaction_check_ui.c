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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_helpers.h"
#include "os_print.h"
#include "io.h"
#include "macros.h"
#include "status_words.h"
#include "nbgl_use_case.h"
#include "transaction_check.h"
#include "transaction_check_entrypoints.h"
#include "transaction_check_internal.h"

#if defined(HAVE_TX_CHECK)

/* Private defines------------------------------------------------------------*/
#define HOW_IT_WORKS_NB 3

/* Private variables ---------------------------------------------------------*/
static bool s_response_expected;
// The reply is sent by the SDK (SDK APDU), else by the app
static bool s_sdk_reply;

// "How it works" page, displayed from the opt-in screen
static nbgl_genericDetails_t      s_how_it_works;
static const char                *s_how_it_works_texts[HOW_IT_WORKS_NB];
static const nbgl_icon_details_t *s_how_it_works_icons[HOW_IT_WORKS_NB];

/* Private functions ---------------------------------------------------------*/
/**
 * @brief Send the 1-byte enable state with SWO_SUCCESS.
 *
 * @param[in] enabled Transaction Check enable state
 */
static void send_opt_in_reply(bool enabled)
{
    uint8_t reply = enabled;
    io_send_response_pointer(&reply, sizeof(reply), SWO_SUCCESS);
}

/**
 * @brief Hand the end of the opt-in flow over to the app.
 */
static void finalize_opt_in(void)
{
    finalize_ui_tx_check_opt_in(s_response_expected);
}

/**
 * @brief Store the user choice, send the pending reply, then display the status.
 *
 * @param[in] enable Whether the user enabled Transaction Check
 */
static void opt_in_choice_cb(bool enable)
{
    handle_tx_check_set_setting(TX_CHECK_SETTING_OPT_IN, true);
    if (enable != handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE)) {
        handle_tx_check_set_setting(TX_CHECK_SETTING_ENABLE, enable);
    }
    if (s_response_expected) {
        bool state = handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE);
        if (s_sdk_reply) {
            send_opt_in_reply(state);
        }
        else {
            handle_tx_check_opt_in_response(state);
        }
    }
    if (enable) {
        nbgl_useCaseStatus("Transaction Check enabled", true, finalize_opt_in);
    }
    else {
        finalize_opt_in();
    }
}

/**
 * @brief Display the opt-in screen if allowed and not seen yet.
 *
 * @param[in] response_expected Whether an APDU reply is pending
 * @param[in] sdk_reply         Whether that reply is sent by the SDK (SDK APDU)
 * @return TX_CHECK_PENDING, TX_CHECK_SUCCESS or TX_CHECK_NOT_ALLOWED
 */
static tx_check_status_t start_opt_in(bool response_expected, bool sdk_reply)
{
    if (tx_check_in_swap()) {
        PRINTF("[TX CHECK] Not available in a swap\n");
        return TX_CHECK_NOT_ALLOWED;
    }
    // The opt-in screen would replace a flow on screen
    if (response_expected && !handle_tx_check_is_idle()) {
        PRINTF("[TX CHECK] Opt-in requested while a flow is on screen\n");
        return TX_CHECK_NOT_ALLOWED;
    }
    if (handle_tx_check_get_setting(TX_CHECK_SETTING_OPT_IN)) {
        PRINTF("[TX CHECK] Already opted in\n");
        return TX_CHECK_SUCCESS;
    }
    s_response_expected = response_expected;
    s_sdk_reply         = sdk_reply;

    s_how_it_works_texts[0] = "Transaction is checked for threats before signing.";
    s_how_it_works_texts[1]
        = "The result is displayed: Critical threat, potential risk or no threat.";
    s_how_it_works_texts[2]
        = "Scan the QR Code on your Ledger device for details on any threat or risk.";
    s_how_it_works_icons[0] = &PRIVACY_ICON;
    s_how_it_works_icons[1] = &WARNING_ICON;
    s_how_it_works_icons[2] = &QRCODE_ICON;

    memset(&s_how_it_works, 0, sizeof(s_how_it_works));
    s_how_it_works.type                 = LEFT_CONTENT_WARNING;
    s_how_it_works.leftContent.title    = "How it works";
    s_how_it_works.leftContent.nbRows   = HOW_IT_WORKS_NB;
    s_how_it_works.leftContent.rowTexts = s_how_it_works_texts;
    s_how_it_works.leftContent.rowIcons = s_how_it_works_icons;

    nbgl_useCaseAdvancedChoiceWithDetails(NULL,
                                          &QUESTION_CIRCLE_ICON,
                                          "Enable\nTransaction Check?",
                                          "Real-time warnings powered\nby 3rd service providers.",
                                          "By enabling, you accept T&Cs: ledger.com/tx-check",
                                          "Yes, enable",
                                          "Maybe later",
                                          &s_how_it_works,
                                          opt_in_choice_cb);
    return TX_CHECK_PENDING;
}

/* Exported functions --------------------------------------------------------*/
/**
 * @brief Default reply to an opt-in request on the app APDU: the 1-byte enable state.
 *
 * @param[in] enabled Transaction Check enable state
 */
WEAK void handle_tx_check_opt_in_response(bool enabled)
{
    send_opt_in_reply(enabled);
}

/**
 * @brief Display the opt-in screen, from the settings or for an app APDU.
 *
 * @param[in] response_expected Whether an APDU reply is pending
 * @return Same as start_opt_in()
 */
tx_check_status_t tx_check_opt_in(bool response_expected)
{
    return start_opt_in(response_expected, false);
}

#if defined(HAVE_TX_CHECK_APDU)
/**
 * @brief Display the opt-in screen for the SDK APDU, replied by the SDK.
 *
 * @return Same as start_opt_in()
 */
tx_check_status_t tx_check_opt_in_sdk_apdu(void)
{
    return start_opt_in(true, true);
}
#endif  // HAVE_TX_CHECK_APDU
#endif  // HAVE_TX_CHECK
