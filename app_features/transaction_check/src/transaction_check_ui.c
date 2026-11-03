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
#include "status_words.h"
#include "nbgl_use_case.h"
#include "transaction_check.h"
#include "transaction_check_entrypoints.h"

#if defined(HAVE_TX_CHECK)

/* Private defines------------------------------------------------------------*/
#define HOW_IT_WORKS_NB 3

/* Private variables ---------------------------------------------------------*/
static bool s_response_expected;

// "How it works" page, displayed from the opt-in screen
static nbgl_genericDetails_t      s_how_it_works;
static const char                *s_how_it_works_texts[HOW_IT_WORKS_NB];
static const nbgl_icon_details_t *s_how_it_works_icons[HOW_IT_WORKS_NB];

/* Private functions ---------------------------------------------------------*/
static void finalize_opt_in(void)
{
    finalize_ui_tx_check_opt_in(s_response_expected);
}

static void opt_in_choice_cb(bool enable)
{
    handle_tx_check_set_setting(TX_CHECK_SETTING_OPT_IN, true);
    if (enable != handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE)) {
        handle_tx_check_set_setting(TX_CHECK_SETTING_ENABLE, enable);
    }
    if (s_response_expected) {
        uint8_t state = handle_tx_check_get_setting(TX_CHECK_SETTING_ENABLE);
        io_send_response_pointer(&state, sizeof(state), SWO_SUCCESS);
    }
    if (enable) {
        nbgl_useCaseStatus("Transaction Check enabled", true, finalize_opt_in);
    }
    else {
        finalize_opt_in();
    }
}

/* Exported functions --------------------------------------------------------*/
bool tx_check_opt_in(bool response_expected)
{
    if (handle_tx_check_get_setting(TX_CHECK_SETTING_OPT_IN)) {
        PRINTF("[TX CHECK] Already opted in\n");
        return false;
    }
    s_response_expected = response_expected;

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
    return true;
}
#endif  // HAVE_TX_CHECK
