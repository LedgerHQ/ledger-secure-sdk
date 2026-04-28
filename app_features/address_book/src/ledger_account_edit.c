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
 * @file ledger_account_edit.c
 * @brief Edit Ledger Account flow (P1=0x12, struct type 0x30)
 *
 * Flow:
 *  1. Receive fully assembled payload (multi-chunk reassembly handled by
 *     address_book.c)
 *  2. Parse TLV payload (previous_name + new_name + derivation_path +
 *     chain_id + blockchain_family + hmac_proof)
 *  3. Verify HMAC proof of the previous registration
 *  4. Display UI: old name → new name
 *  5. On confirm: compute new HMAC Proof of Registration and send to host
 *
 * Active under HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT.
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "os_apdu.h"
#include "status_words.h"
#include "tlv_library.h"
#include "buffer.h"
#include "bip32.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "address_book_common.h"
#include "ledger_account.h"
#include "io.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK) && defined(HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

typedef struct {
    edit_ledger_account_t *edit;
    TLV_reception_t        received_tags;
    uint8_t                hmac_proof[CX_SHA256_SIZE];  ///< HMAC proof of the previous registration
} s_edit_ledger_account_ctx;

/* Private macros-------------------------------------------------------------*/
#define EDIT_LEDGER_ACCOUNT_TAGS(X)                                           \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0xf3, TAG_PREVIOUS_NAME, handle_previous_name, ENFORCE_UNIQUE_TAG)      \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                \
    X(0x29, TAG_HMAC_PROOF, handle_hmac_proof, ENFORCE_UNIQUE_TAG)            \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static edit_ledger_account_t      EDIT_LEDGER_ACCOUNT = {0};
