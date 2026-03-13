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
 * @file identity.c
 * @brief Register Identity flow
 *
 * Flow:
 *  1. Parse TLV payload (name + [scope] + identifier + derivation_path)
 *  2. Coin-app validation: handle_check_identity()
 *  3. Display UI: contact name + identifier
 *  4. On confirm: compute HMAC Proof of Registration and send to host
 *
 * TLV structure type: TYPE_REGISTER_IDENTITY (0x11)
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
#include "address_book_common.h"
#include "identity.h"
#include "io.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    identity_t     *identity;
    TLV_reception_t received_tags;
} s_identity_ctx;

/* Private macros-------------------------------------------------------------*/
#define REGISTER_IDENTITY_TAGS(X)                                             \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0x0B, TAG_CONTACT_SCOPE, handle_contact_scope, ENFORCE_UNIQUE_TAG)      \
    X(0x0F, TAG_IDENTIFIER, handle_identifier, ENFORCE_UNIQUE_TAG)            \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static identity_t                 IDENTITY     = {0};
static nbgl_contentTagValueList_t ui_pairsList = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_identity_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_REGISTER_IDENTITY);
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_identity_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

/**
 * @brief Handler for tag \ref CONTACT_NAME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_name(const tlv_data_t *data, s_identity_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->identity->contact_name, sizeof(context->identity->contact_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref CONTACT_SCOPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_contact_scope(const tlv_data_t *data, s_identity_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->identity->contact_scope, sizeof(context->identity->contact_scope))) {
        PRINTF("CONTACT_SCOPE: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref IDENTIFIER
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_identifier(const tlv_data_t *data, s_identity_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, 1, IDENTIFIER_MAX_LENGTH)) {
        PRINTF("IDENTIFIER: failed to extract\n");
        return false;
    }
    memmove(context->identity->identifier, buf.ptr, buf.size);
    context->identity->identifier_len = (uint8_t) buf.size;
    return true;
}

/**
 * @brief Handler for tag \ref TAG_DERIVATION_PATH.
 */
static bool handle_derivation_path(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->identity->bip32_path);
}

/**
 * @brief Handler for tag \ref TAG_CHAIN_ID.
 */
static bool handle_chain_id(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_chain_id(data, &context->identity->chain_id);
}

/**
 * @brief Handler for tag \ref TAG_BLOCKCHAIN_FAMILY.
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->identity->blockchain_family);
}

DEFINE_TLV_PARSER(REGISTER_IDENTITY_TAGS, NULL, identity_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received.
 *
 * CHAIN_ID is conditional: mandatory for FAMILY_ETHEREUM only.
 *
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool verify_fields(const s_identity_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_CONTACT_SCOPE,
                                          TAG_IDENTIFIER,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY);
    if (!result) {
        PRINTF("Missing mandatory fields in Register Identity descriptor!\n");
        return false;
    }
    if (context->identity->blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("Missing CHAIN_ID for Ethereum family in Register Identity descriptor!\n");
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
static void print_payload(const s_identity_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};

    PRINTF("****************************************************************************\n");
    PRINTF("[Identity] - Retrieved Descriptor:\n");
    PRINTF("[Identity] -    Contact name: %s\n", context->identity->contact_name);
    if (context->identity->contact_scope[0] != '\0') {
        PRINTF("[Identity] -    Contact scope: %s\n", context->identity->contact_scope);
    }
    if (bip32_path_format_simple(&context->identity->bip32_path, out, sizeof(out))) {
        PRINTF(
            "[Identity] -    Derivation path[%d]: %s\n", context->identity->bip32_path.length, out);
    }
    else {
        PRINTF("[Identity] -    Derivation path length[%d] (failed to format)\n",
               context->identity->bip32_path.length);
    }
    if (context->identity->blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[Identity] -    Chain ID: %llu\n", context->identity->chain_id);
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

    if (!address_book_compute_hmac_proof_identity(&IDENTITY.bip32_path,
                                                  IDENTITY.contact_name,
                                                  IDENTITY.contact_scope,
                                                  IDENTITY.identifier,
                                                  IDENTITY.identifier_len,
                                                  hmac_proof)) {
        PRINTF("Error: Failed to compute HMAC proof\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_REGISTER_IDENTITY, hmac_proof);
    explicit_bzero(hmac_proof, sizeof(hmac_proof));
    return ok;
}

/**
 * @brief NBGL callback invoked after the user confirms or rejects the registration
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_register_identity_choice(bool confirm)
{
    if (confirm) {
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Identity registered", true, finalize_ui_register_identity);
        }
        else {
            PRINTF("Error: Failed to build and send HMAC proof\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error registering identity", false, finalize_ui_register_identity);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_register_identity);
    }
}

/**
 * @brief Display the Register Identity review screen
 */
static void ui_display_register_identity(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 3;  // name + identifier + network
    ui_pairsList.callback = get_register_identity_tagValue;
    ui_pairsList.wrapping = true;
    if (IDENTITY.contact_scope[0] != '\0') {
        ui_pairsList.nbPairs++;  // + scope if present
    }

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Register identity",
                            NULL,
                            "Confirm identity registration?",
                            review_register_identity_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Register a new identity.
 *
 * @param[in] buffer_in        the input buffer containing the identity data
 * @param[in] buffer_in_length the length of the input buffer
 * @return bolos_err_t         the result of the registration
 */
bolos_err_t register_identity(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_identity_ctx ctx     = {0};

    // Init the structure
    ctx.identity = &IDENTITY;

    // Parse using SDK TLV parser
    if (!identity_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Check the Identity validity according to the Coin application logic
    if (!handle_check_identity(ctx.identity)) {
        PRINTF("Error: Identity rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    // Display confirmation UI
    ui_display_register_identity();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
