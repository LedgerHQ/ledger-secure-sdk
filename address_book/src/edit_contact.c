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

#ifdef HAVE_ADDRESS_BOOK

/* Private defines------------------------------------------------------------*/
#define STRUCT_VERSION    0x01
#define TYPE_EDIT_CONTACT 0x13

#define AES_GCM_IV_SIZE  12
#define AES_GCM_TAG_SIZE 16

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
    if (!get_string_from_tlv_data(data,
                                  context->edit_contact->new_name,
                                  1,
                                  sizeof(context->edit_contact->new_name) - 1)) {
        PRINTF("CONTACT_NAME: failed to extract\n");
        return false;
    }
    if (!is_printable_string(context->edit_contact->new_name,
                             strlen(context->edit_contact->new_name))) {
        PRINTF("CONTACT_NAME: not printable\n");
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
    buffer_t path_buffer = {0};
    size_t   nb_elements = 0;
    // Get the raw buffer
    if (!get_buffer_from_tlv_data(data, &path_buffer, 4, MAX_BIP32_PATH * 4)) {
        PRINTF("DERIVATION_PATH: failed to extract\n");
        return false;
    }
    nb_elements = path_buffer.ptr[0];  // First byte is the length in bytes
    path_buffer.ptr++;                 // Skip the first byte which is the length in bytes
    path_buffer.size--;                // Adjust the size accordingly

    // Check BIP32 length
    if (path_buffer.size < sizeof(uint32_t) * nb_elements) {
        PRINTF("DERIVATION_PATH: Invalid data\n");
        return false;
    }

    // Parse BIP32 path
    if (!bip32_path_read(path_buffer.ptr,
                         path_buffer.size,
                         (uint32_t *) context->edit_contact->derivation_path,
                         nb_elements)) {
        PRINTF("DERIVATION_PATH: failed to parse BIP32 path\n");
        return false;
    }
    // Store the path length (number of elements)
    context->edit_contact->derivation_path_length = path_buffer.size / 4;
    return true;
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
    if (!get_buffer_from_tlv_data(data, &context->cypher_name, 32, 200)) {
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
    buffer_t sig = {0};
    if (!get_buffer_from_tlv_data(
            data, &sig, CX_ECDSA_SHA256_SIG_MIN_ASN1_LENGTH, CX_ECDSA_SHA256_SIG_MAX_ASN1_LENGTH)) {
        PRINTF("DER_SIGNATURE: failed to extract\n");
        return false;
    }
    context->sig_size = sig.size;
    context->sig      = sig.ptr;
    return true;
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
    // Hash all tags except the signature
    if (data->tag != TAG_DER_SIGNATURE) {
        CX_ASSERT(cx_hash_no_throw(
            (cx_hash_t *) &context->hash_ctx, 0, data->raw.ptr, data->raw.size, NULL, 0));
    }
    return true;
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
    uint8_t          hash[CX_SHA256_SIZE] = {0};
    const buffer_t   buffer               = {.ptr = hash, .size = sizeof(hash)};
    const buffer_t   signature = {.ptr = (uint8_t *) context->sig, .size = context->sig_size};
    const cx_curve_t curve     = CX_CURVE_SECP256R1;
    const uint8_t    key_usage = CERTIFICATE_PUBLIC_KEY_USAGE_ADDRESS_BOOK;

    if (cx_hash_no_throw((cx_hash_t *) &context->hash_ctx, CX_LAST, NULL, 0, hash, sizeof(hash))
        != CX_OK) {
        PRINTF("Could not finalize TLV hash!\n");
        return false;
    }

    if (check_signature_with_pki(buffer, &key_usage, &curve, signature)
        != CHECK_SIGNATURE_WITH_PKI_SUCCESS) {
        PRINTF("TLV signature verification failed!\n");
        return false;
    }

    return true;
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
    if (bip32_path_format(context->edit_contact->derivation_path,
                          context->edit_contact->derivation_path_length,
                          out,
                          sizeof(out))) {
        PRINTF("[Edit Contact] -    Derivation path[%d]: %s\n",
               context->edit_contact->derivation_path_length,
               out);
    }
    else {
        PRINTF("[Edit Contact] -    Derivation path length[%d] (failed to format)\n",
               context->edit_contact->derivation_path_length);
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
    const buffer_t     payload          = {.ptr = (uint8_t *) buffer, .size = buffer_len};
    s_edit_contact_ctx ctx              = {0};
    const uint32_t    *encrypt_path     = NULL;
    size_t             encrypt_path_len = 0;
    char               out[50]          = {0};

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
    encrypt_path     = ctx.edit_contact->derivation_path;
    encrypt_path_len = ctx.edit_contact->derivation_path_length;
    if (bip32_path_format(encrypt_path, encrypt_path_len, out, sizeof(out))) {
        PRINTF("TLV Derivation path[%d]: %s\n", encrypt_path_len, out);
    }
    else {
        PRINTF("TLV Derivation path length[%d] (failed to format)\n", encrypt_path_len);
    }

    // Decrypt cypher_name and extract old contact name directly
    if (!address_book_decrypt((const buffer_t *) &ctx.cypher_name,
                              encrypt_path,
                              encrypt_path_len,
                              ctx.edit_contact->old_name)) {
        PRINTF("Error: Cypher name decryption failed!\n");
        return SWO_SECURITY_CONDITION_NOT_SATISFIED;
    }

    PRINTF("Old contact name: '%s'\n", ctx.edit_contact->old_name);
    PRINTF("New contact name: '%s'\n", ctx.edit_contact->new_name);

    // Pass properly null-terminated strings to UI handler
    handle_edit_contact(ctx.edit_contact->old_name, ctx.edit_contact->new_name);

    // TODO: Implement re-encryption logic here
    PRINTF("Edit contact: functionality not yet implemented\n");

    return SWO_NO_RESPONSE;
}

#endif  // HAVE_ADDRESS_BOOK