static nbgl_contentTagValue_t     ui_pairs[2]         = {0};
static nbgl_contentTagValueList_t ui_pairsList        = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_EDIT_LEDGER_ACCOUNT)) {
        PRINTF("[Edit Ledger Account] Invalid STRUCTURE_TYPE value\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Edit Ledger Account] Invalid STRUCTURE_VERSION value\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref CONTACT_NAME
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_name(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              context->edit->ledger_account.account_name,
                                              sizeof(context->edit->ledger_account.account_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref PREVIOUS_NAME
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_previous_name(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              context->edit->previous_account_name,
                                              sizeof(context->edit->previous_account_name))) {
        PRINTF("PREVIOUS_NAME: failed to parse\n");
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
static bool handle_derivation_path(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->edit->ledger_account.bip32_path);
}

/**
 * @brief Handler for tag \ref CHAIN_ID
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    return address_book_handle_chain_id(data, &context->edit->ledger_account.chain_id);
}

/**
 * @brief Handler for tag \ref BLOCKCHAIN_FAMILY
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    return address_book_handle_blockchain_family(data,
                                                 &context->edit->ledger_account.blockchain_family);
}

/**
 * @brief Handler for tag \ref HMAC_PROOF
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_proof(const tlv_data_t *data, s_edit_ledger_account_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Edit Ledger Account] HMAC_PROOF: invalid length (expected %d bytes)\n",
               CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_proof, buf.ptr, CX_SHA256_SIZE);
    return true;
}

DEFINE_TLV_PARSER(EDIT_LEDGER_ACCOUNT_TAGS, NULL, edit_ledger_account_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_edit_ledger_account_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_PREVIOUS_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_HMAC_PROOF);
    if (!result) {
        PRINTF("Missing mandatory fields in Edit Ledger Account descriptor!\n");
        return false;
    }
    if (context->edit->ledger_account.blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("Missing CHAIN_ID for Ethereum family in Edit Ledger Account descriptor!\n");
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
static void print_payload(const s_edit_ledger_account_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[Edit Ledger Account] - Retrieved Descriptor:\n");
    PRINTF("[Edit Ledger Account] -    Previous name:       %s\n",
           context->edit->previous_account_name);
    PRINTF("[Edit Ledger Account] -    New name:            %s\n",
           context->edit->ledger_account.account_name);
    if (bip32_path_format_simple(&context->edit->ledger_account.bip32_path, out, sizeof(out))) {
        PRINTF("[Edit Ledger Account] -    Derivation path[%d]: %s\n",
               context->edit->ledger_account.bip32_path.length,
               out);
    }
    else {
        PRINTF("[Edit Ledger Account] -    Derivation path length[%d] (failed to format)\n",
               context->edit->ledger_account.bip32_path.length);
    }
    PRINTF("[Edit Ledger Account] -    Blockchain family:   %s\n",
           FAMILY_AS_STR(context->edit->ledger_account.blockchain_family));
    if (context->edit->ledger_account.blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[Edit Ledger Account] -    Chain ID:            %llu\n",
               context->edit->ledger_account.chain_id);
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
    uint8_t hmac_proof[CX_SHA256_SIZE] = {0};

    if (!address_book_compute_hmac_proof_ledger_account(
            &EDIT_LEDGER_ACCOUNT.ledger_account.bip32_path,
            (const char *) EDIT_LEDGER_ACCOUNT.ledger_account.account_name,
            EDIT_LEDGER_ACCOUNT.ledger_account.blockchain_family,
            EDIT_LEDGER_ACCOUNT.ledger_account.chain_id,
            hmac_proof)) {
        PRINTF("[Edit Ledger Account] Error: Failed to compute new HMAC proof\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_EDIT_LEDGER_ACCOUNT, hmac_proof);
    explicit_bzero(hmac_proof, sizeof(hmac_proof));
    return ok;
}

/**
 * @brief NBGL callback invoked after the user confirms or rejects the edit
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_choice(bool confirm)
{
    if (confirm) {
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Account name changed", true, finalize_ui_edit_ledger_account);
        }
        else {
            PRINTF("[Edit Ledger Account] Error: Failed to build and send HMAC proof\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error editing account", false, finalize_ui_edit_ledger_account);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_edit_ledger_account);
    }
}

/**
 * @brief Display the Edit Ledger Account review screen
 */
static void ui_display(void)
{
    uint8_t nbPairs = 0;
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    memset(ui_pairs, 0, sizeof(ui_pairs));
    ui_pairs[nbPairs].item  = "Previous name";
    ui_pairs[nbPairs].value = EDIT_LEDGER_ACCOUNT.previous_account_name;
    nbPairs++;
    ui_pairs[nbPairs].item  = "New name";
    ui_pairs[nbPairs].value = EDIT_LEDGER_ACCOUNT.ledger_account.account_name;
    nbPairs++;
    ui_pairsList.pairs    = ui_pairs;
    ui_pairsList.nbPairs  = nbPairs;  // previous name + new name
    ui_pairsList.wrapping = true;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            NULL,
                            "Edit account name",
                            NULL,
                            "Confirm edit?",
                            review_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Edit the name of an existing Ledger Account
 *
 * @param[in] buffer_in        the input buffer containing the account data
 * @param[in] buffer_in_length the length of the input buffer
 * @return bolos_err_t         the result of the edit
 */
bolos_err_t edit_ledger_account(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t            payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_edit_ledger_account_ctx ctx     = {0};

    // Init the structure
    ctx.edit = &EDIT_LEDGER_ACCOUNT;
    memset(&EDIT_LEDGER_ACCOUNT, 0, sizeof(EDIT_LEDGER_ACCOUNT));

    // Parse using SDK TLV parser
    if (!edit_ledger_account_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Edit Ledger Account] TLV parsing failed\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Verify that the host holds a valid proof from the previous registration
    if (!address_book_verify_hmac_proof_ledger_account(
            &EDIT_LEDGER_ACCOUNT.ledger_account.bip32_path,
            EDIT_LEDGER_ACCOUNT.previous_account_name,
            EDIT_LEDGER_ACCOUNT.ledger_account.blockchain_family,
            EDIT_LEDGER_ACCOUNT.ledger_account.chain_id,
            ctx.hmac_proof)) {
        PRINTF("[Edit Ledger Account] HMAC proof verification failed\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Display confirmation UI
    ui_display();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_LEDGER_ACCOUNT
