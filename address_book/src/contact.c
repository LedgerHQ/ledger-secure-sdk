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
 * @file contact.c
 * @brief Register Contact flow (V2 - "Contacts" proposal)
 *
 * Flow:
 *  1. Parse TLV payload (name + identity_pubkey + derivation_path + PKI sig)
 *  2. Verify Ledger PKI signature (authenticates the Ledger backend payload)
 *  3. Coin-app validation: handle_check_contact()
 *  4. Display UI: contact name + identity public key (hex)
 *  5. On confirm: compute HMAC Proof of Registration and send to host
 *
 * TLV structure type: TYPE_REGISTER_CONTACT (0x14)
 */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "lcx_ecdsa.h"
#include "lcx_sha256.h"
#include "os_utils.h"
#include "os_apdu.h"
#include "os_pki.h"
#include "ledger_pki.h"
#include "status_words.h"
#include "tlv_library.h"
#include "buffer.h"
#include "bip32.h"
#include "address_book.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "address_book_common.h"
#include "contact.h"
#include "io.h"
#include "nbgl_use_case.h"

#if defined(HAVE_ADDRESS_BOOK) && defined(HAVE_ADDRESS_BOOK_CONTACTS)

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION 0x01

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    contact_t      *contact;
    uint8_t         sig_size;
    const uint8_t  *sig;
    cx_sha256_t     hash_ctx;
    TLV_reception_t received_tags;
} s_contact_ctx;

/* Private macros-------------------------------------------------------------*/
#define REGISTER_CONTACT_TAGS(X)                                              \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0x25, TAG_IDENTITY_PUBKEY, handle_identity_pubkey, ENFORCE_UNIQUE_TAG)  \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_contact_ctx *context);

/* Private variables ---------------------------------------------------------*/
static contact_t                  CONTACT      = {0};
static nbgl_contentTagValueList_t ui_pairsList = {0};

/* Private functions ---------------------------------------------------------*/

static bool handle_struct_type(const tlv_data_t *data, s_contact_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_REGISTER_CONTACT);
}

static bool handle_struct_version(const tlv_data_t *data, s_contact_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_version(data, STRUCT_VERSION);
}

static bool handle_contact_name(const tlv_data_t *data, s_contact_ctx *context)
{
    if (!address_book_handle_printable_string(data,
                                              (char *) context->contact->contact_name,
                                              sizeof(context->contact->contact_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

static bool handle_identity_pubkey(const tlv_data_t *data, s_contact_ctx *context)
{
    buffer_t buf = {0};
    if (!get_buffer_from_tlv_data(data, &buf, IDENTITY_PUBKEY_LENGTH, IDENTITY_PUBKEY_LENGTH)) {
        PRINTF("IDENTITY_PUBKEY: expected %d bytes\n", IDENTITY_PUBKEY_LENGTH);
        return false;
    }
    // Compressed key: must start with 0x02 or 0x03
    if (buf.ptr[0] != 0x02 && buf.ptr[0] != 0x03) {
        PRINTF("IDENTITY_PUBKEY: not a compressed secp256r1 key\n");
        return false;
    }
    memmove(context->contact->identity_pubkey, buf.ptr, IDENTITY_PUBKEY_LENGTH);
    return true;
}

static bool handle_derivation_path(const tlv_data_t *data, s_contact_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->contact->bip32_path);
}

static bool handle_signature(const tlv_data_t *data, s_contact_ctx *context)
{
    return address_book_handle_signature(data, &context->sig, &context->sig_size);
}

DEFINE_TLV_PARSER(REGISTER_CONTACT_TAGS, &common_handler, contact_tlv_parser)

static bool common_handler(const tlv_data_t *data, s_contact_ctx *context)
{
    return address_book_hash_handler(data, &context->hash_ctx);
}

static bool verify_fields(const s_contact_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_IDENTITY_PUBKEY,
                                          TAG_DERIVATION_PATH,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in Register Contact descriptor!\n");
    }
    return result;
}

static bool verify_struct(const s_contact_ctx *context)
{
    if (!verify_fields(context)) {
        return false;
    }
    if (!address_book_verify_signature(
            (cx_sha256_t *) &context->hash_ctx, context->sig, context->sig_size)) {
        PRINTF("Signature verification failed for Register Contact!\n");
        return false;
    }
    PRINTF("[Register Contact] name=%s\n", context->contact->contact_name);
    PRINTF("[Register Contact] pubkey=%.*h\n",
           IDENTITY_PUBKEY_LENGTH,
           context->contact->identity_pubkey);
    return true;
}

/**
 * @brief Compute HMAC proof and send response to host
 */
static bool build_and_send_response(void)
{
    uint8_t hmac_proof[32] = {0};

    if (!address_book_compute_hmac_proof(
            &CONTACT.bip32_path, CONTACT.contact_name, CONTACT.identity_pubkey, hmac_proof)) {
        PRINTF("Error: Failed to compute HMAC proof\n");
        return false;
    }

    bool ok = address_book_send_hmac_proof(TYPE_REGISTER_CONTACT, hmac_proof);
    explicit_bzero(hmac_proof, sizeof(hmac_proof));
    return ok;
}

static void review_register_contact_choice(bool confirm)
{
    if (confirm) {
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Contact registered", true, finalize_ui_register_contact);
        }
        else {
            PRINTF("Error: Failed to build and send HMAC proof\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error registering contact", false, finalize_ui_register_contact);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_register_contact);
    }
}

static void ui_display_register_contact(void)
{
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    ui_pairsList.nbPairs  = 2;  // name + pubkey
    ui_pairsList.callback = get_register_contact_tagValue;
    ui_pairsList.wrapping = true;

    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Register contact",
                            NULL,
                            "Confirm contact registration?",
                            review_register_contact_choice);
}

/* Exported functions --------------------------------------------------------*/

bolos_err_t register_contact(uint8_t *buffer_in, size_t buffer_in_length)
{
    const buffer_t payload = {.ptr = buffer_in, .size = buffer_in_length};
    s_contact_ctx  ctx     = {0};

    ctx.contact = &CONTACT;
    cx_sha256_init(&ctx.hash_ctx);

    if (!contact_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        return SWO_INCORRECT_DATA;
    }
    if (!verify_struct(&ctx)) {
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }
    if (!handle_check_contact(ctx.contact)) {
        PRINTF("Error: Contact rejected by coin application\n");
        return SWO_WRONG_PARAMETER_VALUE;
    }

    ui_display_register_contact();
    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK && HAVE_ADDRESS_BOOK_CONTACTS
