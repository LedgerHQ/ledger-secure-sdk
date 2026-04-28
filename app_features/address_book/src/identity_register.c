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
 * @file identity_register.c
 * @brief Register Identity flow (P1=0x01, struct type 0x2d)
 *
 * Flow:
 *  1. Parse TLV payload (name + scope + identifier + derivation_path)
 *  2. Coin-app validation: handle_check_identity()
 *  3. Display UI: contact name + identifier
 *  4. On confirm: compute HMAC Proof of Registration and send to host
 *
 * Active under HAVE_ADDRESS_BOOK.
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

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

typedef struct {
    identity_t     *identity;
    TLV_reception_t received_tags;
} s_identity_ctx;

/* Private macros-------------------------------------------------------------*/
#define REGISTER_IDENTITY_TAGS(X)                                             \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0xf1, TAG_SCOPE, handle_scope, ENFORCE_UNIQUE_TAG)                      \
    X(0xf2, TAG_ACCOUNT_IDENTIFIER, handle_identifier, ENFORCE_UNIQUE_TAG)    \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x23, TAG_CHAIN_ID, handle_chain_id, ENFORCE_UNIQUE_TAG)                \
    X(0x51, TAG_BLOCKCHAIN_FAMILY, handle_blockchain_family, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/
static identity_t                 IDENTITY     = {0};
static nbgl_contentTagValueList_t ui_pairsList = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_identity_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_REGISTER_IDENTITY)) {
        PRINTF("[Register Identity] Invalid STRUCTURE_TYPE value\n");
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
static bool handle_struct_version(const tlv_data_t *data, s_identity_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Register Identity] Invalid STRUCTURE_VERSION value\n");
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
 * @brief Handler for tag \ref SCOPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_scope(const tlv_data_t *data, s_identity_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->identity->scope, sizeof(context->identity->scope))) {
        PRINTF("SCOPE: failed to parse\n");
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
 * @brief Handler for tag \ref DERIVATION_PATH
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->identity->bip32_path);
}

/**
 * @brief Handler for tag \ref CHAIN_ID
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_chain_id(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_chain_id(data, &context->identity->chain_id);
}

/**
 * @brief Handler for tag \ref BLOCKCHAIN_FAMILY
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_blockchain_family(const tlv_data_t *data, s_identity_ctx *context)
{
    return address_book_handle_blockchain_family(data, &context->identity->blockchain_family);
}

DEFINE_TLV_PARSER(REGISTER_IDENTITY_TAGS, NULL, identity_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_identity_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_SCOPE,
                                          TAG_ACCOUNT_IDENTIFIER,
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
    PRINTF("[Register Identity] - Retrieved Descriptor:\n");
    PRINTF("[Register Identity] -    Contact name:        %s\n", context->identity->contact_name);
    if (context->identity->scope[0] != '\0') {
        PRINTF("[Register Identity] -    Contact scope:       %s\n", context->identity->scope);
    }
    if (bip32_path_format_simple(&context->identity->bip32_path, out, sizeof(out))) {
        PRINTF("[Register Identity] -    Derivation path[%d]: %s\n",
               context->identity->bip32_path.length,
               out);
    }
    else {
        PRINTF("[Register Identity] -    Derivation path length[%d] (failed to format)\n",
               context->identity->bip32_path.length);
    }
    PRINTF("[Register Identity] -    Blockchain family:   %s\n",
           FAMILY_AS_STR(context->identity->blockchain_family));
    if (context->identity->blockchain_family == FAMILY_ETHEREUM) {
        PRINTF("[Register Identity] -    Chain ID:            %llu\n", context->identity->chain_id);
    }
}

/**
 * @brief Build and send response
 *
 * Generate group_handle, compute both HMAC proofs, and send them to the host.
 * Response format: TYPE_REGISTER_IDENTITY(1) | group_handle(64) | hmac_name(32) | hmac_rest(32)
 * = 129 B
 *
 * @return true if successful, false otherwise
 */
static bool build_and_send_response(void)
{
    uint8_t group_handle[GROUP_HANDLE_SIZE] = {0};
    uint8_t hmac_name[CX_SHA256_SIZE]       = {0};
    uint8_t hmac_rest[CX_SHA256_SIZE]       = {0};
    bool    ok                              = false;

    if (!address_book_generate_group_handle(&IDENTITY.bip32_path, group_handle)) {
        PRINTF("[Register Identity] Error: Failed to generate group handle\n");
        goto end;
    }
    // gid is the first GID_SIZE bytes of group_handle
    if (!address_book_compute_hmac_name(
            &IDENTITY.bip32_path, group_handle, IDENTITY.contact_name, hmac_name)) {
        PRINTF("[Register Identity] Error: Failed to compute HMAC_NAME\n");
        goto end;
    }
    if (!address_book_compute_hmac_rest(&IDENTITY.bip32_path,
                                        group_handle,
                                        IDENTITY.scope,
                                        IDENTITY.identifier,
                                        IDENTITY.identifier_len,
                                        IDENTITY.blockchain_family,
                                        IDENTITY.chain_id,
                                        hmac_rest)) {
        PRINTF("[Register Identity] Error: Failed to compute HMAC_REST\n");
        goto end;
    }
    ok = address_book_send_register_identity_response(group_handle, hmac_name, hmac_rest);

end:
    explicit_bzero(group_handle, sizeof(group_handle));
    explicit_bzero(hmac_name, sizeof(hmac_name));
    explicit_bzero(hmac_rest, sizeof(hmac_rest));
    return ok;
}

/**
 * @brief NBGL callback invoked after the user confirms or rejects the registration
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_choice(bool confirm)
{
    if (confirm) {
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Saved to your Contacts", true, finalize_ui_register_identity);
        }
        else {
            PRINTF("[Register Identity] Error: Failed to build and send HMAC proof\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error during registration", false, finalize_ui_register_identity);
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
static void ui_display(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 4;  // name + scope + identifier + network
    ui_pairsList.callback = get_register_identity_tagValue;
    ui_pairsList.wrapping = true;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Review contact details",
                            NULL,
                            "Confirm contact details?",
                            review_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Register a new identity
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
    memset(&IDENTITY, 0, sizeof(IDENTITY));

    // Parse using SDK TLV parser
    if (!identity_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Register Identity] Failed to parse TLV payload\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Check the Identity validity according to the Coin application logic
    if (!handle_check_identity(ctx.identity)) {
        PRINTF("[Register Identity] Error: Identity rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    // Display confirmation UI
    ui_display();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
