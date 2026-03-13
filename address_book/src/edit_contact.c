/* SPDX-FileCopyrightText: © 2026 Ledger SAS */
/* SPDX-License-Identifier: Apache-2.0 */
/**
 * @file edit_contact.c
 * @brief Generic handler for editing contact names in address book
 *
 * This file contains SDK-level logic for editing the contact_name field
 * of address book entries using TLV format with encrypted data.
 */

#include <string.h>
#include "external_address.h"
#include "address_book_entrypoints.h"
#include "address_book_crypto.h"
#include "address_book_common.h"
#include "io.h"
#include "os_utils.h"
#include "lcx_ecfp.h"
#include "lcx_aes_gcm.h"
#include "lcx_sha256.h"
#include "crypto_helpers.h"
#include "tlv_library.h"
#include "buffer.h"
#include "bip32.h"
#include "ledger_pki.h"
#include "os_pki.h"
#include "nbgl_use_case.h"

#ifdef HAVE_ADDRESS_BOOK

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION    0x01
#define TYPE_EDIT_CONTACT 0x13

#define AES_GCM_IV_SIZE  12
#define AES_GCM_TAG_SIZE 16

// Encrypted block size limits
#define MIN_ENCRYPTED_BLOCK_SIZE 32  // len(1) + min_ct(1) + len(1) + IV(12) + len(1) + tag(16)
#define MAX_ENCRYPTED_NAME_BLOCK 65  // len(1) + max_ct(1+33) + len(1) + IV(12) + len(1) + tag(16)

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
    buffer_t        cypher_name;  // Encrypted name block from previous response
    edit_contact_t *edit_contact;
    uint8_t         sig_size;
    const uint8_t  *sig;
    cx_sha256_t     hash_ctx;
    TLV_reception_t received_tags;
} s_edit_contact_ctx;

/* Private macros-------------------------------------------------------------*/
/* TLV Tag Definitions -------------------------------------------------------*/
#define EDIT_CONTACT_TAGS(X)                                                  \
    X(0x01, TAG_STRUCTURE_TYPE, handle_struct_type, ENFORCE_UNIQUE_TAG)       \
    X(0x02, TAG_STRUCTURE_VERSION, handle_struct_version, ENFORCE_UNIQUE_TAG) \
    X(0x0A, TAG_CONTACT_NAME, handle_contact_name, ENFORCE_UNIQUE_TAG)        \
    X(0x21, TAG_DERIVATION_PATH, handle_derivation_path, ENFORCE_UNIQUE_TAG)  \
    X(0x0D, TAG_CYPHER_NAME, handle_cypher_name, ENFORCE_UNIQUE_TAG)          \
    X(0x15, TAG_DER_SIGNATURE, handle_signature, ENFORCE_UNIQUE_TAG)

/* Private functions prototypes ----------------------------------------------*/
static bool common_handler(const tlv_data_t *data, s_edit_contact_ctx *context);

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Global structure to store the Names
static edit_contact_t EDIT_CONTACT = {0};
// UI variables
static nbgl_contentTagValue_t     ui_pairs[2]  = {0};
static nbgl_contentTagValueList_t ui_pairsList = {0};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Handler for tag \ref STRUCTURE_TYPE.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_type(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    UNUSED(context);
    return tlv_check_structure_type(data, TYPE_EDIT_CONTACT);
}

/**
 * @brief Handler for tag \ref STRUCTURE_VERSION.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_struct_version(const tlv_data_t *data, s_edit_contact_ctx *context)
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
static bool handle_contact_name(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    if (!address_book_handle_printable_string(
            data, context->edit_contact->new_name, sizeof(context->edit_contact->new_name))) {
        PRINTF("CONTACT_NAME: failed to parse\n");
        return false;
    }
    return true;
}

/**
 * @brief Handler for tag \ref DERIVATION_PATH.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_derivation_path(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    return address_book_handle_derivation_path(data, &context->edit_contact->bip32_path);
}

/**
 * @brief Handler for tag \ref CYPHER_NAME.
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_cypher_name(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    if (!get_buffer_from_tlv_data(
            data, &context->cypher_name, MIN_ENCRYPTED_BLOCK_SIZE, MAX_ENCRYPTED_NAME_BLOCK)) {
        PRINTF("CYPHER_NAME: failed to extract\n");
        return false;
    }
    return true;
}

/**
 * Handler for tag \ref DER_SIGNATURE
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return whether the handling was successful
 */
