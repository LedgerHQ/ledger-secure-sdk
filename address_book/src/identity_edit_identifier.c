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
 * @file identity_edit_identifier.c
 * @brief Edit Identifier flow (P1=0x03, struct type 0x31)
 *
 * Allows changing the IDENTIFIER of an existing Identity contact while
 * keeping the same contact_name and scope.
 *
 * Flow:
 *  1. Receive fully assembled payload (multi-chunk reassembly handled by
 *     address_book.c)
 *  2. Parse TLV payload (contact_name + scope + new_identifier +
 *     previous_identifier + gid + derivation_path +
 *     hmac_name_proof + hmac_rest_proof)
 *  3. Verify HMAC_NAME over (gid, contact_name)
 *  4. Verify HMAC_REST over (gid, scope, previous_identifier, family[, chain_id])
 *  5. Coin-app notification: handle_check_edit_identifier()
 *  6. Display UI: contact name, previous identifier → new identifier
 *  7. On confirm: compute new HMAC_REST over (gid, scope, new_identifier,
 *     family[, chain_id]) and send to host
 *
 * Active under HAVE_ADDRESS_BOOK.
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
#include "identity.h"
#include "io.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    edit_identifier_t *edit;
    TLV_reception_t    received_tags;
    uint8_t            hmac_name[CX_SHA256_SIZE];  ///< HMAC_NAME — verifies contact name
    uint8_t            hmac_rest[CX_SHA256_SIZE];  ///< HMAC_REST — verifies previous identifier
    uint8_t group_handle[GROUP_HANDLE_SIZE];       ///< Group handle from wallet (verified later)
} s_edit_ctx;

/* Private macros-------------------------------------------------------------*/
#define EDIT_IDENTIFIER_TAGS(X)                                                      \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)              \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG)        \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)               \
    X(0xf1, TAG_SCOPE, handle_scope, ENFORCE_UNIQUE_TAG)                             \
    X(0xf2, TAG_ACCOUNT_IDENTIFIER, handle_identifier, ENFORCE_UNIQUE_TAG)           \
    X(0xf4, TAG_PREVIOUS_IDENTIFIER, handle_previous_identifier, ENFORCE_UNIQUE_TAG) \
    X(0xf6, TAG_GROUP_HANDLE, handle_group_handle, ENFORCE_UNIQUE_TAG)               \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)         \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                       \
    X(0x29, TAG_HMAC_NAME, handle_hmac_name, ENFORCE_UNIQUE_TAG)                     \
    X(0xf7, TAG_HMAC_REST, handle_hmac_rest, ENFORCE_UNIQUE_TAG)                     \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static edit_identifier_t          EDIT_IDENTIFIER = {0};
static nbgl_contentTagValueList_t ui_pairsList    = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_edit_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_EDIT_IDENTIFIER)) {
        PRINTF("[Edit Identifier] Invalid STRUCTURE_TYPE value\n");
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
static bool handle_struct_version(const tlv_data_t *data, s_edit_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Edit Identifier] Invalid STRUCTURE_VERSION value\n");
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
static bool handle_contact_name(const tlv_data_t *data, s_edit_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              context->edit->identity.contact_name,
                                              sizeof(context->edit->identity.contact_name))) {
        PRINTF("[Edit Identifier] CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref SCOPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_scope(const tlv_data_t *data, s_edit_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->edit->identity.scope, sizeof(context->edit->identity.scope))) {
        PRINTF("[Edit Identifier] SCOPE: failed to parse\n");
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
static bool handle_identifier(const tlv_data_t *data, s_edit_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, 1, IDENTIFIER_MAX_LENGTH)) {
        PRINTF("[Edit Identifier] IDENTIFIER: failed to extract\n");
        return false;
    }
    memmove(context->edit->identity.identifier, buf.ptr, buf.size);
    context->edit->identity.identifier_len = (uint8_t) buf.size;
    return true;
}

