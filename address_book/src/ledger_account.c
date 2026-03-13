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
 * @file ledger_account.c
 * @brief Register / Rename Ledger Account flow
 *
 * Flow:
 *  1. Parse TLV payload (account name + derivation path + chain ID +
 *     blockchain family)
 *  2. Coin-app validation: handle_check_ledger_account()
 *  3. Display UI: account name + derivation path
 *  4. On confirm: compute HMAC Proof of Registration and send to host
 *
 * Active under HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT.
 * TLV structure types: TYPE_REGISTER_LEDGER_ACCOUNT (0x13) /
 *                      TYPE_RENAME_LEDGER_ACCOUNT   (0x14)
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_utils.h"
#include "os_apdu.h"
#include "status_words.h"
#include "tlv_library.h"
#include "buffer.h"
#include "bip32.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "io.h"
#include "ledger_account.h"
#include "address_book_common.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK) && defined(HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    ledger_account_t *ledger_account;
    TLV_reception_t   received_tags;
} s_ledger_account_ctx;

/* Private macros-------------------------------------------------------------*/
#define LEDGER_ACCOUNT_TAGS(X)                                                \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static ledger_account_t           LEDGER_ACCOUNT = {0};
static nbgl_contentTagValueList_t ui_pairsList   = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_REGISTER_LEDGER_ACCOUNT);
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

/**
 * @brief Handler for tag \ref CONTACT_NAME
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_name(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              (char *) context->ledger_account->account_name,
                                              sizeof(context->ledger_account->account_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->ledger_account->bip32_path);
}

/**
 * @brief Parse the CHAIN_ID value
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_chain_id(data, &context->ledger_account->chain_id);
}

/**
 * @brief Parse the BLOCKCHAIN_FAMILY value
 *
 * @param[in] data data to handle
 * @param[out] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_ledger_account_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->ledger_account->blockchain_family);
}

DEFINE_TLV_PARSER(LEDGER_ACCOUNT_TAGS, NULL, ledger_account_tlv_parser)

/**
 * @brief Verify the received fields
 *
 * Check the mandatory fields are present
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_fields(const s_ledger_account_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY);
    if (!result) {
        PRINTF("Missing mandatory fields in Ledger Account descriptor!\n");
        return false;
    }
    if (context->ledger_account->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("Missing CHAIN_ID for Ethereum family in Ledger Account descriptor!\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Print the received descriptor
 *
 * @param[in] context the received payload
 * Only for debug purpose.
 */
static void print_payload(const s_ledger_account_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[Ledger Account] - Retrieved Descriptor:\n");
    PRINTF("[Ledger Account] -    Account name: %s\n", context->ledger_account->account_name);
    if (bip32_path_format_simple(&context->ledger_account->bip32_path, out, sizeof(out))) {
        PRINTF("[Ledger Account] -    Derivation path[%d]: %s\n",
               context->ledger_account->bip32_path.length,
               out);
    }
    else {
        PRINTF("[Ledger Account] -    Derivation path length[%d] (failed to format)\n",
               context->ledger_account->bip32_path.length);
    }
    if (context->ledger_account->blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[Ledger Account] -    Chain ID: %llu\n", context->ledger_account->chain_id);
    }
}

/**
 * @brief Build and send response
 *
 * Compute HMAC Proof of Registration and send it to the host
 * Response format: type(1) | hmac_proof(32).
 *
 * @return true if successful, false otherwise
 */
static bool build_and_send_response(void)
{
    uint8_t hmac_proof[32] = {0};

    if (!address_book_compute_hmac_proof_ledger_account(&LEDGER_ACCOUNT.bip32_path,
                                                        LEDGER_ACCOUNT.account_name,
                                                        LEDGER_ACCOUNT.blockchain_family,
                                                        LEDGER_ACCOUNT.chain_id,
                                                        hmac_proof)) {
        PRINTF("Error: Failed to compute HMAC proof\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_REGISTER_LEDGER_ACCOUNT, hmac_proof);
    explicit_bzero(hmac_proof, sizeof(hmac_proof));
    return ok;
}

/**
 * @brief NBGL callback invoked after the user confirms or rejects the registration
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_ledger_account_choice(bool confirm)
{
    if (confirm) {
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Saved to your accounts", true, finalize_ui_register_ledger_account);
        }
        else {
            PRINTF("Error: Failed to build and send response\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error saving account", false, finalize_ui_register_ledger_account);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED,
                                 finalize_ui_register_ledger_account);
    }
}

/**
 * @brief Display Register Ledger Account confirmation flow
 */
static void ui_display_ledger_account(void)
{
    const nbgl_icon_details_t *icon = NULL;
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 2;  // name + account details
    ui_pairsList.callback = get_register_ledger_account_tagValue;
    ui_pairsList.wrapping = true;
    if (LEDGER_ACCOUNT.blockchain_family == FAMILY_ETHEREUM) {
        ui_pairsList.nbPairs++;  // + Network if Ethereum family
    }

    // Get the icon for the UI
    icon = get_ledger_account_icon();
    // Display the review screen
    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            icon,
                            "Review account details",
                            NULL,
                            "Confirm account details?",
                            review_ledger_account_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Handle Register Ledger Account operation
 *
 * Parses the TLV payload, validates the account, and displays the review UI.
 *
 * @param[in] buffer_in Complete TLV payload
 * @param[in] buffer_in_length Payload length
 * @return Status word
 */
bolos_err_t register_ledger_account(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t       payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_ledger_account_ctx ctx     = {0};

    // Init the structure
    ctx.ledger_account = &LEDGER_ACCOUNT;

    // Parse using SDK TLV parser
    if (!ledger_account_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("Failed to parse Ledger Account TLV payload!\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Check the account validity according to the Coin application logic
    if (!handle_check_ledger_account(ctx.ledger_account)) {
        PRINTF("Error: Ledger Account rejected by coin application!\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    // Display confirmation UI
    ui_display_ledger_account();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