static bool handle_signature(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    return address_book_handle_signature(data, &context->sig, &context->sig_size);
}

/**
 * @brief Define the TLV parser for External Address.
 *
 * This macro generates a TLV parser function named ext_addr_tlv_parser that processes
 * External Address payloads according to the EXTERNAL_ADDRESS_TAGS specification.
 * The parser iterates through TLV-encoded data, invokes specific tag handlers, and
 * calls the common_handler for additional processing (such as hashing).
 */
DEFINE_TLV_PARSER(EDIT_CONTACT_TAGS, &common_handler, edit_contact_tlv_parser)

/**
 * @brief Common handler that hashes all tags except signature
 *
 * @param[in] data the tlv data
 * @param[in] context the received payload
 * @return true on success
 */
static bool common_handler(const tlv_data_t *data, s_edit_contact_ctx *context)
{
    return address_book_hash_handler(data, &context->hash_ctx);
}

/**
 * @brief Verify the payload signature
 *
 * Verify the SHA-256 hash of the payload against the public key
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_signature(const s_edit_contact_ctx *context)
{
    return address_book_verify_signature(
        (cx_sha256_t *) &context->hash_ctx, context->sig, context->sig_size);
}

/**
 * @brief Verify the received fields
 *
 * Check the mandatory fields are present
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_fields(const s_edit_contact_ctx *context)
{
    bool result = TLV_CHECK_RECEIVED_TAGS(context->received_tags,
                                          TAG_STRUCTURE_TYPE,
                                          TAG_STRUCTURE_VERSION,
                                          TAG_CONTACT_NAME,
                                          TAG_DERIVATION_PATH,
                                          TAG_CYPHER_NAME,
                                          TAG_DER_SIGNATURE);
    if (!result) {
        PRINTF("Missing mandatory fields in Edit Contact TLV!\n");
        return false;
    }
    return true;
}

/**
 * @brief Print the received descriptor.
 *
 * @param[in] context the received payload
 * Only for debug purpose.
 */
static void print_payload(const s_edit_contact_ctx *context)
{
    UNUSED(context);
    char out[50] = {0};
    PRINTF("****************************************************************************\n");
    PRINTF("[Edit Contact] - Retrieved Descriptor:\n");
    PRINTF("[Edit Contact] -    Contact name: %s\n", context->edit_contact->new_name);
    if (bip32_path_format_simple(&context->edit_contact->bip32_path, out, sizeof(out))) {
        PRINTF("[Edit Contact] -    Derivation path[%d]: %s\n",
               context->edit_contact->bip32_path.length,
               out);
    }
    else {
        PRINTF("[Edit Contact] -    Derivation path length[%d] (failed to format)\n",
               context->edit_contact->bip32_path.length);
    }
}

/**
 * @brief Verify the struct
 *
 * Verify the SHA-256 hash of the payload against the public key
 *
 * @param[in] context the received payload
 * @return whether it was successful
 */
static bool verify_struct(const s_edit_contact_ctx *context)
{
    if (!verify_fields(context)) {
        PRINTF("Error: Missing mandatory fields in Edit Contact descriptor!\n");
        return false;
    }

    if (!verify_signature(context)) {
        PRINTF("Error: Signature verification failed for Edit Contact descriptor!\n");
        return false;
    }
    print_payload(context);
    return true;
}

/**
 * @brief Build and send encrypted response
 *
 * This function constructs the Edit Contact entry payload and delegates
 * to the generic crypto function for signing, encryption and sending.
 *
 * @return true if successful, false otherwise
 */
static bool build_and_send_response(void)
{
    uint8_t name_payload[EXTERNAL_NAME_LENGTH] = {0};
    size_t  name_payload_len;

    // Build name payload
    if (!build_name_payload(
            EDIT_CONTACT.new_name, name_payload, sizeof(name_payload), &name_payload_len)) {
        PRINTF("Error: Failed to build name payload\n");
        return false;
    }

    // Send encrypted response with only the new name (no remaining block for Edit Contact)
    return address_book_encrypt_and_send(
        TYPE_EDIT_CONTACT, &EDIT_CONTACT.bip32_path, name_payload, name_payload_len, NULL, 0);
}