/**
 * @brief Handler for tag \ref PREVIOUS_IDENTIFIER
 *
 * Used to verify the HMAC proof of the previous registration.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_previous_identifier(const tlv_data_t *data, s_edit_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, 1, IDENTIFIER_MAX_LENGTH)) {
        PRINTF("[Edit Identifier] PREVIOUS_IDENTIFIER: failed to extract\n");
        return false;
    }
    memmove(context->edit->previous_identifier, buf.ptr, buf.size);
    context->edit->previous_identifier_len = (uint8_t) buf.size;
    return true;
}

/**
 * @brief Handler for tag \ref GROUP_HANDLE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_group_handle(const tlv_data_t *data, s_edit_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, GROUP_HANDLE_SIZE, GROUP_HANDLE_SIZE)) {
        PRINTF("[Edit Identifier] GROUP_HANDLE: invalid length (expected %d bytes)\n",
               GROUP_HANDLE_SIZE);
        return false;
    }
    memmove(context->group_handle, buf.ptr, GROUP_HANDLE_SIZE);
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_edit_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->edit->identity.bip32_path);
}

/**
 * @brief Handler for tag \ref CHAIN_ID
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_edit_ctx *context)
{
    return address_book_handle_chain_id(data, &context->edit->identity.chain_id);
}

/**
 * @brief Handler for tag \ref BLOCKCHAIN_FAMILY
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_edit_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->edit->identity.blockchain_family);
}

/**
 * @brief Handler for tag \ref HMAC_NAME
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_name(const tlv_data_t *data, s_edit_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Edit Identifier] HMAC_NAME: invalid length (expected %d bytes)\n", CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_name, buf.ptr, CX_SHA256_SIZE);
    return true;
}

/**
 * @brief Handler for tag \ref HMAC_REST
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_rest(const tlv_data_t *data, s_edit_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Edit Identifier] HMAC_REST: invalid length (expected %d bytes)\n", CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_rest, buf.ptr, CX_SHA256_SIZE);
    return true;
}

DEFINE_TLV_PARSER(EDIT_IDENTIFIER_TAGS, NULL, edit_identifier_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_edit_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_ACCOUNT_IDENTIFIER,
                                          TAG_PREVIOUS_IDENTIFIER,
                                          TAG_GROUP_HANDLE,
                                          TAG_DERIVATION_PATH,
                                          TAG_BLOCKCHAIN_FAMILY,
                                          TAG_HMAC_NAME,
                                          TAG_HMAC_REST);
    if (!result) {
        PRINTF("[Edit Identifier] Missing mandatory fields!\n");
        return false;
    }
    if (context->edit->identity.blockchain_family == FAMILY_ETHEREUM) {
        result = TLV_CHECK_RECEIVED_TAGS(context->received_tags, TAG_CHAIN_ID);
        if (!result) {
            PRINTF("[Edit Identifier] Missing CHAIN_ID for Ethereum family!\n");
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
static void print_payload(const s_edit_ctx *context)
{
    UNUSED(context);
    PRINTF("****************************************************************************\n");
    PRINTF("[Edit Identifier] - Retrieved Descriptor:\n");
    PRINTF("[Edit Identifier] -    Contact name:        %s\n",
           context->edit->identity.contact_name);
    if (context->edit->identity.scope[0] != '\0') {
        PRINTF("[Edit Identifier] -    Scope:               %s\n", context->edit->identity.scope);
    }
    PRINTF("[Edit Identifier] -    New identifier len:  %d\n",
           context->edit->identity.identifier_len);
    PRINTF("[Edit Identifier] -    Prev identifier len: %d\n",
           context->edit->previous_identifier_len);
    PRINTF("[Edit Identifier] -    Blockchain family:   %s\n",
           FAMILY_AS_STR(context->edit->identity.blockchain_family));
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
    uint8_t hmac_rest[CX_SHA256_SIZE] = {0};

    if (!address_book_compute_hmac_rest(&EDIT_IDENTIFIER.identity.bip32_path,
                                        EDIT_IDENTIFIER.identity.gid,
                                        EDIT_IDENTIFIER.identity.scope,
                                        EDIT_IDENTIFIER.identity.identifier,
                                        EDIT_IDENTIFIER.identity.identifier_len,
                                        EDIT_IDENTIFIER.identity.blockchain_family,
                                        EDIT_IDENTIFIER.identity.chain_id,
                                        hmac_rest)) {
        PRINTF("[Edit Identifier] Error: Failed to compute new HMAC_REST\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_EDIT_IDENTIFIER, hmac_rest);
    explicit_bzero(hmac_rest, sizeof(hmac_rest));
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
            if (EDIT_IDENTIFIER.identity.blockchain_family == FAMILY_ETHEREUM) {
                nbgl_useCaseStatus("Address changed", true, finalize_ui_edit_identifier);
            }
            else {
                nbgl_useCaseStatus("Identifier changed", true, finalize_ui_edit_identifier);
            }
        }
        else {
            PRINTF("[Edit Identifier] Error: Failed to build and send HMAC proof\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error during update", false, finalize_ui_edit_identifier);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_edit_identifier);
    }
}

/**
 * @brief Display the Edit Identifier review screen
 */
static void ui_display(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 4;  // name + scope + old identifier + new identifier
    ui_pairsList.callback = get_edit_identifier_tagValue;
    ui_pairsList.wrapping = true;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Review change to contact details",
                            NULL,
                            "Confirm change?",
                            review_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Edit the IDENTIFIER of an existing Identity contact.
 *
 * @param[in] buffer_in        the fully assembled TLV payload
 * @param[in] buffer_in_length the length of the payload
 * @return bolos_err_t         the result of the edit
 */
bolos_err_t edit_identifier(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_edit_ctx     ctx     = {0};

    // Init the structure
    ctx.edit = &EDIT_IDENTIFIER;

    // Parse using SDK TLV parser
    if (!edit_identifier_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Edit Identifier] TLV parsing failed\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Verify the group handle and extract the gid
    if (!address_book_verify_group_handle(
            &EDIT_IDENTIFIER.identity.bip32_path, ctx.group_handle, EDIT_IDENTIFIER.identity.gid)) {
        PRINTF("[Edit Identifier] Group handle verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Verify that the wallet holds a valid HMAC_NAME for the contact name
    if (!address_book_verify_hmac_name(&EDIT_IDENTIFIER.identity.bip32_path,
                                       EDIT_IDENTIFIER.identity.gid,
                                       EDIT_IDENTIFIER.identity.contact_name,
                                       ctx.hmac_name)) {
        PRINTF("[Edit Identifier] HMAC_NAME verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Verify that the wallet holds a valid HMAC_REST for the previous identifier
    if (!address_book_verify_hmac_rest(&EDIT_IDENTIFIER.identity.bip32_path,
                                       EDIT_IDENTIFIER.identity.gid,
                                       EDIT_IDENTIFIER.identity.scope,
                                       EDIT_IDENTIFIER.previous_identifier,
                                       EDIT_IDENTIFIER.previous_identifier_len,
                                       EDIT_IDENTIFIER.identity.blockchain_family,
                                       EDIT_IDENTIFIER.identity.chain_id,
                                       ctx.hmac_rest)) {
        PRINTF("[Edit Identifier] HMAC_REST verification failed\n");
        return SWO_CONDITIONS_NOT_SATISFIED;
    }

    // Notify the coin app so it can validate and store display data
    if (!handle_check_edit_identifier(&EDIT_IDENTIFIER)) {
        PRINTF("[Edit Identifier] Rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    // Display confirmation UI
    ui_display();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
