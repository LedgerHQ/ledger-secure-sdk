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
 * @file identity_edit_contact_name.c
 * @brief Edit Contact Name flow (P1=0x02, struct type 0x2e)
 *
 * Allows changing the CONTACT_NAME of an existing Identity contact.
 * Because HMAC_PROOF covers only (gid, name), the wallet does not need
 * to transmit the identifier, scope, or network fields.
 *
 * Flow:
 *  1. Parse TLV payload (gid + new_name + previous_name +
 *     derivation_path + hmac_proof)
 *  2. Verify HMAC_PROOF over (gid, previous_name)
 *  3. Display UI: old name → new name
 *  4. On confirm: compute new HMAC_PROOF over (gid, new_name) and send
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

/* Private types, structures, unions -----------------------------------------*/

typedef struct {
    edit_contact_name_t *edit;
    TLV_reception_t      received_tags;
    uint8_t              hmac_proof[CX_SHA256_SIZE];  ///< HMAC_PROOF of the previous registration
    uint8_t group_handle[GROUP_HANDLE_SIZE];          ///< Group handle from wallet (verified later)
} s_edit_contact_name_ctx;

/* Private macros-------------------------------------------------------------*/
#define EDIT_CONTACT_NAME_TAGS(X)                                             \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0xf0, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0xf3, TAG_PREVIOUS_NAME, handle_previous_name, ENFORCE_UNIQUE_TAG)      \
    X(0xf6, TAG_GROUP_HANDLE, handle_group_handle, ENFORCE_UNIQUE_TAG)        \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x29, TAG_HMAC_PROOF, handle_hmac_proof, ENFORCE_UNIQUE_TAG)

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, TYPE_EDIT_CONTACT_NAME)) {
        PRINTF("[Edit Contact Name] Invalid STRUCTURE_TYPE value\n");
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
static bool handle_struct_version(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    UNUSED(context);
    if (!tlv_enforce_u8_value(data, STRUCT_VERSION)) {
        PRINTF("[Edit Contact Name] Invalid STRUCTURE_VERSION value\n");
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
static bool handle_contact_name(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->edit->contact_name, sizeof(context->edit->contact_name))) {
        PRINTF("[Edit Contact Name] CONTACT_NAME: failed to parse\n");
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
static bool handle_previous_name(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->edit->old_contact_name, sizeof(context->edit->old_contact_name))) {
        PRINTF("[Edit Contact Name] PREVIOUS_CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref GROUP_HANDLE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_group_handle(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, GROUP_HANDLE_SIZE, GROUP_HANDLE_SIZE)) {
        PRINTF("[Edit Contact Name] GROUP_HANDLE: invalid length (expected %d bytes)\n",
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
static bool handle_derivation_path(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->edit->bip32_path);
}

/**
 * @brief Handler for tag \ref HMAC_PROOF
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_hmac_proof(const tlv_data_t *data, s_edit_contact_name_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, CX_SHA256_SIZE, CX_SHA256_SIZE)) {
        PRINTF("[Edit Contact Name] HMAC_PROOF: invalid length (expected %d bytes)\n",
               CX_SHA256_SIZE);
        return false;
    }
    memmove(context->hmac_proof, buf.ptr, CX_SHA256_SIZE);
    return true;
}

DEFINE_TLV_PARSER(EDIT_CONTACT_NAME_TAGS, NULL, edit_contact_name_tlv_parser)

/**
 * @brief Check that all mandatory TLV tags were received
 *
 * @param[in] context the received payload
 * @return whether all mandatory fields are present
 */
static bool verify_fields(const s_edit_contact_name_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_PREVIOUS_NAME,
                                          TAG_GROUP_HANDLE,
                                          TAG_DERIVATION_PATH,
                                          TAG_HMAC_PROOF);
    if (!result) {
        PRINTF("[Edit Contact Name] Missing mandatory fields!\n");
    }
    return result;
}

/**
 * @brief Print the received descriptor
 *
 * @param[in] context the received payload
 * Only for debug purpose.
 */
static void print_payload(const s_edit_contact_name_ctx *context)
{
    UNUSED(context);
    PRINTF("****************************************************************************\n");
    PRINTF("[Edit Contact Name] - Retrieved Descriptor:\n");
    PRINTF("[Edit Contact Name] -    Old name: %s\n", context->edit->old_contact_name);
    PRINTF("[Edit Contact Name] -    New name: %s\n", context->edit->contact_name);
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

    if (!address_book_compute_hmac_proof(&g_ab_payload.edit_contact_name.bip32_path,
                                         g_ab_payload.edit_contact_name.gid,
                                         g_ab_payload.edit_contact_name.contact_name,
                                         hmac_proof)) {
        PRINTF("[Edit Contact Name] Error: Failed to compute new HMAC_PROOF\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_EDIT_CONTACT_NAME, hmac_proof);
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
        bool ok = build_and_send_response();
        if (ok) {
            on_edit_contact_name_applied(&g_ab_payload.edit_contact_name);
        }
        else {
            PRINTF("[Edit Contact Name] Error: Failed to build and send HMAC proof\n");
        }
        address_book_finalize_review(
            ok, "Contact name changed", "Error during update", finalize_ui_edit_contact_name);
    }
    else {
        address_book_handle_review_rejected(finalize_ui_edit_contact_name);
    }
}

/**
 * @brief Display the Edit Contact Name review screen
 */
static void ui_display(void)
{
    uint8_t nbPairs = 0;
    memset(&g_ab_ui.list, 0, sizeof(g_ab_ui.list));
    memset(g_ab_ui.pairs, 0, sizeof(g_ab_ui.pairs));
    g_ab_ui.pairs[nbPairs].item  = "Old name";
    g_ab_ui.pairs[nbPairs].value = g_ab_payload.edit_contact_name.old_contact_name;
    nbPairs++;
    g_ab_ui.pairs[nbPairs].item  = "New name";
    g_ab_ui.pairs[nbPairs].value = g_ab_payload.edit_contact_name.contact_name;
    nbPairs++;
    g_ab_ui.list.pairs   = g_ab_ui.pairs;
    g_ab_ui.list.nbPairs = nbPairs;
    address_book_display_review(&LARGE_ADDRESS_BOOK_ICON,
                                "Review change to contact details",
#ifdef SCREEN_SIZE_WALLET
                                "Confirm change?",
#else
                                "Confirm change",
#endif
                                review_choice);
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Edit the CONTACT_NAME of an existing Identity contact.
 *
 * @param[in] buffer_in        the TLV payload
 * @param[in] buffer_in_length the length of the payload
 * @return bolos_err_t         the result of the edit
 */
bolos_err_t edit_contact_name(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t          payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_edit_contact_name_ctx ctx     = {0};

    // Init the structure
    ctx.edit = &g_ab_payload.edit_contact_name;
    memset(&g_ab_payload.edit_contact_name, 0, sizeof(g_ab_payload.edit_contact_name));

    // Parse using SDK TLV parser
    if (!edit_contact_name_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("[Edit Contact Name] TLV parsing failed\n");
        return SWO_INCORRECT_DATA;
    }
    if (!verify_fields(&ctx)) {
        return SWO_INCORRECT_DATA;
    }
    print_payload(&ctx);

    // Verify the group handle and extract the gid
    if (!address_book_verify_group_handle(&g_ab_payload.edit_contact_name.bip32_path,
                                          ctx.group_handle,
                                          g_ab_payload.edit_contact_name.gid)) {
        PRINTF("[Edit Contact Name] Group handle verification failed\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Verify the wallet holds a valid HMAC_PROOF for the previous name
    if (!address_book_verify_hmac_proof(&g_ab_payload.edit_contact_name.bip32_path,
                                        g_ab_payload.edit_contact_name.gid,
                                        g_ab_payload.edit_contact_name.old_contact_name,
                                        ctx.hmac_proof)) {
        PRINTF("[Edit Contact Name] HMAC_PROOF verification failed\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Display confirmation UI
    ui_display();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