/**
 * @brief Callback when user confirms or rejects the Edit Contact action
 *
 * @param[in] confirm true if user confirmed, false if rejected
 */
static void review_edit_contact_choice(bool confirm)
{
    if (confirm) {
        // Build and send encrypted response to host
        if (build_and_send_response()) {
            nbgl_useCaseStatus("Contact name changed", true, finalize_ui_edit_contact);
        }
        else {
            PRINTF("Error: Failed to build and send response\n");
            io_send_sw(SWO_INCORRECT_DATA);
            nbgl_useCaseStatus("Error changing contact name", false, finalize_ui_edit_contact);
        }
    }
    else {
        io_send_sw(SWO_INCORRECT_DATA);
        nbgl_useCaseReviewStatus(STATUS_TYPE_OPERATION_REJECTED, finalize_ui_edit_contact);
    }
}

/**
 * @brief Display the confirmation screen for editing a contact name
 */
static void ui_confirm_edit_contact(void)
{
    uint8_t pairIdx = 0;

    // Set up pairs
    memset(ui_pairs, 0, sizeof(ui_pairs));
    memset(&ui_pairsList, 0, sizeof(ui_pairsList));
    pairIdx                 = 0;
    ui_pairs[pairIdx].item  = "Previous name";
    ui_pairs[pairIdx].value = EDIT_CONTACT.old_name;
    pairIdx++;
    ui_pairs[pairIdx].item  = "New name";
    ui_pairs[pairIdx].value = EDIT_CONTACT.new_name;
    pairIdx++;
    ui_pairsList.nbPairs  = pairIdx;
    ui_pairsList.pairs    = ui_pairs;
    ui_pairsList.wrapping = true;

    // Display the review screen
    nbgl_useCaseReviewLight(TYPE_OPERATION,
                            &ui_pairsList,
                            &LARGE_ADDRESS_BOOK_ICON,
                            "Review changes to contact details",
                            NULL,
                            "Confirm change?",
                            review_edit_contact_choice);
}

/* Exported Functions ----------------------------------------------------------*/

/**
 * @brief Handle Edit Contact operation
 *
 * Parses the TLV payload, verifies the signature, checks the contact validity and stores it.
 *
 * @param[in] buffer_in Complete TLV payload
 * @param[in] buffer_in_length Payload length
 * @return Status word
 */
bolos_err_t edit_contact(const uint8_t *buffer, size_t buffer_len)
{
    const buffer_t     payload = {.ptr = (uint8_t *) buffer, .size = buffer_len};
    s_edit_contact_ctx ctx     = {0};
    char               out[50] = {0};

    // Init the structure
    ctx.edit_contact = &EDIT_CONTACT;
    // Initialize hash context
    cx_sha256_init(&ctx.hash_ctx);

    // Parse TLV
    if (!edit_contact_tlv_parser(&payload, &ctx, &ctx.received_tags)) {
        PRINTF("Failed to parse Edit Contact TLV payload!\n");
        return SWO_INCORRECT_DATA;
    }

    if (!verify_struct(&ctx)) {
        PRINTF("Failed to verify Edit Contact structure!\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Use derivation path from TLV
    if (bip32_path_format_simple(&ctx.edit_contact->bip32_path, out, sizeof(out))) {
        PRINTF("TLV Derivation path[%d]: %s\n", ctx.edit_contact->bip32_path.length, out);
    }
    else {
        PRINTF("TLV Derivation path length[%d] (failed to format)\n",
               ctx.edit_contact->bip32_path.length);
    }

    // Decrypt cypher_name and extract old contact name directly
    if (!address_book_decrypt((const buffer_t *) &ctx.cypher_name,
                              &ctx.edit_contact->bip32_path,
                              ctx.edit_contact->old_name)) {
        PRINTF("Error: Cypher name decryption failed!\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    // Display confirmation UI
    ui_confirm_edit_contact();

    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
